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
- 遞迴走訪核心是平台中立 adapter，使用 Catch2 覆蓋 breadth-first 走訪、
  進度回報、取消、曲目/資料夾上限與 fetch error 傳遞。ObjC++ UI 只負責
  worker queue、狀態列與 playlist 寫入。
- 大型樹的走訪仍佔用同一條 serial worker queue；遞迴加入期間其他 browse
  工作會排隊，這是避免 `HttpClient` 並行共用的刻意取捨。

## Revision: M4 播放體驗打磨範圍（2026-07-04）

### Context

「播放體驗打磨」沒有更細的需求定義。候選包含 seek/range、加入後自動播放、
狀態列訊息、不可播放資源提示。自動播放或播放控制整合會改變使用者現有
playlist 語意；在沒有明確需求前，這類變更可能比它解決的問題更大。

### Decision

- M4 採保守範圍：不自動開始播放、不改變 active playlist 選取或 playback
  control，只改善使用者對「會播放什麼」與「為何沒有加入」的可見性。
- Browser 選取 item 時顯示 `selectBestResource` 實際會交給 fb2k 的 resource
  摘要：MIME type、DIDL duration、sample rate、bit depth、channel count。
- `addToActivePlaylist` 回傳加入與跳過計數；UI 狀態列顯示加入幾首，並在有
  item 因沒有可播放 HTTP resource 而略過時明確提示。
- seek/range 不在 component 內另行實作。實際播放與 seek 仍由 fb2k 的 HTTP
  input stack 處理；驗收以真實 server 播放中 seek 是否正常為準。

### Consequences

- 播放體驗改善集中在診斷與預期管理，不引入自動播放造成的干擾。
- 若之後使用者明確希望「加入後自動播放」或更深 playback control，需要新
  revision 定義觸發條件、playlist 選取語意與手動驗收案例。

## Revision: playlist album art fallback（2026-07-05）

### Context

M4 真機驗證發現：browser 內能顯示 album art（view controller 自行下載
`albumArtURI`），但加入 playlist 後封面是空的。fb2k 的封面來源是曲目
「檔案本身」（內嵌圖、資料夾圖），遠端 HTTP 串流兩者皆無，而 hint 只帶
文字欄位，`albumArtURI` 在 hint mapping 層被丟棄。

### Decision

- hint mapping 額外寫入自訂欄位 `UPNP_ALBUM_ART_URI`（值為 DIDL
  `albumArtURI` 原字串），隨 `add_hint_browse` 進入 browse info。
  副作用：欄位會出現在 Properties 對話框，與 foo_upnp Windows 版做法一致。
- 註冊 `album_art_fallback` service（`DmsAlbumArtFallback.mm`）。fb2k
  在內嵌／外部來源都找不到圖時呼叫 fallback；實作從 handle 的
  browse info 讀出 URI，以 `HttpClient` 下載後回傳 front cover。
- 成功結果進 in-memory cache（上限 64 筆，滿了整批清空）；下載失敗不
  快取，保持可重試。只服務 `cover_front`。

### Consequences

- 已加入 playlist 的舊曲目沒有該欄位，重新加入才會有封面。
- fallback 只在 fb2k 查詢封面時觸發（Now Playing、playlist artwork
  column 等），瀏覽器內的預覽仍走原本的 NSURLSession 路徑，互不影響。
- `open()` 必須回傳有效 instance，不可回傳 null——fb2k core（2.25.8）
  對回傳值不做 null 檢查，回傳 null 會在使用者點選任何無封面曲目時
  SIGSEGV（真機 crash 驗證）。無 URI 時改由 `query()` 丟
  `exception_album_art_not_found`。
