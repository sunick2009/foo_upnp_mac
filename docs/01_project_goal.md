# 專案最終目標

## 專案名稱

建議名稱：

```text
foo_dms_browser_mac
```

DMS 指 Digital Media Server。這個名稱比 `foo_upnp_browser_mac` 更精準，因為第一版目標不是完整 UPnP 套件，而是 UPnP / DLNA Media Server Browser。

## 最終產品定位

開發一個 foobar2000 macOS 原生 component，讓使用者可以：

1. 手動新增 UPnP / DLNA Media Server 的 `rootDesc.xml` URL。
2. 解析 UPnP device description。
3. 找到 ContentDirectory service。
4. 呼叫 ContentDirectory `Browse` action。
5. 瀏覽遠端 container 與 audio item。
6. 將選取的音樂資源加入 foobar2000 playlist。
7. 在本機 foobar2000 播放遠端 HTTP resource URL。

## 核心流程

```text
使用者輸入 rootDesc.xml URL
→ component 讀取 UPnP device description
→ 找到 ContentDirectory service
→ 呼叫 Browse action
→ 解析 DIDL-Lite
→ 顯示 container / track
→ 使用者選取 track
→ 加入 foobar2000 playlist 或立即播放
```

## 明確非目標

第一版不做：

- foobar2000 自己當 UPnP MediaServer。
- foobar2000 自己當 UPnP Renderer。
- 控制其他 UPnP Renderer。
- 音訊轉碼。
- per-device profile。
- 完整 DLNA 相容性矩陣。
- 強制依賴 SSDP discovery。

## 設計原則

1. MVP 優先支援手動 server URL。
2. UPnP discovery 延後做，不阻塞 MVP。
3. 核心 UPnP 邏輯應抽成不依賴 foobar2000 SDK 的 core library。
4. 所有 parser 必須使用 fixture-based tests。
5. 網路錯誤必須有可診斷錯誤訊息。
6. 文件必須誠實說明已知限制。
