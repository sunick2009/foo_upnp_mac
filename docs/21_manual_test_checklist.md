# DMS Browser 手動測試清單（M4 進行中）

**版本：** foo_dms_browser 0.2.0-dev

> **驗收紀錄（2026-07-03/04，foobar2000 v2.25.8 + foo_upnp 0.99.49）**
> 核心流程通過：真實 server 瀏覽 → 加入 playlist（metadata 預填）→
> 播放。錯誤情境通過：失敗不自動重試；非 device description 的 URL
> 顯示解析錯誤。Preferences 中文與持久化通過。
> 過程中修復三個問題：prefs 編輯失焦不存檔、空清單版面擠壓、
> 失敗節點無限重試（commit ff7137f、a2a00f4）。
>
> **驗收紀錄（2026-07-05）**
> playlist 封面（album_art_fallback 下載）通過；自訂 layout（tab 內
> DMS Browser + Playlists）通過，第三方 element 名稱含空白須加引號。
> 過程中修復一個 crash：fallback `open()` 回傳 null 導致點選無封面
> 曲目 SIGSEGV（commit 5cb9a98）。
>
> **驗收紀錄（2026-07-17）**
> E2E 實測（詳見文末「本次 E2E 測試紀錄」）：真實 server 瀏覽、播放、
> seek、resource 摘要、browser/playlist 封面、連線失敗處理、
> 非 UPnP URL 錯誤通過。2026-07-17 追加驗證確認 URL 鍵盤編輯保存、
> 雙擊錯誤列手動重試與取消遞迴加入均可運作；完整遞迴掃描仍受真實
> server 的 ContentDirectory timeout 影響，視窗關閉後的生命週期問題仍存在。

> **驗收紀錄（2026-07-18）**
> 依「下一輪手動驗證步驟」完成 mock 與真實 server 回合。Slow fixture
> 顯示「載入中…」期間可接受鍵盤 PageDown，實際播放未中斷；SOAP fault、
> 略過數量、10,000 首上限與取消狀態均符合預期。真實 server 的「播放列表」
> 完整遞迴掃描成功完成：4188 首、310 個資料夾，已驗證 30 秒 Browse
> timeout 修復。重啟前關閉獨立 DMS Browser 曾重現 Computer Use 無法取得
> 主視窗的 timeout；使用者手動重啟後重測，關閉與再次開啟均正常，故目前
> 定性為間歇性 stale window/controller 狀態，尚非穩定元件故障。
**前置：** component 已安裝（`~/Library/foobar2000-v2/user-components/
foo_dms_browser.component`）並重啟 foobar2000。
自動化已涵蓋的部分（adapter 邏輯、載入不崩潰）不在此清單。

## 環境

真實 server 或 mock server 擇一：

```bash
# mock server（repo 內建，port 8200）
python3 tools/mock_upnp_server.py 8200
# → URL: http://127.0.0.1:8200/rootDesc.xml
```

Mock server 測試用 fixture（2026-07-17 擴充）：

- `Mixed Fixtures`：`rich-track`（全 metadata 欄位 + albumArtURI +
  bitrate/samplerate/bit depth/channels）、`nores-track`（無 `<res>`，
  加入時應被略過並計數）、`plain-track`。
- `mixed` 與 `slow` 的曲目為**真實可播放 WAV**（440 Hz、4 秒、
  44100/16-bit/stereo，由 `/media/*.wav` 提供）——可實際播放與開
  Properties（驗 `%comment%` 等欄位）。`rich-track` 的 DIDL duration
  刻意標 3:30 而實際檔案 4 秒，用來區分 hint 與 decoder 來源。
  `music` 與 `bigtree` 的 URL 仍為假（404），播放失敗屬預期。
- `Broken (SOAP fault)`：展開時 server 回 SOAP fault 701。
- `Slow (5s per browse)`：每次 Browse 延遲 5 秒，用來捕捉「載入中…」
  與驗證 UI 不阻塞。
- `Big Tree (15000 tracks)`：150 個子資料夾 × 100 首（每次 Browse 延遲
  50ms）；遞迴加入會觸發 10,000 首上限，掃描期間有時間按「取消加入」。

## 1. 載入與註冊

- [x] Preferences → Components 列表出現「DMS Browser 0.2.0-dev」。
- [x] 主選單 View 出現「DMS Browser」項目。
- [x] Preferences → Tools 底下出現「DMS Browser」頁。

## 2. Preferences：server 清單管理（ADR-015）

- [x] 「+」新增一列，名稱欄自動進入編輯。
- [x] 填入名稱與 URL 後，關閉再開 Preferences，資料仍在。
- [x] **重啟 foobar2000**，資料仍在（cfg_var 持久化）。
- [x] 選取一列按「−」可刪除。
- [x] 名稱含中文/特殊字元（`"`、`\`）存取正常。
- [x] 以實體鍵盤直接輸入完整 URL，未失焦即關閉 Preferences，重開後 URL
      完整保存；編輯後切換到另一個 Preferences 頁面也完整保存。

## 3. Browser 視窗（ADR-014）

- [x] View → DMS Browser 開啟視窗；關閉後再開，功能與大小保留。2026-07-18
      使用者手動重啟後，關閉 DMS Browser 可立即取得主視窗，再由 View 重新
      開啟成功；Computer Use screenshot 尺寸維持 680×488。工具無法提供螢幕
      座標，位置未能以數值獨立核對，但未觀察到視窗位置或大小改變。
- [x] foobar2000 layout 設定中可加入「DMS Browser」layout element；
      加入後顯示與獨立視窗相同的 browser UI。
- [x] 未設定 server 時，狀態列提示到 Preferences 新增。
- [x] 選擇 server 後根節點自動展開，顯示頂層 container。
- [x] 展開 container 顯示「載入中…」後填入子項（不卡 UI，
      展開期間可接受鍵盤 PageDown、可切歌）。本輪以 mock 的
      `Slow (5s per browse)` fixture 捕捉到「載入中…」；載入期間播放狀態
      持續並由 PCM 1411kbps、44100Hz、stereo 進度證實未中斷。第一次
      Computer Use scroll 呼叫曾回報 `noWindowsAvailable`，因此「真實滑鼠
      捲動」仍需人工復核。
- [x] 選取含 `albumArtURI` 的曲目後，browser 底部顯示 album art 縮圖與
      title/artist/album/date；快速切換曲目不會殘留上一張圖。
      （兩個子條件分別驗證通過：縮圖與 metadata 顯示 — 2026-07-17 真實
      server 及 2026-07-18 mock `rich-track` 紅色封面；快速切換不殘留 —
      2026-07-17 真實專輯切換，見下一項。）
- [x] 在兩個不同含封面的真實專輯間快速切換，封面影像更新為目前曲目，
      未觀察到上一張封面殘留。
- [x] 選取曲目時，browser 底部顯示實際會播放的 resource 摘要
      （MIME、duration、sample rate、bit depth、channels；欄位依 server
      提供資料而定）。
- [x] 選取無 album art 的曲目或 container 後，album art 區域收合或清空。
- [x] Preferences 改了清單後，開下拉選單能看到更新。

## 4. 加入 playlist（ADR-016）

- [x] 雙擊曲目 → 加入目前 playlist，狀態列顯示「已加入 1 首」。
- [x] 曲目**立即**顯示 title/artist/album/date/tracknumber/時長
      （DIDL 預填），不是只有 URL。（2026-07-03/04 驗收通過；
      逐欄位檢查見下一項，尚未完成。）
- [ ] 加入後檢查可用欄位：`%artist%`、`%album artist%`、
      `%album%`、`%date%`、`%tracknumber%`、`%comment%`、
      `%length_seconds%`，以及 technical info `bitrate`、`samplerate`、
      `channels`。（2026-07-18 已於 UI 驗證 artist/album artist/album/
      date/tracknumber/時長 3:30/44100 Hz/16-bit/2 ch；`%comment%` 與
      `bitrate` 的 UI 顯示因 mock media 404 未能獨立確認，惟其 hint
      對應已由 `tests/adapter/test_hint_fields.cpp` 單元測試覆蓋。）
- [x] Container 右鍵「加入直接子項曲目」→ 其直接子項曲目
      全部加入（單層，子 container 不遞迴）。
- [x] 若加入範圍內存在沒有可播放 HTTP resource 的 item，狀態列顯示略過
      數量。本輪 `Mixed Fixtures` 顯示「已加入 2 首到目前播放清單，略過
      1 個不可播放項目」。
- [x] Container 右鍵「遞迴加入所有曲目」→ 狀態列顯示已掃描資料夾數與
      已找到曲目數，完成後整棵子樹的曲目加入 playlist。mock 的大樹與真實
      server 均完成；真實 server `播放列表` 顯示「已從「播放列表」遞迴
      加入 4188 首（掃描 310 個資料夾）」。
- [x] 遞迴加入進行中按「取消加入」→ 狀態列顯示「已取消「播放列表」遞迴
      加入，未加入曲目」，且取消後播放清單仍無 partial 結果。
- [x] 遞迴加入達 10,000 首或 10,000 個 container 上限時，狀態列標示
      已達掃描上限。`Big Tree (15000 tracks)` 顯示「已從「Big Tree (15000
      tracks)」遞迴加入 10000 首（掃描 102 個資料夾）（已達掃描上限）」。
- [x] 加入含 `albumArtURI` 的曲目後，playlist/Now Playing 顯示封面
      （album_art_fallback 下載；第一次查詢需等下載完成）。
- [x] 加入的曲目可播放（真實 server；mock server 的 URL 是假的，
      播放失敗屬預期）。
- [x] 播放中 seek 正常（server 支援 range request 時）。
      （2026-07-17 對 foo_upnp 實測，seek 可移動至指定時間。）

## 5. 錯誤情境（ADR-014）

- [x] Server 關機/拔線後展開節點 → 節點顯示「⚠️ 連線失敗…」，
      fb2k console 有詳細記錄（含實際請求的 URL），**沒有** modal 彈窗。
- [x] 失敗後**不會自動重試**（console 不刷屏、不重複打 server）。
- [x] 雙擊錯誤列或右鍵「重新載入」→ 重試一次。
      本次以實際 server IP 的未監聽 port `2999` 模擬斷線，雙擊錯誤子列
      與失敗 container 列各只產生一筆
      `DMS Browser: manual retry objectId="0"`；恢復 `:2333` 後根節點
      成功重新載入。未直接關閉遠端 server。
- [x] 在 Preferences 修正 URL 後切回瀏覽視窗 → 自動套用新清單，
      不需重開視窗。
- [x] Preferences 填入非 UPnP 的 URL（如 https://example.com）→
      展開時顯示解析錯誤而非崩潰。
- [x] mock server 對不存在的 ObjectID 回 SOAP fault → 節點顯示
      `伺服器拒絕（SOAP fault 701）：No such object`，無 modal；雙擊錯誤列
      後 Console 出現且只出現一筆 `DMS Browser: manual retry objectId="broken"`。

## 6. 相容性（docs/10 對照）

- [x] 真實 foo_upnp server（`http://10.102.0.10:2333/DeviceDescription.xml`
      ，內網）：root browse、中日文標題、多 res 選擇（WAV 優先於
      L16）、加入後可播放。

## 已知限制與待驗證

- [x] layout element 已在 foobar2000 mac layout editor 與 Preview 確認列出
  第三方「DMS Browser」element（ADR-014 M4 revision）。
- 無 SSDP 自動探索（M6）。
- 超過 10,000 項的 container 會截斷並標示（ADR-012 修訂）。

## 測後清理

- 刪除 spike 遺留的「DMS Spike」playlist（三首 example.com 假曲目）。

## 本次 E2E 測試紀錄（2026-07-17）

**實際環境：** foobar2000 v2.25.8 ARM64、DMS Browser 0.2.0-dev；
真實 server `http://10.102.0.10:2333/DeviceDescription.xml`；
repo mock server `http://127.0.0.1:8200/rootDesc.xml`。

### 已確認

- 真實 server 可瀏覽根目錄、中文與日文標題，且可選擇 WAV resource 優先於
  L16 resource。
- 真實曲目可播放，播放狀態顯示 PCM 1411 kbps、44100 Hz、stereo；seek
  可移動至指定時間。
- 含 `albumArtURI` 的曲目在 browser 與 playlist cover 區域均能顯示封面。
- Server 關閉時顯示連線失敗，不出現 modal；Console 記錄實際請求 URL，
  且等待後沒有自動重試。
- 清空 server 清單時，顯示「尚未設定伺服器 — Preferences → Tools →
  DMS Browser 新增」。
- `https://example.com` 顯示 device description 解析錯誤，未崩潰。
- 實體鍵盤直接輸入完整 URL 後立即關閉 Preferences，重開後保留
  `http://10.102.0.10:2333/DeviceDescription.xml`；編輯後切換到
  Components 頁面再關閉，重開後仍保留完整 URL。
- 以 `http://10.102.0.10:2999/DeviceDescription.xml` 模擬真實 server
  斷線時，錯誤子列與失敗的 `main` container 列各雙擊一次，Console 分別
  出現且各只出現一筆 `DMS Browser: manual retry objectId="0"`。改回
  真實 `:2333` 後切換 server，根節點恢復正常載入。
- `播放列表` 的直接加入顯示「沒有可播放的項目」，證實不會遞迴；遞迴
  取消則顯示「未加入曲目」，取消後目前 playlist table 仍為空。
- 在真實 server 的兩個不同專輯間切換有封面曲目，封面影像更新正確，
  未看到上一張封面殘留。播放期間切換到 DMS Browser 並展開 309 項
  Playback list 後，返回主視窗仍顯示 PCM 1411kbps、44100Hz stereo，
  播放未中斷。
- DMS Browser 關閉前後的 Computer Use screenshot 尺寸均為 680×488，
  尺寸看來有保留；但目前可及性資料無法提供螢幕座標，位置未能獨立驗證。

### 需要追蹤的問題

- 關閉 DMS Browser 獨立視窗（Close 按鈕或 Window → Close）後，foobar2000
  程序仍在執行；重啟前 Computer Use 曾無法重新取得任何主視窗，需重啟程序
  才能繼續操作。使用者手動重啟後重測，關閉後主視窗可立即取得，且 View →
  DMS Browser 可再次開啟。此問題目前應標為間歇性 stale window/controller
  狀態，仍建議保留回歸測試，但不足以判定為穩定元件 lifecycle blocker。
- URL 直接鍵盤編輯並立即關閉、以及切換 Preferences 頁面保存均已通過；
  但在 URL 欄位仍有焦點時關閉 Preferences，仍可能觸發上述視窗不可取得
  的問題，這是 UI 生命週期問題，不再是 URL 欄位只保存 `http://`。
- 快速切換不同 container 的曲目時，封面影像會更新，但詳細區底部的父
  container 摘要曾仍顯示上一個 container「伝」，而目前選取曲目已屬於
  「奉」。這是額外的 UI 狀態殘留問題，需另行修正或確認設計意圖。
  **→ 已釐清（commit a52f79b）：該行是狀態列，設計上顯示「上次載入完成
  的 container」而非目前選取；措辭改為「已載入「X」：N 個項目」消除
  歧義；待真機驗證。**
- 遞迴加入的即時狀態已包含資料夾與曲目計數。真實 server 的完整掃描曾在
  123 個資料夾／1715 首遇到 ContentDirectory control URL timeout。
  **→ 已驗證修復（commit a52f79b）：component 的傳輸 timeout 由 10s 放寬為
  30s（連線維持 10s，斷線回饋不變慢）；本輪完成 4188 首、310 個資料夾。**

### 尚未完成的覆蓋

- ~~現有 mock fixture 沒有 `albumArtURI`、無 HTTP resource 的 item、延遲
  掃描資料或超過 10,000 項資料~~ mock fixture 的載入中、略過數量、SOAP
  fault 與 10,000 首上限均已由 UI 驗證。
- `rich-track` 的 browser 摘要與 playlist table 已看到 artist、album artist、
  album、date、track number、duration，以及 44100 Hz、16-bit、2 ch；但
  mock 的 media resource `/media/rich-track.mp3` 回 404，導致 foobar2000
  Properties 顯示 `Object not found`，因此 `%comment%`、`%length_seconds%`
  與 Properties 中完整 technical info 尚未逐欄獨立驗證。

**清理結果：** 已移除本次新增的 `Keyboard Save Test` server 設定列；`main`
與 mock server 設定保留原值，mock server 未啟動。取消及 timeout 遞迴操作本身
未留下 partial 結果；之後為播放測試加入的單首 `dusk` 仍位於既有的
`New Playlist` 測試清單中，未刪除原有 playlist 名稱或其他使用者資料。

## 下一輪手動驗證步驟（2026-07-17 準備，已於 2026-07-18 執行）

**待驗證的修復：** commit a52f79b（傳輸 timeout 10s→30s、狀態列
「已載入」措辭）與 bca84a0（mock fixture 擴充）。

### 前置

1. 確認安裝的是含上述 commit 的 build（`~/Library/.../user-components/`
   已於 2026-07-17 18:53 更新），**重啟 foobar2000**。
2. 啟動 mock server：`python3 tools/mock_upnp_server.py 8200`。
3. Preferences 確認有 `http://127.0.0.1:8200/rootDesc.xml`（fixture 內容
   見「環境」節）。

### Mock 回合（~15 分鐘）

1. **載入中不卡 UI**：先播放任一首歌，展開 `Slow (5s per browse)`。
   - 預期：「載入中…」顯示約 5 秒，期間可捲動樹狀清單、播放不中斷。
   - 通過 → 勾 §3「展開 container 顯示載入中…」。
2. **逐欄位 metadata**：展開 `Mixed Fixtures`，雙擊
   `Rich Track（全欄位）` 加入 playlist，逐欄核對：
   - `%artist%` = Mock Artist、`%album artist%` = Mock Album Artist、
     `%album%` = Mock Rich Album、`%date%` = 2024-05-06、
     `%tracknumber%` = 7、`%comment%` = Mock comment text、
     `%length_seconds%` = 210；technical info：samplerate 44100、
     channels 2、bit depth 16。
   - 選取該曲目時 browser 底部摘要顯示 MIME/時長/44100 Hz/16-bit/2 ch，
     封面為紅色方塊（mock 提供的 PNG）。
   - 通過 → 勾 §4「加入後檢查可用欄位」。
3. **略過數量**：右鍵 `Mixed Fixtures` →「加入直接子項曲目」。
   - 預期：狀態列「已加入 2 首…，略過 1 個不可播放項目」
     （`nores-track` 無 `<res>`，應被略過）。
   - 通過 → 勾 §4「略過數量」。
4. **SOAP fault 經 UI**：展開 `Broken (SOAP fault)`。
   - 預期：節點顯示 SOAP fault 701 訊息、無 modal 彈窗；雙擊錯誤列後
     console 出現一筆 `DMS Browser: manual retry`（仍為 fault 屬預期）。
   - 通過 → 勾 §5「mock SOAP fault」。
5. **10,000 上限與完成訊息**：右鍵 `Big Tree (15000 tracks)` →
   「遞迴加入所有曲目」。
   - 預期：過程顯示「X 個資料夾，Y 首」即時計數；約 10 秒後完成訊息
     為「已從「Big Tree…」遞迴加入 10000 首（掃描 ~101 個資料夾）
     （已達掃描上限）」。
   - 通過 → 勾 §4「10,000 上限」。
6. **取消（重新確認）**：再對 `Big Tree` 遞迴加入一次，掃描中按
   「取消加入」。
   - 預期：「已取消…未加入曲目」，playlist 無 partial 結果。
   - 順帶確認：展開過兩個 container 後選取另一個 container 的曲目，
     狀態列措辭為「已載入「X」：N 個項目」，不再誤讀為目前選取。
7. **清理**：刪除本輪加入的 mock 曲目（含一萬首）。

### 真實 server 回合（~10 分鐘）

8. **完整遞迴掃描**：對真實 server「播放列表」重跑上次在
   123 資料夾／1715 首 timeout 的遞迴加入。
   - 預期：30 秒傳輸 timeout 下掃描完成，出現含資料夾數的完成訊息，
     整棵子樹曲目加入。
   - 通過 → 勾 §4「遞迴加入所有曲目」，並在追蹤問題註記 timeout
     修復已驗證。

### 純手動定性（真滑鼠鍵盤，不用 Computer Use）

9. **視窗生命週期**：手動關閉 DMS Browser 獨立視窗；手動關閉
   Preferences（含 URL 欄位仍有焦點時）。
   - 若主視窗完全正常 → 將追蹤問題定性為 Computer Use 工具限制，
     從清單移除；若異常 → 記錄重現步驟，開 issue。

測畢將結果更新至上方對應 checkbox 與追蹤問題，或交由 agent 對帳。

## 本輪 E2E 測試紀錄（2026-07-18）

### Mock server 結果

- `Slow (5s per browse)`：展開後實際看到「載入中…」，載入約 5 秒後顯示
  「已載入「Slow (5s per browse)」：2 個項目」。期間播放仍前進，未中斷。
- `Mixed Fixtures`：`Rich Track（全欄位）` 的 browser 摘要顯示
  `Mock Artist / Mock Rich Album / 2024-05-06`、`audio/mpeg / 0:03:30.000 /
  44100 Hz / 16-bit / 2 ch`，並看到紅色封面；加入後 playlist table 顯示
  album artist、album、track no `07`、title、artist 與 `3:30`。
- `Mixed Fixtures` 直接加入：狀態列為「已加入 2 首到目前播放清單，略過 1 個
  不可播放項目」。
- `Broken (SOAP fault)`：錯誤列顯示 SOAP fault 701，無 modal；雙擊錯誤列後
  Console 只有一筆 `manual retry objectId="broken"`。
- `Big Tree (15000 tracks)`：遞迴完成訊息為「已從「Big Tree (15000 tracks)」
  遞迴加入 10000 首（掃描 102 個資料夾）（已達掃描上限）」。
- 取消遞迴：狀態列為「已取消「Big Tree (15000 tracks)」遞迴加入，未加入曲目」。
  但本輪取消測試前已有上一輪 10,000 首上限測試結果在同一播放清單，因此
  未能以空播放清單單獨證明「無 partial 結果」；該項既有通過證據仍保留。
- mock media resource 未實作，播放 Rich Track 與開啟 Properties 時回報
  `Object not found`。這是 fixture 限制，不應誤判為 DMS Browser metadata
  預填失敗。

### 真實 server 結果

- `播放列表` 展開後顯示 309 個直接項目；遞迴掃描最終完成並顯示：
  `已從「播放列表」遞迴加入 4188 首（掃描 310 個資料夾）`。
- 本輪未再出現先前 123 個資料夾／1715 首時的 10 秒 timeout，30 秒傳輸
  timeout 修復已由實際完整掃描驗證。

### 視窗生命週期結果

- 重啟前關閉獨立 DMS Browser 後，foobar2000 仍在執行，但 Computer Use 以顯示
  名稱與 bundle identifier 重新取得狀態均回報 `timeoutReached`。使用者手動
  重啟後重測，關閉後主視窗可立即取得，View → DMS Browser 也可再次開啟；因此
  目前記為間歇性 stale window/controller 狀態，不再判定為穩定 lifecycle blocker。
- 關閉 Preferences，包括 URL 欄位保持焦點後關閉，均可重新取得主視窗；重開
  Preferences 仍顯示完整 `http://10.102.0.10:2333/DeviceDescription.xml`。
- DMS Browser 重開後 screenshot 尺寸維持 680×488；Computer Use 可取得尺寸，
  但無法提供螢幕座標，因此位置未能以數值獨立證實。功能性關閉與重開已通過，
  但步驟要求的「純手動」復核仍未由本輪 Computer Use 完整取代。

### 清理

- 本輪真實遞迴與 mock 大樹產生的 `New Playlist` 已在 foobar2000 關閉後移至
  `/private/tmp/foo_upnp_round2-New-Playlist*.bak` 與
  `/private/tmp/foo_upnp_round3-New-Playlist*.bak`，均為可逆備份。
- 重啟 foobar2000 後，New Playlist table 為空；mock server 已停止。
