# Phase 0 CLI PoC 技術設計

## 目的

Phase 0 的目標不是完成 foobar2000 component，而是先驗證：

1. 是否能讀取 UPnP MediaServer 的 `rootDesc.xml`。
2. 是否能找到 ContentDirectory service。
3. 是否能呼叫 SOAP Browse。
4. 是否能解析 DIDL-Lite。
5. 是否能取得可播放的音訊 resource URL。

## CLI 介面

```bash
upnp-browser-cli browse \
  --server http://192.168.1.10:8200/rootDesc.xml \
  --object-id 0 \
  --output json
```

## browse command 參數

| 參數 | 說明 |
|---|---|
| `--server` | rootDesc.xml URL |
| `--object-id` | UPnP ObjectID，預設 `0` |
| `--browse-flag` | `BrowseDirectChildren` 或 `BrowseMetadata` |
| `--starting-index` | pagination 起始值 |
| `--requested-count` | pagination 數量 |
| `--timeout` | request timeout |
| `--output` | `json` 或 `table` |

## 執行流程

```text
1. GET rootDesc.xml
2. Parse device description XML
3. Locate ContentDirectory service
4. Resolve controlURL
5. Build SOAP Browse request
6. POST to controlURL
7. Parse SOAP response
8. Extract DIDL-Lite Result
9. Parse container / item / res
10. Output JSON or table
```

## JSON 輸出範例

```json
{
  "server": "http://192.168.1.10:8200/rootDesc.xml",
  "object_id": "0",
  "number_returned": 2,
  "total_matches": 2,
  "items": [
    {
      "type": "container",
      "id": "music",
      "parent_id": "0",
      "title": "Music",
      "child_count": 120
    },
    {
      "type": "item",
      "id": "track-123",
      "parent_id": "music",
      "title": "Example Track",
      "artist": "Example Artist",
      "album": "Example Album",
      "duration": "00:04:12",
      "mime_type": "audio/flac",
      "protocol_info": "http-get:*:audio/flac:*",
      "resource_url": "http://192.168.1.10:8200/media/track.flac"
    }
  ]
}
```

## Phase 0 驗收標準

- 可對至少一台真實 UPnP server 執行 browse。
- 可列出 container。
- 可列出 audio item。
- 可取得 `<res>` URL。
- 有 fixture-based parser tests。
- 錯誤訊息可診斷。
