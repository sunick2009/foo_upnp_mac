# ADR-007: ResourceSelector — 多個 res 元素的選擇策略

**狀態：** Accepted  
**日期：** 2026-07-03

## Context

一個 DIDL-Lite `<item>` 可以有多個 `<res>` 元素，例如：

```xml
<item id="track-1" ...>
  <res protocolInfo="http-get:*:audio/flac:*">http://server/track.flac</res>
  <res protocolInfo="http-get:*:audio/mpeg:DLNA.ORG_PN=MP3">http://server/track.mp3</res>
  <res protocolInfo="http-get:*:audio/x-ms-wma:*">http://server/track.wma</res>
</item>
```

`DidlLiteParser` 把所有 `<res>` 都存進 `UpnpObject.resources`。
但 CLI 輸出、foobar2000 playlist 加入時，需要選一個「最佳」resource。

## 問題

- `DidlLiteParser` 應該選 resource 嗎？（「解析」和「選擇」職責混在一起）
- 還是應該有獨立的 `ResourceSelector`？

## Decision

**新增 `ResourceSelector` 作為獨立的 free function 或 utility class。**

`DidlLiteParser` 只負責解析，不做選擇。
`ContentDirectoryClient::browse()` 後，由呼叫端或 `ResourceSelector` 選擇。

### ResourceSelector API

```cpp
namespace upnp {

// Priority list of preferred MIME types (highest priority first)
// Caller can override this list.
const std::vector<std::string> kDefaultMimeTypePriority = {
    "audio/flac",
    "audio/x-flac",
    "audio/wav",
    "audio/x-wav",
    "audio/aiff",
    "audio/mpeg",
    "audio/mp4",
    "audio/ogg",
    "audio/x-ms-wma",
};

// Returns the "best" resource from the item's resource list.
// Selection criteria (in order):
//   1. protocol must be "http-get" (only HTTP resources are playable by foobar2000)
//   2. Highest priority mimeType per kDefaultMimeTypePriority
//   3. If no match, return first http-get resource
//   4. If no http-get resource, return nullopt
std::optional<UpnpResource> selectBestResource(
    const std::vector<UpnpResource>& resources,
    const std::vector<std::string>& mimeTypePriority = kDefaultMimeTypePriority
);

} // namespace upnp
```

### 選擇邏輯

```
1. Filter: keep only resources where protocolInfo starts with "http-get"
   (discard rtsp://, mms://, etc.)
2. Among filtered: find first resource whose mimeType is in priority list
   (lowest index = highest priority)
3. If no match: use first http-get resource (best effort)
4. If empty: return nullopt
```

## Phase 0 CLI 使用

CLI 在 JSON 輸出裡同時輸出：
- `resources`: all resources (raw)
- `selected_resource`: ResourceSelector 選出的結果（可為 null）

這讓使用者能 debug selector 的決策，也能看到 server 提供了哪些選項。

## Fixture Tests

測試 case：
1. 單一 FLAC resource → 選它
2. FLAC + MP3 → 選 FLAC
3. MP3 + WMA → 選 MP3
4. 只有 RTSP → 回傳 nullopt
5. 空 resources → 回傳 nullopt
6. FLAC + RTSP → 選 FLAC（過濾掉 RTSP）

## Consequences

- `ResourceSelector` 加入 `src/core/upnp/ResourceSelector.hpp`（純 header 或小 .cpp）。
- `09_testing_strategy.md` 的 `ResourceSelector` 測試項目現在有正式設計依據。
- Phase 1 component 可以允許使用者在 Preferences 裡調整 MIME type 優先順序。
