# Phase 0 CLI Usage

## Build

```bash
brew install curl cmake        # one-time setup
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j 8
```

pugixml 與 Catch2 由 CMake FetchContent 自動下載（首次 configure 需要網路）。

## Run tests

```bash
ctest --test-dir build --output-on-failure
```

## Browse a server

```bash
./build/upnp-browser-cli browse \
  --server http://192.168.1.10:8200/rootDesc.xml \
  --object-id 0 \
  --output json
```

| 參數 | 說明 | 預設 |
|---|---|---|
| `--server` | rootDesc.xml URL（必填）| — |
| `--object-id` | UPnP ObjectID | `0` |
| `--browse-flag` | `BrowseDirectChildren` 或 `BrowseMetadata` | `BrowseDirectChildren` |
| `--starting-index` | pagination 起始值 | `0` |
| `--requested-count` | 數量上限，`0` = server 決定 | `100` |
| `--timeout` | request timeout 秒數 | `10` |
| `--output` | `json` 或 `table` | `json` |

Exit codes：`0` 成功、`1` UPnP / 網路錯誤、`2` 參數錯誤。

## End-to-end test with the mock server

不需要真實 UPnP server 也能驗證整條 pipeline：

```bash
python3 tools/mock_upnp_server.py 18200 &

./build/upnp-browser-cli browse --server http://127.0.0.1:18200/rootDesc.xml
./build/upnp-browser-cli browse --server http://127.0.0.1:18200/rootDesc.xml --object-id music
./build/upnp-browser-cli browse --server http://127.0.0.1:18200/rootDesc.xml --object-id bogus
# → Error [SoapFault 701]: No such object

kill %1
```

Mock server 提供：root 兩個 container、`music` 內兩個 audio item
（multi-res FLAC+MP3、unicode 標題）、其他 ObjectID 回傳 UPnP 701 fault。

## Testing against a real server

MiniDLNA / ReadyMedia：

```bash
# rootDesc.xml 通常在 port 8200
./build/upnp-browser-cli browse --server http://<nas-ip>:8200/rootDesc.xml --output table
```

Jellyfin（需啟用 DLNA plugin）：

```bash
./build/upnp-browser-cli browse --server http://<server-ip>:8096/dlna/<udn>/description.xml
```

找 rootDesc URL 的方式：檢查 server 文件，或用 `gssdp-discover` /
Wireshark 觀察 SSDP NOTIFY 內的 `LOCATION` header。

## Known limitations

- 不自動翻頁（ADR-012）：`total_matches > number_returned` 時，
  用 `--starting-index` 手動取下一頁。
- 不支援 SSDP discovery（設計上排除，見 docs/02）。
- 不支援 HTTP authentication。
- `albumArtURI` 若是 relative URL，原樣輸出，不解析。
- 尚未對真實 MiniDLNA / Jellyfin / Plex 驗證（僅 mock server）。
  相容性狀態依 docs/09 的原則標記為 unknown。

## Next steps

- 對至少一台真實 server（MiniDLNA 優先）跑通 browse 並記錄到
  docs/10_compatibility_plan.md。
- M2：整理 core library API、補 mock HTTP server 的 C++ integration tests。
- M3：foobar2000 macOS component（Xcode project、Preferences、Browser panel）。
