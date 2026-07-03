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
