# UPnP / DLNA Server 相容性測試計畫

## 優先測試 Server

| Server | 優先級 | 理由 |
|---|---:|---|
| MiniDLNA / ReadyMedia | 高 | 輕量、常見、適合 baseline |
| Jellyfin DLNA | 高 | 開源媒體伺服器，常見 |
| Plex DLNA | 中 | 常見但 DLNA 行為可能較特殊 |
| Synology Media Server | 中 | NAS 使用者常見 |
| Asset UPnP | 中 | 音樂導向，適合高品質音樂庫 |
| BubbleUPnP Server | 中 | 進階 UPnP 場景 |
| Windows Media Sharing | 低 | 可後補 |

## 測試項目

- root Browse。
- child Browse。
- BrowseMetadata。
- pagination。
- 中文 / 日文 / 特殊字元。
- albumArtURI。
- multiple resource。
- HTTP Range。
- seek。
- large library。
- timeout / disconnect。

## Compatibility Matrix 格式

```markdown
| Server | Version | Browse | Playback | Metadata | Album Art | Pagination | Notes |
|---|---|---:|---:|---:|---:|---:|---|
| MiniDLNA | TBD | ✅ | ✅ | ✅ | TBD | TBD | baseline |
```

## 跨網段說明

MVP 使用手動 `rootDesc.xml` URL，因此不依賴 SSDP discovery。  
後續若加入 SSDP discovery，跨 VLAN / VPN / routed network 可能會失效。文件必須明確說明此限制。
