# ADR-014: Component UI 形式與建構方式

**狀態：** Accepted
**日期：** 2026-07-03（M3 grilling session，見 `docs/20_m3_grilling_session.md`）

## Context

M3 的方向是「UX 盡可能對齊 Windows 版 foo_upnp」：可停靠的 browser
tree panel、雙擊加入 playlist、右鍵選單。mac SDK 提供的 UI 整合面：

- `preferences_page::instantiate()` → 回傳包裝的 NSViewController。
- `ui_element_mac` → layout element，同樣回傳 NSViewController，
  **但 SDK 零文件、foo_sample 零範例**（僅 header + GUID）。
- contextmenu / mainmenu API（foo_sample mac target 有編譯，可用）。

另一個關鍵限制：foo_sample 的 mac UI 全部用 `.xib`，但編譯 xib 的
`ibtool` **不在 Command Line Tools 裡**（`xcrun --find ibtool` 失敗）。
照抄 foo_sample 會讓本地 build 需要完整 Xcode，直接觸發 ADR-013
的 fallback 情境。

## Decision

1. **UI 形式：先獨立 NSWindow，後 ui_element_mac。**
   M3 MVP 以主選單命令開啟獨立 browser 視窗（NSWindow +
   NSOutlineView tree）。核心 UI 做成可重用的 NSViewController，
   M4 再把同一個 view controller 掛進 `ui_element_mac` 變成可停靠
   panel。先用低風險路徑真機驗證，不把 MVP 押在無文件的 API 上。
2. **全程式碼建 UI，零 xib。**
   所有 view 以 ObjC++ 程式碼建構（NSOutlineView、NSStackView、
   Auto Layout anchors）。ADR-013 的「CLT 就能編」策略完整保留，
   本地與 CI 走同一條路。
3. **錯誤呈現：節點內顯示 + console。**
   Browse 失敗時在 tree 該節點下顯示可重試的「載入失敗：<原因>」
   列（利用既有例外階層區分 Http / SoapFault / Network / Parse），
   詳細診斷寫 fb2k console（對齊 foo_upnp 行為）。不彈 modal。

## Consequences

- Preferences page（server 清單管理）同樣 programmatic，不用
  foo_sample 的 initWithNibName 模式，改覆寫 `loadView`。
- `ui_element_mac` 的可行性驗證移到 M4 開頭；若屆時發現 mac 版
  layout 系統對第三方 element 支援有缺，獨立視窗已是可用產品，
  不影響 M3 交付。
- UI 程式碼較囉唆，換來單一 build 路徑與 CI 可重現。

## Revision: M4 `ui_element_mac` feasibility（2026-07-04）

### Context

SDK 的 `ui_element_mac` header 只有四個方法：`instantiate()` 回傳
wrapped `NSViewController`、`match_name()`、`get_name()`、`get_guid()`。
沒有 mac sample，也沒有 layout host 的行為文件。Preferences page 已用
`fb2k::wrapNSObject()` 成功回傳 programmatic `NSViewController`，因此
最小可行路徑是把相同 wrapping 模式套用到 browser view controller。

### Decision

- 新增一個 `ui_element_mac` service，名稱與現有主選單一致為
  `DMS Browser`，`instantiate()` 每次建立新的
  `DmsBrowserViewController` 並以 `fb2k::wrapNSObject()` 回傳。
- 獨立視窗保留。layout element 只是第二個宿主，不共用 singleton window
  controller，也不改變 View → DMS Browser 的使用方式。
- `DmsBrowserViewController` 繼續作為主要 UI owner；server 清單刷新、
  browse worker queue、playlist integration 均由同一個 controller 管理。

### Consequences

- compile/link 已驗證 `ui_element_mac` service 可在 mac component target
  中註冊，且不需要 `.xib` 或完整 Xcode。
- 尚需真機手動驗收：foobar2000 mac layout 設定是否列出第三方
  `DMS Browser` element、加入 layout 後是否正確 instantiate、關閉/移除
  element 時 `viewDidDisappear` 與背景回呼是否符合預期。
- 若 host UI 不暴露第三方 `ui_element_mac`，現有獨立視窗仍是支援路徑；
  此情況應視為 host 能力限制，而非退回 xib 或改寫 UI 架構。

## Revision: M4 album art preview（2026-07-04）

### Context

DIDL parser 已保留 `upnp:albumArtURI`。fb2k SDK 的 `album_art_*` API 主要
用於從媒體檔或既有 metadb handle 讀取/寫入封面；它沒有明確路徑可把
DIDL item 上的任意 HTTP album art URL 註冊成 browse-info artwork。若為
了單一預覽功能自行建立 HTTP 圖片快取，會引入 cache invalidation 與磁碟
清理問題，超出 M4 需求。

### Decision

- Browser UI 在選取 item 時顯示一個 96px album art 預覽與簡短 metadata。
- 圖片來源只使用該 item 的 `albumArtURI`，以 `NSURLSession` 非同步抓取；
  不阻塞 browse worker queue，不建立自有持久化 cache。
- 選取改變時使用 generation counter 丟棄過期圖片回呼，避免慢回應覆蓋
  新選取項。

### Consequences

- 使用者能在 browser panel/視窗中看到 server 提供的 album art。
- playlist 或 fb2k 全域 artwork panel 仍由 fb2k 自身的 metadata/tag 掃描
  決定；本 revision 不聲稱把 DIDL album art 注入 metadb artwork。
- 需要手動驗收圖片載入成功、選取快速切換不錯圖、無 album art 的曲目不保留
  上一張圖片。
