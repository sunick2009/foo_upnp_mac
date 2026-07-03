# DMS Browser 手動測試清單（M3 MVP）

**版本：** foo_dms_browser 0.1.0

> **驗收紀錄（2026-07-03/04，foobar2000 v2.25.8 + foo_upnp 0.99.49）**
> 核心流程通過：真實 server 瀏覽 → 加入 playlist（metadata 預填）→
> 播放。錯誤情境通過：失敗不自動重試；非 device description 的 URL
> 顯示解析錯誤。Preferences 中文與持久化通過。
> 過程中修復三個問題：prefs 編輯失焦不存檔、空清單版面擠壓、
> 失敗節點無限重試（commit ff7137f、a2a00f4）。
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

- [ ] Preferences → Components 列表出現「DMS Browser 0.1.0」。
- [ ] 主選單 View 出現「DMS Browser」項目。
- [ ] Preferences → Tools 底下出現「DMS Browser」頁。

## 2. Preferences：server 清單管理（ADR-015）

- [ ] 「+」新增一列，名稱欄自動進入編輯。
- [ ] 填入名稱與 URL 後，關閉再開 Preferences，資料仍在。
- [ ] **重啟 foobar2000**，資料仍在（cfg_var 持久化）。
- [ ] 選取一列按「−」可刪除。
- [ ] 名稱含中文/特殊字元（`"`、`\`）存取正常。

## 3. Browser 視窗（ADR-014）

- [ ] View → DMS Browser 開啟視窗；關閉後再開，位置大小有記住。
- [ ] 未設定 server 時，狀態列提示到 Preferences 新增。
- [ ] 選擇 server 後根節點自動展開，顯示頂層 container。
- [ ] 展開 container 顯示「載入中…」後填入子項（不卡 UI，
      展開期間可捲動、可切歌）。
- [ ] Preferences 改了清單後，開下拉選單能看到更新。

## 4. 加入 playlist（ADR-016）

- [ ] 雙擊曲目 → 加入目前 playlist，狀態列顯示「已加入 1 首」。
- [ ] 曲目**立即**顯示 title/artist/album/時長（DIDL 預填），
      不是只有 URL。
- [ ] Container 右鍵「將曲目加入到目前播放清單」→ 其直接子項曲目
      全部加入（單層，子 container 不遞迴）。
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

## 已知限制（v1 設計如此）

- Container 加入為單層，不遞迴（M4，ADR-016）。
- 視窗不可停靠為 layout element（M4，ADR-014）。
- 無 SSDP 自動探索（M6）。
- 超過 10,000 項的 container 會截斷並標示（ADR-012 修訂）。

## 測後清理

- 刪除 spike 遺留的「DMS Spike」playlist（三首 example.com 假曲目）。
