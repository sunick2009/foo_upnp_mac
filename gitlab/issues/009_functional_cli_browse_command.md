# Implement functional CLI browse command


## Description

整合 HTTP client、device parser、SOAP Browse client 與 DIDL-Lite parser，讓 CLI 可以對指定 `rootDesc.xml` URL 瀏覽 ObjectID。

## Acceptance Criteria

- `upnp-browser-cli browse --server <url> --object-id 0` 可列出結果。
- 支援 JSON output。
- 支援 table output。
- 網路錯誤與 UPnP 錯誤可診斷。
- 至少使用一個真實 UPnP server 測試。

## Labels

`type::feature`, `area::cli`, `area::upnp`, `priority::p0`
