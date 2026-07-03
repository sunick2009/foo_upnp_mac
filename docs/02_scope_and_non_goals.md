# 範圍與非範圍

## MVP 範圍

MVP 只解決一件事：

> 從指定 UPnP / DLNA Media Server 瀏覽遠端音樂並加入 foobar2000 播放清單。

## MVP 功能

### Preferences

- 新增 server。
- 移除 server。
- 儲存 server list。
- Test connection。
- 設定 timeout。
- 顯示解析到的 ContentDirectory controlURL。

### Browser Panel

- 選擇 server。
- 瀏覽 root container。
- 瀏覽 child container。
- 顯示 item list。
- 雙擊加入 playlist。
- 右鍵選單：
  - Add to playlist
  - Play now

### Metadata

- title
- artist
- album
- genre
- duration
- MIME type
- resource URL
- albumArtURI，後續補強

## 延後功能

- SSDP discovery。
- album art 顯示。
- Search action。
- 大型目錄快取。
- favorites。
- debug report export。
- compatibility matrix 自動化測試。

## 不建議第一版實作

- MediaServer。
- Renderer。
- Control Point 控制其他裝置。
- transcoding。
- UPnP event subscription。
- 網際網路遠端 UPnP relay。
