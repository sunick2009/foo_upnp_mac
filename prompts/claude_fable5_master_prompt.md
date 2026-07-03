# Claude Fable 5 Master Prompt

你是一位資深 macOS / C++ / Objective-C++ / foobar2000 component 開發工程師。請協助我從零開始開發一個開源專案：`foo_dms_browser_mac`。

## 專案目標

開發一個 foobar2000 macOS 原生 component，讓使用者可以手動新增 UPnP / DLNA Media Server 的 `rootDesc.xml` URL，瀏覽 ContentDirectory，並將遠端音樂項目加入 foobar2000 playlist 進行播放。

## 重要限制

1. 第一版不是完整 `foo_upnp` clone。
2. 第一版不做 MediaServer。
3. 第一版不做 MediaRenderer。
4. 第一版不做控制其他 renderer。
5. 第一版不做轉碼。
6. 第一版不依賴 SSDP discovery。
7. 第一版必須支援手動 `rootDesc.xml` URL。
8. UPnP discovery 可作為後續功能，不可阻塞 MVP。
9. 第一階段不要建立 Xcode project。
10. 第一階段不要整合 foobar2000 SDK。

## 建議技術方向

- Phase 0：CMake-based CLI PoC。
- Core：C++17 或 C++20。
- XML：pugixml 或 tinyxml2。
- HTTP：libcurl 或簡單 wrapper。
- Phase 2 才導入 foobar2000 macOS SDK。
- Phase 2 才建立 Xcode project。
- macOS UI 使用 Objective-C++ / Cocoa。

## 請先完成

1. 建立 repo 結構。
2. 建立 CMake-based CLI skeleton。
3. 實作 Phase 0 CLI PoC。
4. 所有 parser 使用 fixture-based tests。
5. 網路請求必須有 timeout 與診斷錯誤。
6. 每個 issue 對應一個小 MR。
7. 每完成一個 issue，附上測試方法、已知限制與下一步建議。

## 第一階段交付目標

請先輸出或實作：

- repo tree
- CMake skeleton
- CLI command design
- core library module design
- 第一批 10 個 GitLab issues
- 第一個 MR 的具體修改內容

## 嚴格要求

請不要一次產生大型不可審查變更。  
請不要直接跳到 foobar2000 UI。  
請先讓 CLI 可以成功 browse 真實 UPnP server。
