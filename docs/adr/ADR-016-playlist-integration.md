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

## Revision: M4 遞迴加入與 metadata 完整化（2026-07-04）

### Context

真實 foo_upnp server 的 DIDL-Lite item 除了 M3 已使用的
`dc:title`、`upnp:artist`、`upnp:album`、`res@duration` 外，還提供
`dc:creator`、`dc:date`、`upnp:originalTrackNumber`、
`upnp:longDescription`，以及 `res` 的 `bitrate`、`bitsPerSample`、
`sampleFrequency`、`nrAudioChannels`。若 parser 或 hint adapter 丟掉
這些資料，playlist 欄位會落後於 Windows foo_upnp 的使用者預期。

M3 把 container 加入限制在「直接子項」；這對 Album container 足夠，
但對 Artist、Genre、Folder 等多層 container 不足。遞迴加入會觸發多次
blocking browse，因此必須沿用 ADR-017 的 worker queue，並提供進度與
取消。

### Decision

1. **DIDL model 與 hint mapping 擴充。**
   - `UpnpObject` 保留 creator、date、original track number、disc
     number、total discs、long description、album artist role 與完整
     artist 列表。
   - `UpnpResource` 保留 bitrate、bit depth、sample rate、channel
     count。
   - playlist browse-info hints 寫入 `title`、`artist`、`album artist`、
     `album`、`genre`、`date`、`tracknumber`、`discnumber`、
     `totaldiscs`、`comment`；若沒有 `upnp:artist` 則以
     `dc:creator` 作為 `artist` fallback。resource 的 technical info
     寫入 `bitrate`、`bitspersample`、`samplerate`、`channels`。
2. **Container 右鍵同時保留兩種加入語意。**
   - 「加入直接子項曲目」維持 M3 行為，只對已載入的直接子 item 生效。
   - 「遞迴加入所有曲目」從該 container 的 ObjectID 開始走訪整棵子樹，
     不要求 UI tree 事先展開。
3. **遞迴加入採先收集、後加入。**
   - 背景 worker 逐 container 呼叫 `fetchChildren`，狀態列回報已掃描
     container 數與已找到曲目數。
   - 使用者按「取消加入」時在 container/item 邊界停止，且不把已收集的
     partial 結果加入 playlist。這避免「取消後仍新增一批曲目」的模糊語意。
   - 上限為 10,000 首曲目與 10,000 個 container。達上限時加入已收集曲目，
     並在狀態列標示已達掃描上限。

### Consequences

- metadata 完整化已有 parser fixture、adapter tests、真實 server CLI JSON
  三層驗證；真機 playlist 欄位仍需手動確認，因 fb2k UI 無可用自動化。
- 遞迴加入的 cancellation 不是中斷進行中的 HTTP request；它在目前 request
  返回後停止後續走訪，符合 ADR-017 的同步 HTTP 模型。
- 大型樹的走訪仍佔用同一條 serial worker queue；遞迴加入期間其他 browse
  工作會排隊，這是避免 `HttpClient` 並行共用的刻意取捨。
