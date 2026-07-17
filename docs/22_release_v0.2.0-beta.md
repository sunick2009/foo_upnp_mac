# v0.2.0 beta 發佈定義與驗證（issue #8）

## 發佈物

| 項目 | 決定 |
|---|---|
| 格式 | `foo_dms_browser-<版本>-arm64.fb2k-component`（`.component` bundle 的 zip 封裝，`ditto -c -k --keepParent` 產生） |
| 校驗 | 同名 `.sha256`（`shasum -a 256`），隨 artifact 一起上傳 |
| 架構 | arm64（Apple Silicon）；Intel 使用者從原始碼編譯 |
| 最低系統 | macOS 11.0（`CMAKE_OSX_DEPLOYMENT_TARGET`，設定於根 CMakeLists.txt） |
| 簽章 | ad-hoc（`codesign -s -`）；無 Developer ID / notarization（beta 已知限制） |
| 產生方式 | GitHub Actions `ci.yml` macOS job（Release build）每次 main push 產生；發佈時取綠色 run 的 artifact 附加到 GitHub Release |

CI 對每個 artifact 執行煙霧測試：bundle 結構、`Info.plist` lint、
arm64 架構、`_foobar2000_get_interface` 匯出、
`codesign --verify --deep --strict`。

## 安裝

1. 下載 `.fb2k-component` 與 `.sha256`，`shasum -a 256 -c <檔名>.sha256`。
2. foobar2000 → Preferences → Components → Install… 選取檔案，重啟。
3. 若 Install… 不接受（待 clean-install 驗證），手動解壓：
   `ditto -x -k <檔名>.fb2k-component ~/Library/foobar2000-v2/user-components/`
   後重啟。

## Changelog（0.1.0 → 0.2.0）

- 「DMS Browser」layout element（`ui_element_mac`），可放入自訂 layout。
- Browser 底部 metadata / resource 摘要（MIME、時長、sample rate、
  bit depth、channels）與 album art 預覽。
- Container「加入直接子項曲目」與「遞迴加入所有曲目」：即時進度
  （資料夾/曲目計數）、取消、10,000 首/資料夾掃描上限、
  不可播放項目略過計數。
- playlist / Now Playing 封面（`album_art_fallback` 下載）。
- 錯誤處理強化：手動重試（雙擊錯誤列或失敗列，console 記錄）、
  SOAP fault 訊息顯示、傳輸 timeout 30s（連線 10s）。
- 修復：prefs 編輯中關窗遺失輸入、無封面曲目 SIGSEGV、
  失敗節點無限重試。

## 已知限制

- 無 SSDP 自動探索（M6）；需手動輸入 device description URL。
- ad-hoc 簽章：Gatekeeper 可能要求首次允許（beta 接受）。
- 超過 10,000 項的 container 截斷並標示。
- 間歇性：關閉獨立 browser 視窗後曾觀察到 stale window/controller
  狀態（docs/21 追蹤中，非穩定重現）。

## 相容性

見 `docs/10_compatibility_plan.md`。已驗證：foo_upnp 0.99.49
（browse/playback/seek/metadata/album art/pagination）。
MiniDLNA/Jellyfin/Plex 尚為 unknown（issue #6）。

## 驗證紀錄

- [x] CI run（Release build + smoke check + 打包）通過：
      <https://github.com/sunick2009/foo_upnp_mac/actions/runs/29612715910>
      （commit 72545ea，2026-07-18；Linux 測試 + macOS build/test/
      codesign/smoke check/打包全綠）。
- [x] artifact SHA-256 與 `.sha256` 檔一致（2026-07-18 下載驗證：
      `c3b929176c3f4d172a105940b87a54c7f527f3065097faa333ce1c151bfdc9ff`
      `foo_dms_browser-0.2.0-dev-arm64.fb2k-component`）；解壓後
      bundle 結構、arm64、`_foobar2000_get_interface`、
      `codesign --verify --deep --strict` 均通過。
- [ ] 乾淨環境 clean-install 測試（不使用本機 build 目錄）：
  - [ ] Preferences → Components → Install… 接受 `.fb2k-component`
        （或記錄不支援並改用手動路徑）。
  - [ ] Components 列表顯示 DMS Browser 0.2.0。
  - [ ] Preferences 頁、layout element、browse、加入、播放、封面正常。
- [ ] 發佈 GitHub Release：附上套件、`.sha256`、本文件 changelog 節。
- [ ] 發佈前把 `ComponentEntry.mm` 版本 `0.2.0-dev` → `0.2.0`。
