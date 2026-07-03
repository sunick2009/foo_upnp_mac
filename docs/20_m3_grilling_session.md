# M3 Grilling Session — Component 設計決策

**日期：** 2026-07-03
**指令：** 「我們盡可能的根據 Windows 版 foo_upnp 實作」
**前提功課（事實依據）：**

- mac SDK UI 整合面：`preferences_page::instantiate()`（NSViewController）、
  `ui_element_mac`（有介面無文件無範例）、contextmenu/mainmenu（foo_sample
  mac 有編譯）、`metadb_hint_list`（metadb.h 有宣告）。
- `ibtool` 不在 CLT（`xcrun --find ibtool` 失敗）→ xib 路線會破壞 ADR-013。
- Windows foo_upnp 0.99 browser UX：可停靠 tree panel、雙擊加入、
  container 遞迴加入、SSDP 自動探索、DIDL metadata 立即顯示。

## 決策一覽（12 題，全數經使用者確認）

| # | 問題 | 決策 | 落點 |
|---|---|---|---|
| 1 | 「依照 foo_upnp」的界線 | **UX 對齊、守 v1 約束**：browser 體驗照抄，SSDP/renderer/server 仍排除，server 來源維持手動 URL（SSDP 留 M6） | docs/02、docs/06 |
| 2 | 主 UI 形式 | **先獨立 NSWindow（主選單開啟），M4 再掛 ui_element_mac** 成可停靠 panel；共用同一 NSViewController | ADR-014 |
| 3 | UI 建構方式 | **全程式碼建 UI，零 xib**（ibtool 不在 CLT） | ADR-014 |
| 4 | 加入 playlist 語意 | **單層加入**：雙擊 item 加入現行 playlist；container 右鍵加入直接子項曲目；遞迴留 M4 | ADR-016 |
| 5 | Server 清單持久化 | **cfg_var<string> 存 JSON 陣列**，隨 fb2k profile 走 | ADR-015 |
| 6 | Metadata 呈現 | **metadb_hint_list 預填 DIDL** + fb2k 播放時自然補充檔內 tag。使用者澄清後確認：foo_upnp 顯示的資料正是 DIDL 來的 | ADR-016 |
| 7 | 執行緒模型 | **背景 std::thread + 回主緒**（main_thread_callback 或 GCD），節點顯示 loading；不用 threaded_process modal | ADR-017 |
| 8 | 分頁策略 | **Component 迴圈抓到齊**（上限 10,000、背景執行、可放棄）；CLI 維持單頁 | ADR-012 修訂 |
| 9 | 命名 | **bundle `foo_dms_browser.component`、顯示名「DMS Browser」**；不用 foo_upnp 名義（別人的作品），repo 名不動 | docs/06 |
| 10 | 錯誤體驗 | **tree 節點內顯示可重試的失敗列 + 詳情寫 console**；不彈 modal | ADR-014 |
| 11 | 測試策略 | **薄 UI + adapter 層 Catch2 測試**（tree 狀態機、DIDL→hint 映射、ServerListStore JSON、翻頁迴圈）+ 文件化手動測試清單 + marker 式載入測試 | docs/08 精神延續 |
| 12 | Metadata 疑問澄清 | 「遠端看得到資料」的來源 = DIDL（server 給的）+ fb2k 遠端讀 tag（慢、逐檔）；預填即重現該體驗 | ADR-016 |

## M3 開工順序（依風險排序）

1. **Spike：metadb_hint_list 真機驗證**（ADR-016 的風險點）——
   最小程式碼：hardcode 一個 http URL + hint，確認 playlist 顯示
   預填欄位。失敗則觸發 ADR-016 fallback 重新決策。
2. Component 骨架：`foo_dms_browser` target（`component_macos/`）、
   ComponentEntry、mainmenu 命令開空視窗。
3. Preferences page（programmatic）：server 清單 CRUD + ServerListStore。
4. Browser 視窗：NSOutlineView tree + 背景 browse + 翻頁迴圈 +
   節點錯誤列。
5. Playlist 整合：雙擊/右鍵加入 + metadb hint 預填。
6. 手動測試清單文件 + mock server 場景對照。

## 未決事項（不阻塞 M3）

- ui_element_mac 可行性（M4 開頭驗證，ADR-014）。
- 遞迴加入的進度/取消 UI（M4，ADR-016）。
- albumArtURI 的呈現（fb2k mac 的 album art API 尚未研究；掛在
  docs/14 open questions O3 之後）。
