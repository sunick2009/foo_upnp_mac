# Create Phase 0 CLI skeleton


## Description

建立 `upnp-browser-cli` 的最小 CLI 程式，支援 `browse` subcommand，但可以先回傳 stub output。

## Acceptance Criteria

- 可執行 `upnp-browser-cli --help`。
- 可執行 `upnp-browser-cli browse --server <url> --object-id 0`。
- browse command 有清楚錯誤訊息。
- 尚未實作實際 UPnP request 時，需回傳 `not implemented`。

## Labels

`type::feature`, `area::cli`, `priority::p1`
