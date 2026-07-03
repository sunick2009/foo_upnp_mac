# Parse UPnP device description and locate ContentDirectory service


## Description

實作 `rootDesc.xml` parser，找到 MediaServer device 下的 ContentDirectory service，並解析 serviceType、serviceId、controlURL、eventSubURL、SCPDURL。

## Acceptance Criteria

- 可解析 fixture `rootDesc.xml`。
- 可找到 ContentDirectory service。
- 可處理 relative URL。
- 找不到 ContentDirectory 時有清楚錯誤。
- 有 unit tests。

## Labels

`type::feature`, `area::upnp`, `area::metadata`, `priority::p1`
