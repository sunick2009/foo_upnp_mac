# Architecture Decision Records

本目錄存放本專案的 ADR（Architecture Decision Records）。
每個 ADR 記錄一個架構或設計決策：背景、選項、決策理由與後果。

狀態：`Proposed` → `Accepted` → `Deprecated` / `Superseded`

| ADR | 標題 | 狀態 |
|---|---|---|
| [ADR-001](ADR-001-upnp-object-model.md) | UpnpObject 型別設計 — union struct vs tagged types | Accepted |
| [ADR-002](ADR-002-protocol-info-parsing.md) | protocolInfo 解析策略 | Accepted |
| [ADR-003](ADR-003-naming-conventions.md) | 命名規範 — Browse / browse / Browser | Accepted |
| [ADR-004](ADR-004-url-resolver-api.md) | UrlResolver API 設計 | Accepted |
| [ADR-005](ADR-005-error-model.md) | Error Model — Exception vs Expected vs Error Code | Accepted |
| [ADR-006](ADR-006-soap-didl-parse-layers.md) | SOAP Browse Response 解析層次設計 | Accepted |
| [ADR-007](ADR-007-resource-selector.md) | ResourceSelector — 多個 res 元素的選擇策略 | Accepted |
| [ADR-008](ADR-008-http-sync-vs-async.md) | HttpClient — 同步 vs 非同步設計 | Accepted |
| [ADR-009](ADR-009-http-library.md) | HTTP 實作 — libcurl vs Apple URLSession | Accepted |
| [ADR-010](ADR-010-dependency-management.md) | CMake Dependency Management | Accepted |
| [ADR-011](ADR-011-test-framework.md) | Test Framework 選擇 | Accepted |
| [ADR-012](ADR-012-pagination-strategy.md) | Browse Pagination 策略 | Accepted |
| [ADR-013](ADR-013-macos-build-without-xcode.md) | macOS Component Build — 不依賴完整 Xcode | Accepted |
| [ADR-014](ADR-014-component-ui-architecture.md) | Component UI 形式與建構方式 | Accepted |
| [ADR-015](ADR-015-server-list-persistence.md) | Server 清單持久化 | Accepted |
| [ADR-016](ADR-016-playlist-integration.md) | Playlist 整合 — 加入語意與 metadata 預填 | Accepted |
| [ADR-017](ADR-017-component-threading-model.md) | Component 執行緒模型 | Accepted |
