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
| MiniDLNA / ReadyMedia | 1.3.3 | ✅ (CLI) | ⏳ 元件級待測 | ✅ | ✅ (URL 取得) | ✅ | 見下方 quirks；CLI 級 2026-07-18 實測 |
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

### MiniDLNA 1.3.3 實測紀錄（2026-07-18，CLI 級）

環境：docker `vladgh/minidlna`（OrbStack，port 對映 18300→8200），
media 為 ffmpeg 產生的 13 首含 tag 測試庫（12 MP3 + 1 FLAC，內嵌封面、
中日文標題）。Device description：`http://<host>:8200/rootDesc.xml`
（與 mock 相同路徑）。

- **root Browse**：4 個 container（Browse Folders / Music / Pictures /
  Video）。
- **child Browse**：`Music` → Album / All Music / Artist / Folders /
  Genre / Playlists / Recently Added；`All Music (1$4)` 13 首全對。
- **pagination**：`--starting-index 5 --requested-count 3` 正確回傳
  `number_returned=3, total_matches=13`。
- **metadata**：title / album / date（正規化為 `2024-01-01`）/
  originalTrackNumber 完整；artist 對應見 quirks。
- **中文 / 日文標題**：`第二首歌`、`三曲目のテスト` 正常。
- **albumArtURI**：absolute URL（`/AlbumArt/…jpg`），HTTP 200
  image/jpeg 可下載。
- **resource**：每 item 單一 `<res>`（不轉碼）；MP3 帶完整 DLNA 參數
  protocolInfo，FLAC 為 `audio/x-flac`（已在 ResourceSelector 優先
  清單）。duration/size/bitrate/sampleFrequency/nrAudioChannels 齊全。
- **HTTP Range**：`Accept-Ranges: bytes`，`Range: bytes=0-99` 回 206。
- **SOAP fault**：不存在的 ObjectID 回標準 `701 No such object`。
- **fixtures**：真實回應已入 repo —
  `tests/fixtures/didl_lite/minidlna_133_audio_items.xml`、
  `tests/fixtures/soap_responses/minidlna_133_browse_response.xml`，
  parser 回歸測試 `parses a real MiniDLNA 1.3.3 browse payload`。
- **元件級待測**（fb2k UI）：加入 playlist、播放、seek、封面顯示
  （可直接用上述 docker 環境，見 docs/21）。

### MiniDLNA quirks（相容性差異）

1. **ID3 tag 對應**：album artist（TPE2）放在 `upnp:artist`，
   track artist（TPE1）放在 `dc:creator` —— 與 foo_upnp 相反習慣。
   元件顯示的 `%artist%` 會是 album artist；不可假設 `upnp:artist`
   一定是曲目演出者。
2. **ObjectID 用 `$` 分隔**（`1$4$0`），並有 `refID` 屬性
   （同一曲目在多個 view 中互相引用）。
3. **protocolInfo 第 4 欄帶完整 DLNA 參數**
   （`DLNA.ORG_PN=MP3;DLNA.ORG_OP=01;…`）；mimeType 欄位（第 3 欄）
   維持乾淨，不影響優先清單比對。
4. **FLAC 的 mime 是 `audio/x-flac`**（非 `audio/flac`）。
5. **resource / albumArt URL 以 server 綁定介面的 IP 產生**
   （容器內為容器 IP）；跨 NAT / port-forward 環境需注意 URL
   可達性——這是部署議題而非元件缺陷。
6. **date 正規化**：tag 只有年份 `2024` 時回 `2024-01-01`。

## 跨網段說明

MVP 使用手動 `rootDesc.xml` URL，因此不依賴 SSDP discovery。  
後續若加入 SSDP discovery，跨 VLAN / VPN / routed network 可能會失效。文件必須明確說明此限制。
