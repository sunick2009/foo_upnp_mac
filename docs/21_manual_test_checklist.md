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

- [x] View → DMS Browser 開啟視窗；關閉後再開，位置大小有記住。
- [x] foobar2000 layout 設定中可加入「DMS Browser」layout element；
      加入後顯示與獨立視窗相同的 browser UI。
- [x] 未設定 server 時，狀態列提示到 Preferences 新增。
- [x] 選擇 server 後根節點自動展開，顯示頂層 container。
- [ ] 展開 container 顯示「載入中…」後填入子項（不卡 UI，
      展開期間可捲動、可切歌）。
- [x] 選取含 `albumArtURI` 的曲目後，browser 底部顯示 album art 縮圖與
      title/artist/album/date；快速切換曲目不會殘留上一張圖。
- [ ] 選取曲目時，browser 底部顯示實際會播放的 resource 摘要
      （MIME、duration、sample rate、bit depth、channels；欄位依 server
      提供資料而定）。
- [ ] 選取無 album art 的曲目或 container 後，album art 區域收合或清空。
- [ ] Preferences 改了清單後，開下拉選單能看到更新。

## 4. 加入 playlist（ADR-016）

- [ ] 雙擊曲目 → 加入目前 playlist，狀態列顯示「已加入 1 首」。
- [ ] 曲目**立即**顯示 title/artist/album/date/tracknumber/時長
      （DIDL 預填），不是只有 URL。
- [ ] 加入後檢查可用欄位：`%artist%`、`%album artist%`、
      `%album%`、`%date%`、`%tracknumber%`、`%comment%`、
      `%length_seconds%`，以及 technical info `bitrate`、`samplerate`、
      `channels`。
- [ ] Container 右鍵「加入直接子項曲目」→ 其直接子項曲目
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
- [ ] 加入的曲目可播放（真實 server；mock server 的 URL 是假的，
      播放失敗屬預期）。
- [ ] 播放中 seek 正常（server 支援 range request 時）。

## 5. 錯誤情境（ADR-014）

- [ ] Server 關機/拔線後展開節點 → 節點顯示「⚠️ 連線失敗…」，
      fb2k console 有詳細記錄（含實際請求的 URL），**沒有** modal 彈窗。
- [ ] 失敗後**不會自動重試**（console 不刷屏、不重複打 server）。
- [ ] 雙擊錯誤列或右鍵「重新載入」→ 重試一次。
- [ ] 在 Preferences 修正 URL 後切回瀏覽視窗 → 自動套用新清單，
      不需重開視窗。
- [ ] Preferences 填入非 UPnP 的 URL（如 https://example.com）→
      展開時顯示解析錯誤而非崩潰。
- [ ] mock server 對不存在的 ObjectID 回 SOAP fault →
      節點顯示 fault 訊息（可用右鍵「重新載入」驗證）。

## 6. 相容性（docs/10 對照）

- [ ] 真實 foo_upnp server（`http://10.102.0.10:2333/DeviceDescription.xml`
      ，內網）：root browse、中日文標題、多 res 選擇（WAV 優先於
      L16）、加入後可播放。

## 已知限制與待驗證

- layout element 已可編譯註冊，但仍需真機確認 foobar2000 mac layout UI
  是否列出第三方 element（ADR-014 M4 revision）。
- 無 SSDP 自動探索（M6）。
- 超過 10,000 項的 container 會截斷並標示（ADR-012 修訂）。

## 測後清理

- 刪除 spike 遺留的「DMS Spike」playlist（三首 example.com 假曲目）。
