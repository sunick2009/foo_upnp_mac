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
> 非 UPnP URL 錯誤通過。發現 4 個追蹤問題（雙擊錯誤列不重試、
> 遞迴加入未顯示已掃描資料夾數、URL 欄位關閉時只存 `http://`、
> 疑似視窗生命週期問題）。
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

## 3. Browser 視窗（ADR-014）

- [ ] View → DMS Browser 開啟視窗；關閉後再開，位置大小有記住。
- [x] foobar2000 layout 設定中可加入「DMS Browser」layout element；
      加入後顯示與獨立視窗相同的 browser UI。
- [x] 未設定 server 時，狀態列提示到 Preferences 新增。
- [x] 選擇 server 後根節點自動展開，顯示頂層 container。
- [ ] 展開 container 顯示「載入中…」後填入子項（不卡 UI，
      展開期間可捲動、可切歌）。
- [ ] 選取含 `albumArtURI` 的曲目後，browser 底部顯示 album art 縮圖與
      title/artist/album/date；快速切換曲目不會殘留上一張圖。
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
      `channels`。
- [x] Container 右鍵「加入直接子項曲目」→ 其直接子項曲目
      全部加入（單層，子 container 不遞迴）。
- [ ] 若加入範圍內存在沒有可播放 HTTP resource 的 item，狀態列顯示略過
      數量。
- [ ] Container 右鍵「遞迴加入所有曲目」→ 狀態列顯示已掃描資料夾數與
      已找到曲目數，完成後整棵子樹的曲目加入 playlist。
- [ ] 遞迴加入進行中按「取消加入」→ 狀態列顯示取消，且不加入 partial
      結果。
- [ ] 遞迴加入達 10,000 首或 10,000 個 container 上限時，狀態列標示
      已達掃描上限。
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
- [ ] 雙擊錯誤列或右鍵「重新載入」→ 重試一次。
      （2026-07-17：雙擊未觀察到重新請求；僅根節點右鍵「重新載入」
      可重試，見文末追蹤問題。）
- [x] 在 Preferences 修正 URL 後切回瀏覽視窗 → 自動套用新清單，
      不需重開視窗。
- [x] Preferences 填入非 UPnP 的 URL（如 https://example.com）→
      展開時顯示解析錯誤而非崩潰。
- [ ] mock server 對不存在的 ObjectID 回 SOAP fault →
      節點顯示 fault 訊息（可用右鍵「重新載入」驗證）。

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

### 需要追蹤的問題

- 關閉 Preferences 或 layout Preview 後，foobar2000 程序仍在執行，但曾
  出現主視窗無法被電腦控制介面重新取得的情況，需重啟程序才能繼續操作。
  尚未判定是元件視窗生命週期問題或控制工具限制。
- URL 欄位直接編輯後立即關閉，曾只保存 `http://`；先讓欄位失焦後可
  完整保存。需以實際鍵盤輸入補測關閉時的 commit 行為。
  **→ 已修（commit 89b410c）：viewWillDisappear 強制 commit 編輯中欄位；
  待真機驗證。**
- 錯誤列直接雙擊未觀察到明確重新請求，但右鍵根節點的「重新載入」可
  重新執行一次請求。
  **→ 已修（commit 89b410c）：雙擊失敗的 container 列本身也會重試，
  且手動重試會寫入 console；待真機驗證。**
- 遞迴加入結果顯示已加入曲目數，但未顯示清單要求的已掃描資料夾數。
  **→ 已修（commit 89b410c）：完成訊息加入已掃描資料夾數；待真機驗證。**

### 尚未完成的覆蓋

- 現有 mock fixture 沒有 `albumArtURI`、無 HTTP resource 的 item、延遲
  掃描資料或超過 10,000 項資料，因此略過數量、取消遞迴加入、掃描上限及
  albumArt metadata 的完整欄位組合尚未完整驗證。
- 現有 browser UI 沒有可直接建立不存在 ObjectID 的節點，因此 mock SOAP
  fault 尚未透過 UI 完成驗證。
- 未逐欄驗證 `%artist%`、`%album artist%`、`%date%`、`%tracknumber%`、
  `%comment%` 及所有 technical info 欄位。

**清理結果：** 已清除本次加入的測試曲目，恢復 `main` 與 mock server 設定，
並停止 mock server。
