# foobar2000 macOS Component 規劃

## 導入時機

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
  foo_dms_browser_mac.xcodeproj
  foo_dms_browser_mac/
    ComponentEntry.mm
    PreferencesPanel.mm
    BrowserPanel.mm
    ServerConfigStore.mm
    PlaylistIntegration.mm
```

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
- Xcode project 與 SDK 設定可能需要手動調整。
- UI lifecycle 與 thread safety 需要特別注意。
- playlist item metadata 可能需要研究 foobar2000 SDK API。
