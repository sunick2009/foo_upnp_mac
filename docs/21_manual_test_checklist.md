# DMS Browser 手動測試清單

**版本：** foo_dms_browser 0.2.0（v0.2.0 beta 已發佈，見 `docs/22`）

本檔只保留 **checklist 與最終結論**。逐輪測試的原始記錄（含當時的
中間結論）歸檔於 `docs/e2e_records/`，以本檔為準。

## 驗收摘要

| 日期 | 範圍與結果 | 紀錄 |
|---|---|---|
| 2026-07-03/04 | M3 核心流程（瀏覽→加入→播放）、錯誤情境、Preferences 持久化通過；修復 prefs 失焦存檔、空清單版面、失敗節點無限重試 | commit ff7137f、a2a00f4 |
| 2026-07-05 | playlist 封面（album_art_fallback）、自訂 layout 通過；修復無封面曲目 SIGSEGV | commit 5cb9a98 |
| 2026-07-17 | 真實 server E2E：seek、resource 摘要、封面、連線失敗處理通過；發現 4 個追蹤問題（後均修復） | [紀錄](e2e_records/2026-07-17.md) |
| 2026-07-18 | 三輪驗證：mock fixture 全套（載入中、略過、fault、上限、取消）、真實 server 完整遞迴掃描、MiniDLNA 元件級、clean-profile 首跑、五輪視窗回歸——issues #3–#12 全數收案 | [紀錄](e2e_records/2026-07-18.md) |

**前置：** component 已安裝（`~/Library/foobar2000-v2/user-components/
foo_dms_browser.component`，或經 Components → Install…，兩者擇一）
並重啟 foobar2000。自動化已涵蓋的部分（adapter 邏輯、載入不崩潰）
不在此清單。

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
  44100/16-bit/stereo，內嵌 RIFF INFO tag，由 `/media/*.wav` 提供）——
  可實際播放與開 Properties。`rich-track` 的 DIDL duration 刻意標 3:30
  而實際檔案 4 秒，用來區分 hint 與 decoder 來源。`music` 與 `bigtree`
  的 URL 仍為假（404），播放失敗屬預期。
- `Broken (SOAP fault)`：展開時 server 回 SOAP fault 701。
- `Slow (5s per browse)`：每次 Browse 延遲 5 秒，用來捕捉「載入中…」
  與驗證 UI 不阻塞。
- `Big Tree (15000 tracks)`：150 個子資料夾 × 100 首（每次 Browse 延遲
  50ms）；遞迴加入會觸發 10,000 首上限，掃描期間有時間按「取消加入」。

## 1. 載入與註冊

- [x] Preferences → Components 列表出現「DMS Browser 0.2.0」
      （2026-07-18 clean-profile 首跑）。
- [x] 主選單 View 出現「DMS Browser」項目。
- [x] Preferences → Tools 底下出現「DMS Browser」頁。

## 2. Preferences：server 清單管理（ADR-015）

- [x] 「+」新增一列，名稱欄自動進入編輯。
- [x] 填入名稱與 URL 後，關閉再開 Preferences，資料仍在。
- [x] **重啟 foobar2000**，資料仍在（cfg_var 持久化）。
- [x] 選取一列按「−」可刪除。
- [x] 名稱含中文/特殊字元（`"`、`\`）存取正常。
- [x] 以實體鍵盤直接輸入完整 URL，未失焦即關閉 Preferences，重開後 URL
      完整保存；編輯後切換到另一個 Preferences 頁面也完整保存
      （2026-07-18，commit 89b410c 的關窗 commit 修復）。

## 3. Browser 視窗（ADR-014）

- [x] View → DMS Browser 開啟視窗；關閉後再開，功能與大小保留
      （2026-07-18 五輪回歸；位置未以數值座標獨立核對）。
- [x] foobar2000 layout 設定中可加入「DMS Browser」layout element；
      加入後顯示與獨立視窗相同的 browser UI。
- [x] 未設定 server 時，狀態列提示到 Preferences 新增。
- [x] 選擇 server 後根節點自動展開，顯示頂層 container。
- [x] 展開 container 顯示「載入中…」後填入子項，期間 UI 不阻塞、
      播放不中斷（2026-07-18，mock `Slow` fixture）。
- [x] 選取含 `albumArtURI` 的曲目後，browser 底部顯示 album art 縮圖與
      title/artist/album/date；快速切換曲目不會殘留上一張圖
      （2026-07-17 真實專輯切換 + 2026-07-18 mock `rich-track`）。
- [x] 選取曲目時，browser 底部顯示實際會播放的 resource 摘要
      （MIME、duration、sample rate、bit depth、channels；欄位依 server
      提供資料而定）。
- [x] 選取無 album art 的曲目或 container 後，album art 區域收合或清空。
- [x] Preferences 改了清單後，開下拉選單能看到更新。

## 4. 加入 playlist（ADR-016）

- [x] 雙擊曲目 → 加入目前 playlist，狀態列顯示「已加入 1 首」。
- [x] 曲目**立即**顯示 title/artist/album/date/tracknumber/時長
      （DIDL 預填），不是只有 URL。
- [x] 加入後檢查可用欄位：`%artist%`、`%album artist%`、`%album%`、
      `%date%`、`%tracknumber%`、`%comment%`、`%length_seconds%`，
      以及 technical info `bitrate`、`samplerate`、`channels`。
      （2026-07-18 逐欄驗證；`%comment%` 以 MP3+ID3 路徑驗證通過。
      已知限制：fb2k mac 不顯示遠端 WAV 的 RIFF INFO comment，
      非欄位映射缺陷——#9。）
- [x] Container 右鍵「加入直接子項曲目」→ 其直接子項曲目全部加入
      （單層，子 container 不遞迴）；**未展開過的 container 亦可**，
      會先隨選取回直接子項（#11 修復，2026-07-18 驗證）。
- [x] 加入範圍內有無可播放 HTTP resource 的 item 時，狀態列顯示略過
      數量（mock `Mixed Fixtures`：已加入 2 首、略過 1）。
- [x] Container 右鍵「遞迴加入所有曲目」→ 狀態列顯示已掃描資料夾數與
      已找到曲目數，完成後整棵子樹加入（真實 server：4188 首／
      310 個資料夾，驗證 30s 傳輸 timeout——a52f79b）。
- [x] 遞迴加入進行中按「取消加入」→ 顯示已取消，無 partial 結果。
- [x] 遞迴加入達 10,000 首或 10,000 個 container 上限時，狀態列標示
      已達掃描上限（mock `Big Tree`：10000 首／102 個資料夾）。
- [x] 加入含 `albumArtURI` 的曲目後，playlist/Now Playing 顯示封面
      （album_art_fallback 下載；第一次查詢需等下載完成）。
- [x] 加入的曲目可播放（真實 server 與 mock 的 WAV fixture；
      `music`/`bigtree` 的假 URL 播放失敗屬預期）。
- [x] 播放中 seek 正常（server 支援 range request 時；
      2026-07-17 foo_upnp、2026-07-18 MiniDLNA）。

## 5. 錯誤情境（ADR-014）

- [x] Server 關機/拔線後展開節點 → 節點顯示「⚠️ 連線失敗…」，
      fb2k console 有詳細記錄（含實際請求的 URL），**沒有** modal 彈窗。
- [x] 失敗後**不會自動重試**（console 不刷屏、不重複打 server）。
- [x] 雙擊錯誤列或失敗 container 列 → 恰好重試一次，console 記錄
      `DMS Browser: manual retry`（2026-07-18，以未監聽 port 模擬）。
- [x] 在 Preferences 修正 URL 後切回瀏覽視窗 → 自動套用新清單，
      不需重開視窗。
- [x] Preferences 填入非 UPnP 的 URL（如 https://example.com）→
      展開時顯示解析錯誤而非崩潰。
- [x] mock server 對不存在的 ObjectID 回 SOAP fault → 節點顯示
      `伺服器拒絕（SOAP fault 701）：No such object`，無 modal，
      雙擊重試恰好一次。

## 6. 相容性（docs/10 對照）

- [x] 真實 foo_upnp server（`http://10.102.0.10:2333/DeviceDescription.xml`
      ，內網）：root browse、中日文標題、多 res 選擇（WAV 優先於
      L16）、加入後可播放。
- [x] MiniDLNA 1.3.3（docker）：元件級 browse → 加入 → 播放狀態
      （MP3 與 FLAC 均進入解碼並可 seek）→ 封面通過；`%artist%` 顯示
      album artist 為 MiniDLNA quirk（詳見 docs/10）。#6 以此證據
      收案；喇叭實際出聲未以工具獨立記錄（自動化無法監聽音訊）。

## 已知限制

- 無 SSDP 自動探索（M6）；需手動輸入 device description URL。
- 超過 10,000 項的 container 會截斷並標示（ADR-012 修訂）。
- fb2k mac 不顯示遠端 WAV 的 RIFF INFO comment（#9 定性，
  hint 映射由 `tests/adapter/test_hint_fields.cpp` 覆蓋）。
- Install… 與手動放入 `user-components/` 的複本不可並存
  （component name clash，見 docs/22 安裝說明）。

## 視窗生命週期回歸測試（issue #10 定義；2026-07-18 五輪通過）

設計說明：獨立視窗為刻意的 singleton——建立一次、關閉僅 orderOut
（`releasedWhenClosed=NO`）、重開重用同一 controller，不會累積 stale
controller；layout 內的 ui_element 是獨立 instance，不受獨立視窗影響。
含診斷 log 的 build 起，每次建立/顯示/關閉都會寫 fb2k console
（`DMS Browser: standalone window created / shown / closed`）。

若懷疑再現，重複下列序列 **5 次**（乾淨啟動、單一元件複本）：

1. View → DMS Browser 開啟獨立視窗 → console 出現 `shown`
   （首次另有 `created`）。
2. 點紅色 Close 按鈕關閉 → console 出現 `closed (ordered out,
   controller retained)`。
3. 點主視窗任意處 → 主視窗可正常取得焦點、選單可操作。
4. View → DMS Browser 重開 → 視窗回復、位置大小保留、
   server/樹狀態仍在（同一 controller）。
5. 若 layout 中有 DMS Browser element，確認它全程不受影響。

任一輪主視窗失去回應：記錄該輪 console log（含生命週期訊息順序）與
操作並開 issue。

## 測後清理

- 測試加入的曲目/playlist 清除或備份；mock server 與 docker 容器停止。

## 歷史紀錄

- [2026-07-17 E2E 紀錄](e2e_records/2026-07-17.md)
- [2026-07-18 E2E 紀錄與指引](e2e_records/2026-07-18.md)
