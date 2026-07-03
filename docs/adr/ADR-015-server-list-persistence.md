# ADR-015: Server 清單持久化

**狀態：** Accepted
**日期：** 2026-07-03（M3 grilling session）

## Context

v1 的 server 來源是手動輸入的 device description URL（每筆：顯示
名稱 + rootDesc URL）。foo_upnp 把設定存在 fb2k 設定系統，隨
profile 備份與搬移。候選方案：cfg_var、新式 configStore 逐筆、
自管 JSON 檔。

## Decision

**單一 `cfg_var<string>`（fb2k 設定系統）存 JSON 陣列。**

```json
[{"name": "客廳 NAS", "url": "http://10.0.0.2:2333/DeviceDescription.xml"}]
```

理由：

- 跟 fb2k profile 備份/搬移一起走，對齊 foo_upnp 行為。
- `cfg_var` 已由 foo_sample 驗證在 mac 可用（`cfg_uint` 實例）。
- 專案已有 JSON 序列化經驗（CLI 的 jsonEscape/jsonString），
  序列化邏輯是純 C++，可放 adapter 層用 Catch2 測。
- configStore 逐筆 API 較新、mac 行為未驗證，且要自管 key 列舉；
  自管檔案則與 fb2k 設定體系分裂。

## Consequences

- 新增 `ServerListStore`（純 C++：JSON ↔ `std::vector<ServerEntry>`）
  進 adapter 層 + 單元測試；cfg_var 讀寫只是薄封裝。
- Schema 內含於 JSON 物件欄位，未來加欄位（如 auth）向後相容：
  未知欄位保留、缺欄位給預設值。
- 清單異動即寫回（cfg_var 賦值），不做延遲儲存。
