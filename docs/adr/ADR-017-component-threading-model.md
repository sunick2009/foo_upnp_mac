# ADR-017: Component 執行緒模型

**狀態：** Accepted
**日期：** 2026-07-03（M3 grilling session）

## Context

`HttpClient` 是同步阻塞設計（ADR-008），每次展開 tree 節點都是
至少一次網路往返（分頁時多次），不能在 AppKit 主執行緒上呼叫。
候選：背景 thread + 回主緒、fb2k `threaded_process` modal 對話框、
改寫 async HttpClient。

## Decision

**背景 `std::thread` 執行 blocking browse，完成後回主執行緒更新 UI。**

- 展開節點時節點顯示 loading 狀態，背景 thread 跑
  `ContentDirectoryClient::browse`（含分頁迴圈，見 ADR-012 修訂）。
- 結果回主緒：用 fb2k 的 `main_thread_callback`（SDK 已編譯可用）
  或 `dispatch_async(dispatch_get_main_queue(), ...)`——實作時擇一，
  以「component 卸載時能安全取消/忽略回呼」為準。
- 每個 tree 節點同時間至多一個 in-flight browse；重複展開不重發。
- 取消語意 v1 從簡：關窗/卸載後回呼落地時發現 UI 已不在就丟棄
  結果，不強求中斷進行中的 HTTP 請求（timeout 會自然收尾）。
  （2026-07-18 修訂：傳輸 timeout 由 10s 放寬為 30s／連線維持 10s，
  因真實 server 單頁 Browse 可能超過 10s；為補償取消延遲，
  `fetchAllChildren` 增加分頁之間的取消輪詢，遞迴與隨選加入均使用，
  故取消回應時間上限是「單頁」而非「單 container」的傳輸時間。）

不選 `threaded_process`：每次展開都跳 modal 進度框，瀏覽體驗
無法接受。不改 async HttpClient：推翻 ADR-008/009，core 與測試
全部重寫，v1 成本過高。

## Consequences

- ADR-008 的同步設計保留，非同步性由 component 層提供。
- UI 狀態機（idle / loading / loaded / failed per node）是純邏輯，
  抽成可測的 C++/ObjC++ 類別。
- 背景 thread 拿的是 `HttpClient` 實例；執行緒安全策略：每次
  browse 用獨立的 client 實例（curl easy handle 不跨緒共用）。
