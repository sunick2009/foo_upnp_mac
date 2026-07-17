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

## 實測結果

依 docs/09 原則：未實測的項目標記 unknown，不宣稱支援。

| Server | Version | Browse | Playback | Metadata | Album Art | Pagination | Notes |
|---|---|---:|---:|---:|---:|---:|---|
| foobar2000 UPnP Media Server (foo_upnp) | 0.99.49 | ✅ | ✅* | ✅ | ✅ (URL 取得) | ✅ | 見下方 quirks |
| MiniDLNA / ReadyMedia | — | unknown | unknown | unknown | unknown | unknown | 尚未實測 |
| Jellyfin DLNA | — | unknown | unknown | unknown | unknown | unknown | 尚未實測 |
| Plex DLNA | — | unknown | unknown | unknown | unknown | unknown | 尚未實測 |

\* 2026-07-03 由 component（foo_dms_browser 0.1.0 in foobar2000 v2.25.8
macOS）實測：browser 加入 playlist 後可直接播放。此前 CLI 階段已由
HTTP HEAD 驗證 `Accept-Ranges: bytes` + 正確 `Content-Type`。
2026-07-17 由 component（0.2.0-dev，同環境 ARM64）實測 seek 可移動至
指定時間，播放狀態顯示 PCM 1411 kbps / 44100 Hz / stereo
（見 `docs/21_manual_test_checklist.md` E2E 紀錄）。

### foo_upnp 0.99.49 實測紀錄（2026-07-03）

CLI 對 `http://10.102.0.10:2333/DeviceDescription.xml` 全部通過：

- **root Browse**：3 個 container（播放列表 / 媒体库 / 播放流捕获）。
- **child Browse**：多層瀏覽正常（`0/0` → 309 個 playlist container → audio items）。
- **BrowseMetadata**：對單一 item 回傳完整 metadata。
- **pagination**：`--starting-index 100 --requested-count 5` 正確回傳
  `number_returned=5, total_matches=309`。
- **中文 / 日文標題**：正常（server 本身是簡中介面，含大量日文專輯名）。
- **albumArtURI**：absolute URL，直接可用。
- **multiple res**：每個 item 3 個 res（LPCM ×2 + WAV 轉碼 profile）。
  ResourceSelector 正確選 `audio/wav`（L16 不在優先清單 → fallback 規則生效）。
- **HTTP Range**：resource URL 回傳 `Accept-Ranges: bytes`。

### foo_upnp quirks（相容性差異）

1. **Device description 路徑是 `/DeviceDescription.xml`**，不是 `rootDesc.xml`。
   手動輸入 URL 時需知道正確路徑。
2. **SOAP fault 用自訂 errorCode `404`（"File not found"）**，
   而非 UPnP 標準的 `701 No such object`。錯誤處理不可 hardcode 701。
3. **protocolInfo 的 contentFormat 可含參數**：
   `audio/L16;rate=44100;channels=2`。mimeType 欄位會包含完整參數字串，
   優先清單比對是 exact match，所以帶參數的 mime 走 fallback 路徑。
4. ObjectID 格式為路徑狀（`0/0/58/0I`），非 flat ID。

## 跨網段說明

MVP 使用手動 `rootDesc.xml` URL，因此不依賴 SSDP discovery。  
後續若加入 SSDP discovery，跨 VLAN / VPN / routed network 可能會失效。文件必須明確說明此限制。
