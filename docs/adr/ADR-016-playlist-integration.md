# ADR-016: Playlist 整合 — 加入語意與 metadata 預填

**狀態：** Accepted
**日期：** 2026-07-03（M3 grilling session）

## Context

foo_upnp（Windows）的體驗：雙擊曲目加入 playlist、container 可整個
（遞迴）加入、加入的曲目**立即**顯示 artist/title/album——那些資料
來自 server 的 DIDL-Lite 回應，不是音訊檔內的 tag。fb2k 對支援
range request 的 HTTP 位址也能逐檔讀取檔內 tag，但整批加入時又慢
又依賴 server 行為。

## Decision

1. **加入語意（v1 單層）：**
   - 雙擊 item → 加入現行 playlist。
   - Container 右鍵 →「加入直接子項曲目」（單層，不遞迴）。
   - 右鍵選單同時提供 item 的「加入」與未來擴充點。
   - 完整遞迴加入（整棵子樹）延到 M4：需要進度 UI、取消機制與
     深度/數量上限，v1 不揹。
2. **Metadata：`metadb_hint_list` 預填 DIDL 資料 + fb2k 自然更新。**
   加入時把 DIDL 的 title / artist / album / duration（以及可得的
   track number、genre）以 metadb hint 寫入，playlist 立即顯示完整
   欄位；之後 fb2k 播放時若從遠端讀到檔內 tag 會自然補充。
   這是 M3 的**第一個 spike**：`metadb_hint_list` 在 mac 的行為
   SDK 有宣告但未經我們驗證，先用最小程式碼真機確認再蓋 UI。

## Spike 驗證結果（2026-07-03）

真機（foobar2000 v2.25.8 macOS）三變體實測：

- `add_hint`（freshflag=false）：**被拒收**，讀回全部 fallback 值。
- `add_hint`（freshflag=true）：生效，但語意錯誤——宣稱資料讀自
  檔案本身，會佔據 primary info。
- `metadb_hint_list_v2::add_hint_browse`：**生效且語意正確**——
  browse info 正是 fb2k 給「外部來源 metadata」（如 m3u EXTINF）
  的通道，播放時讀到檔內真 tag 會自然覆蓋。

**production 採用 `add_hint_browse`。** title/artist/album/length
均以 `%title%|%artist%|%album%|%length_seconds%` 讀回驗證。

## Consequences

- DIDL `UpnpObject` → metadb hint 欄位的映射是純 C++ adapter
  函式，用 Catch2 測。
- 加入用的 URL 是 `selectBestResource`（ADR-007）選出的資源。
- 專輯資料夾（最常見情境）單層加入即可整張加入；多層結構
  （如 Artist → Album）使用者需逐層操作，v1 接受此限制。
- 若 spike 發現 metadb hint 在 mac 不可用，fallback 是只加
  URL（體驗降級），並記錄新 ADR 重新決策。
