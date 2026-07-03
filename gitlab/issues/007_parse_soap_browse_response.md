# Parse SOAP Browse response and extract DIDL-Lite result


## Description

解析 ContentDirectory Browse response，取得 Result、NumberReturned、TotalMatches、UpdateID。

## Acceptance Criteria

- 可解析正常 SOAP response。
- 可解析 escaped DIDL-Lite Result。
- 可處理 SOAP fault。
- 可處理空結果。
- 有 fixture tests。

## Labels

`type::feature`, `area::upnp`, `area::metadata`, `priority::p1`
