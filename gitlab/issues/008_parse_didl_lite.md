# Parse DIDL-Lite containers and audio items


## Description

實作 DIDL-Lite parser，解析 container 與 item，並擷取 title、artist、album、duration、protocolInfo、resource URL。

## Acceptance Criteria

- 可解析 container。
- 可解析 musicTrack item。
- 可解析多個 res。
- 可解析 duration。
- 可解析 albumArtURI。
- 中文與特殊字元不亂碼。
- 有 fixture tests。

## Labels

`type::feature`, `area::metadata`, `area::upnp`, `priority::p1`
