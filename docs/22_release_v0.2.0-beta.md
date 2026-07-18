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

格式與生態一致性（2026-07-18 查證）：macOS 第三方元件（如
[JendaT mac suite](https://github.com/JendaT/fb2k-components-mac-suite)、
[官方 mac components repo](https://www.foobar2000.org/components/system/mac)）
均以 `.fb2k-component`（zip）發佈，安裝方式為雙擊 / Preferences →
Components → Install… / 手動放入 `user-components/`，系統需求同為
macOS 11+——與本專案的格式決定一致。最終仍以 clean-install 實測為準。

**Install… 實測與修正（2026-07-18）**：初版打包（bundle 直接在 zip
根目錄）被 foobar2000 Install… 拒絕：`Component installation failure:
Unsupported format or corrupted file`。逆向可安裝的第三方 mac 元件
（foo_jl_simplaylist 1.5.1）後確認正確 layout 為 **zip 根目錄下一層
`mac/` 目錄**（`mac/<name>.component/…`）。CI 打包已改為此 layout。
**修正版重測（2026-07-18）通過**：Install… 接受套件並解壓安裝至
`user-components/foo_dms_browser/foo_dms_browser.component`
（fb2k mac 的 Install… 慣例：以元件名建立子目錄）。
**注意**：Install… 與手動放入 `user-components/` 的複本會同時載入並
造成 component name clash——兩種安裝方式擇一，不可並存。

## 安裝

1. 下載 `.fb2k-component` 與 `.sha256`，`shasum -a 256 -c <檔名>.sha256`。
2. foobar2000 → Preferences → Components → Install… 選取檔案，重啟
   （已實測可用；安裝到 `user-components/foo_dms_browser/`）。
3. 替代（擇一，勿與 Install… 並存）：手動解壓
   `ditto -x -k <檔名>.fb2k-component /tmp/pkg` 後將
   `/tmp/pkg/mac/foo_dms_browser.component` 放入
   `~/Library/foobar2000-v2/user-components/`，重啟。

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

見 `docs/10_compatibility_plan.md`。已驗證：foo_upnp 0.99.49 與
MiniDLNA 1.3.3（browse/playback/seek/metadata/album art/pagination，
CLI + 元件級）。Jellyfin/Plex 尚為 unknown。

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
- [x] **0.2.0 release artifact** 驗證（2026-07-18，run 29639328169，
      commit 772cab6）：SHA-256
      `4a35d492fa93eef6cdae721249f9b7b1ac53b8a4ca7d392f32b0514d94c70a8d`
      `foo_dms_browser-0.2.0-arm64.fb2k-component`；`mac/` layout、
      嚴格簽章、入口點、binary 內版號 `0.2.0` 均通過，並用於
      clean-profile 首跑與 GitHub Release。
- [x] 乾淨環境 clean-install 測試（不使用本機 build 目錄）：
      （2026-07-18 clean-profile 首跑：備份原 profile 後以全新 profile
      啟動，Install… 安裝 run 29639328169 的
      `foo_dms_browser-0.2.0-arm64.fb2k-component` 後重啟——見下列子項。）
  - [x] 手動路徑已驗證（2026-07-18，開發機）：下載 run 29612715910 的
        artifact → `shasum -c` 通過 → `ditto -x -k` 解壓到
        `user-components/` → 解壓後 bundle 通過
        `codesign --verify --deep --strict` 與 arm64 檢查。
  - [x] Preferences → Components → Install… 接受 `.fb2k-component`
        （2026-07-18，修正 `mac/` layout 後通過；安裝至
        `user-components/foo_dms_browser/`。注意勿與手動複本並存，
        會 component name clash）。
  - [x] Components 列表顯示 DMS Browser 0.2.0（2026-07-18 首跑）。
  - [x] Preferences 頁、browse、加入、播放、封面正常
        （2026-07-18 首跑，對真實 foo_upnp server 驗證）。
  - [x] layout element 於 0.2.0 artifact 安裝下正常
        （2026-07-18 補測：release artifact 換裝至日常 profile 後，
        Components 顯示 0.2.0，layout 中的 DMS Browser element 正常
        顯示；同碼 build 先前已於 2026-07-17/18 驗證）。
- [x] 發佈 GitHub Release（v0.2.0 pre-release）：附上套件、`.sha256`、
      changelog（run 29639328169 artifact，commit 772cab6）。
- [x] 發佈前把 `ComponentEntry.mm` 版本 `0.2.0-dev` → `0.2.0`
      （2026-07-18，issue #12）。
