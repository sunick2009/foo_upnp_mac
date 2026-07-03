# ADR-012: Browse Pagination 策略

**狀態：** Accepted  
**日期：** 2026-07-03

## Context

ContentDirectory Browse 支援 pagination：

```
Browse(objectId, BrowseDirectChildren, "", startingIndex=0, requestedCount=100, ...)
→ NumberReturned=100, TotalMatches=450
```

若 container 有 450 個 item，第一次只拿到 100 個。
要全部拿到需要 5 次 request。

問題：**Phase 0 CLI 要不要自動翻頁？**

## Options

### Option A: 只拿第一頁（Phase 0）

CLI `--requested-count` 預設 100（或 UPnP 的 0 = all，依 server 實作）。
回傳的 JSON 包含 `totalMatches` 和 `numberReturned`，讓使用者知道是否有更多。

### Option B: 自動翻頁，拿全部

自動 loop，直到 `startingIndex + numberReturned >= totalMatches`。
每頁等待一次 HTTP round-trip，大型目錄可能很慢。

## Decision

**Phase 0 只拿第一頁（Option A）。**

理由：
- Phase 0 目標是驗證 browse 可行，不是完整 UPnP client。
- 自動翻頁在 CLI 有意義，但在 component 裡通常是 lazy loading（使用者滾動才拿下一頁）。
- `ContentDirectoryClient::browse()` 的 API 設計要能支援 pagination，
  但 Phase 0 的呼叫端只呼叫一次。

### ContentDirectoryClient API 設計

```cpp
struct BrowseParams {
    std::string objectId = "0";
    BrowseFlag browseFlag = BrowseFlag::DirectChildren;
    std::string filter = "*";
    uint32_t startingIndex = 0;
    uint32_t requestedCount = 100;   // 0 = server decides
    std::string sortCriteria = "";
};

struct BrowseResponse {
    std::vector<UpnpObject> objects;
    uint32_t numberReturned;
    uint32_t totalMatches;
    uint32_t updateId;
};

BrowseResponse ContentDirectoryClient::browse(const BrowseParams& params);
```

### CLI 行為

```bash
upnp-browser-cli browse --server http://... --object-id 0 --count 100

# 輸出：
{
  "number_returned": 100,
  "total_matches": 450,
  "note": "Use --starting-index 100 to get next page",
  "items": [...]
}
```

若 `requestedCount == 0`，讓 server 自行決定（有些 server 支援「返回全部」）。
若 server 回傳的 `NumberReturned < requestedCount`，代表已是最後一頁。

## Phase 1 延伸（已由 2026-07-03 修訂取代）

~~Phase 1 component 的 Browser Panel 使用 lazy loading~~ — 見下方修訂。

## 修訂（2026-07-03，M3 grilling session）

Component 的策略改為**展開節點時在背景執行緒迴圈翻頁抓到齊**
（`startingIndex` 累加直到累計 `numberReturned >= totalMatches`），
不做滾動 lazy loading：

- 對齊 foo_upnp 行為：container 展開即呈現完整內容，使用者不會
  誤以為曲目不見了。
- 安全上限 10,000 項，超過即停止並在 UI 標示截斷；整個迴圈在
  背景 thread（ADR-017），可因 UI 關閉而放棄。
- 翻頁迴圈由 component 的 adapter 層實作（純 C++、可測），
  `ContentDirectoryClient::browse()` 仍是單次呼叫、不自動翻頁。

**CLI 行為不變**（單頁 + totalMatches 提示），本 ADR 的 Phase 0
決策仍然有效。

## Consequences

- `ContentDirectoryClient::browse()` 接受 `BrowseParams`，不自動翻頁。
- CLI 的 `--starting-index` 和 `--count` 參數直接對應 SOAP Browse 的同名欄位。
- Component 的翻頁迴圈（fetch-all with cap）是獨立的 adapter 函式，
  以 fake client 注入測試多頁/截斷/短頁（NumberReturned < requested）情境。
