# Implement UPnP URL resolver for relative service URLs


## Description

UPnP device description 中的 `controlURL` 可能是 relative path。實作 URL resolver，根據 `rootDesc.xml` URL 與 `URLBase` 解析出 absolute `controlURL`。

## Acceptance Criteria

- 支援 absolute URL。
- 支援 root-relative path。
- 支援 relative path。
- 支援 device description 中的 URLBase。
- 有 unit tests。

## Labels

`type::feature`, `area::core`, `area::upnp`, `priority::p1`
