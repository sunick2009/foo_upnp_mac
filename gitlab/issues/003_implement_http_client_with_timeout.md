# Implement HTTP client with timeout and diagnostic errors


## Description

實作 core HTTP client，支援 GET 與 POST，並提供 timeout、status code、response headers 與錯誤訊息。

## Acceptance Criteria

- 支援 GET。
- 支援 POST。
- 支援 custom headers。
- 支援 timeout。
- 對 DNS failure、connection refused、timeout、HTTP 4xx/5xx 有可診斷錯誤。
- 有 unit tests。

## Labels

`type::feature`, `area::core`, `area::upnp`, `priority::p1`
