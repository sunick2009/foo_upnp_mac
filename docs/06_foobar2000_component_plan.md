# foobar2000 macOS Component 規劃

## 導入時機

本文件描述的是 M3 的功能範圍與模組切分。
建置策略已由 `docs/adr/ADR-013-macos-build-without-xcode.md` 修正，
因此這裡提到的 `.xcodeproj` 應視為歷史規劃，不再是預設路線。

**2026-07-03 更新**：M3 設計已由 grilling session 定案
（`docs/20_m3_grilling_session.md`、ADR-014～017）。要點：UX 對齊
Windows 版 foo_upnp 但守 v1 約束；bundle 命名 `foo_dms_browser`、
顯示名「DMS Browser」；MVP 先做主選單開啟的獨立視窗（Browser
Panel 一節的功能不變，載體是 NSWindow），M4 再掛 `ui_element_mac`；
UI 全程式碼建構、零 xib。

不要在 Phase 0 一開始就導入 foobar2000 SDK 或 Xcode project。

應在以下條件成立後再導入：

- CLI 可以 browse 至少一台 UPnP server。
- core parser 有 fixture tests。
- 可以取得可播放 resource URL。
- metadata mapping 初步可用。

## component 功能

### Preferences Page

- 新增 server URL。
- 移除 server URL。
- 測試連線。
- 顯示解析結果。
- 設定 timeout。
- 儲存 server list。

### Browser Panel

- server selector。
- container list。
- item list。
- breadcrumb。
- back / forward。
- context menu。
- Add to playlist。
- Play now。

## 專案結構

```text
component_macos/
  CMakeLists.txt
  fb2k_sdk.cmake          # SDK static libs（ADR-013）
  smoke/
    Fb2kSmokeComponent.mm
  sdk_check/
    SdkCheckComponent.mm  # 拋棄式 SDK 驗證 target
  foo_dms_browser/
    ComponentEntry.mm     # DECLARE_COMPONENT_VERSION + initquit
    MainMenuCommands.mm   # View → DMS Browser 開窗命令
    BrowserWindow.mm      # NSWindow 殼（M4 換 ui_element_mac 掛載）
    BrowserViewController.mm  # NSOutlineView tree（可重用核心）
    BrowseTreeModel.{hpp,cpp} # 節點狀態機 + 翻頁迴圈（純 C++、可測）
    PreferencesPage.mm    # programmatic NSViewController
    ServerListStore.{hpp,cpp} # JSON ↔ vector<ServerEntry>（純 C++、可測）
    PlaylistIntegration.mm    # 加入 playlist + metadb hint 預填
    DidlToHint.{hpp,cpp}      # UpnpObject → hint 欄位映射（純 C++、可測）
```

純 C++ 檔案（BrowseTreeModel、ServerListStore、DidlToHint）掛進
既有 Catch2 測試；`.mm` 檔維持薄殼（grilling 決策 #11）。

## 整合點

| 區塊 | 說明 |
|---|---|
| component registration | foobar2000 SDK service registration |
| preferences | 儲存 server list |
| UI panel | 顯示 Browser |
| playlist | 將 URL 與 metadata 加入 playlist |
| playback | Play now |
| logging | debug output |

## 風險

- macOS foobar2000 component 生態較小。
- foobar2000 SDK 的編譯設定可能需要手動移植到 CMake。
- UI lifecycle 與 thread safety 需要特別注意。
- playlist item metadata 可能需要研究 foobar2000 SDK API。
