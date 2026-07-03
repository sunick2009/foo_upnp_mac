# 測試策略

## Parser Tests

所有 parser 都應使用 fixture-based tests。

### Fixture 類型

```text
tests/fixtures/
  device_descriptions/
    minidlna_rootDesc.xml
    jellyfin_rootDesc.xml
  soap_responses/
    browse_root_response.xml
    browse_music_response.xml
    soap_fault.xml
  didl_lite/
    containers.xml
    audio_items.xml
    multiple_resources.xml
    unicode_titles.xml
```

## 單元測試重點

| 模組 | 測試 |
|---|---|
| UrlResolver | relative URL、absolute URL、URLBase |
| DeviceDescriptionParser | 找 ContentDirectory |
| SoapBrowseRequestBuilder | SOAP XML 與 headers |
| SoapBrowseResponseParser | Result、NumberReturned、TotalMatches |
| DidlLiteParser | container、item、res、metadata |
| ResourceSelector | 選最佳 audio resource |

## Integration Tests

使用 mock HTTP server 模擬：

- rootDesc.xml
- SOAP Browse response
- SOAP Fault
- timeout
- HTTP 404
- malformed XML

## Manual Compatibility Tests

至少測：

- MiniDLNA / ReadyMedia
- Jellyfin DLNA
- Plex DLNA

## 驗收原則

任何 server 相容性測試若未完成，文件必須標記為 unknown，不應宣稱支援。
