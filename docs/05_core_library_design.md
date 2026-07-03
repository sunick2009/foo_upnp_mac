# Core Library 設計

## 目標

建立一套可被 CLI 與 foobar2000 component 共用的 UPnP core library。

## 建議目錄

```text
src/core/
  http/
    HttpClient.hpp
    HttpClient.cpp
    HttpError.hpp
  xml/
    XmlUtils.hpp
  upnp/
    UrlResolver.hpp
    UpnpObject.hpp
    UpnpServerConfig.hpp
    DeviceDescriptionParser.hpp
    ContentDirectoryClient.hpp
    SoapBrowseRequestBuilder.hpp
    SoapBrowseResponseParser.hpp
    DidlLiteParser.hpp
    ResourceSelector.hpp   ← 從多個 res 中選出最適合播放的資源
```

## 主要類別

### `HttpClient`

責任：

- GET
- POST
- custom headers
- timeout
- status code
- diagnostic errors

### `UrlResolver`

責任：

- rootDesc.xml URL 與 relative `controlURL` 合成 absolute URL。
- 支援 `URLBase`。
- 支援 root-relative path。

### `DeviceDescriptionParser`

責任：

- 解析 UPnP device description XML。
- 找到 `urn:schemas-upnp-org:service:ContentDirectory:1` 或相容版本。
- 取得 `controlURL`、`eventSubURL`、`SCPDURL`。

### `ContentDirectoryClient`

責任：

- 呼叫 Browse。
- 呼叫 BrowseMetadata。
- 後續可加入 Search。
- 管理 pagination。

### `DidlLiteParser`

責任：

- 解析 container。
- 解析 item。
- 解析 `<res>`。
- 擷取 metadata。

### `UpnpObject`

建議 model：

```cpp
enum class UpnpObjectType {
    Container,
    AudioItem,
    Unknown
};

struct UpnpResource {
    std::string url;
    std::string protocolInfo;
    std::string mimeType;
    std::string duration;
    std::optional<uint64_t> size;
    std::optional<uint32_t> bitrate;
};

struct UpnpObject {
    UpnpObjectType type;
    std::string id;
    std::string parentId;
    std::string title;
    std::optional<std::string> artist;
    std::optional<std::string> album;
    std::optional<std::string> genre;
    std::optional<std::string> albumArtUri;
    std::vector<UpnpResource> resources;
};
```

### `ResourceSelector`

責任：

- 從 `UpnpObject.resources` 裡選出最適合播放的 `UpnpResource`。
- 只考慮 `http-get` protocol。
- 依 MIME type 優先順序選擇（FLAC > WAV > MP3 > ...）。
- 參見 ADR-007。

## 測試策略

- parser 使用 fixture-based tests。
- 網路層可用 mock server。
- URL resolver 使用大量 edge cases。
- ResourceSelector 使用多個 res 選擇的 edge cases。
