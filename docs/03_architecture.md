# 架構設計

## 分層架構

```text
foobar2000 macOS component
  ├─ Preferences UI
  ├─ Browser Panel UI
  ├─ Playlist / Playback integration
  │
  └─ core library
      ├─ HttpClient
      ├─ UrlResolver
      ├─ DeviceDescriptionParser
      ├─ ContentDirectoryClient
      ├─ SoapBrowseRequestBuilder
      ├─ SoapBrowseResponseParser
      ├─ DidlLiteParser
      └─ UpnpObject model
```

## Core Library 原則

Core library 不應依賴：

- foobar2000 SDK
- Cocoa UI
- Objective-C runtime
- Xcode-specific project setting

這樣可以先用 CLI 驗證 UPnP 邏輯，再整合到 component。

## Phase 0 CLI

CLI 是第一階段的技術風險驗證工具。

```bash
upnp-browser-cli browse \
  --server http://192.168.1.10:8200/rootDesc.xml \
  --object-id 0 \
  --output json
```

## Phase 1 Component

component 只負責：

- 讀取使用者設定。
- 呼叫 core library。
- 顯示資料。
- 將 resource URL 轉成 foobar2000 playlist item。

## 建議語言與工具

| 區塊 | 建議 |
|---|---|
| core library | C++17 或 C++20 |
| CLI | C++ + CMake |
| XML parser | pugixml 或 tinyxml2 |
| HTTP client | libcurl 或輕量 HTTP wrapper |
| macOS UI | Objective-C++ / Cocoa |
| build | Phase 0 使用 CMake；Phase 2 導入 Xcode |
| tests | fixture-based tests |
