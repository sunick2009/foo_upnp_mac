# ADR-003: 命名規範 — Browse / browse / Browser

**狀態：** Accepted  
**日期：** 2026-07-03

## Context

「Browse」這個詞在本專案裡有三種意思，容易混淆：

1. **UPnP SOAP action**: `Browse`（ContentDirectory 規範定義，大寫開頭）
2. **CLI subcommand**: `upnp-browser-cli browse`（小寫）
3. **UI 行為**: 使用者在 Browser Panel 裡瀏覽伺服器內容

若在 code、API、文件裡不統一，PR review 時容易搞混。

## Decision

### C++ code 命名規則

| 概念 | 命名 |
|---|---|
| SOAP Browse action (UPnP 規範) | `ContentDirectoryClient::browse()` |
| SOAP BrowseMetadata action | `ContentDirectoryClient::browseMetadata()` |
| Browse 的 BrowseFlag 參數 | `BrowseFlag::DirectChildren`, `BrowseFlag::Metadata` |
| Browse 回傳的結果 | `BrowseResult` |
| CLI subcommand | `browse`（全小寫，符合 Unix 慣例）|

### 文件命名規則

- 當指 UPnP 規範的 SOAP action 時，寫 `Browse action` 或 `Browse`（保留大寫）。
- 當指 CLI 指令時，寫 `` `browse` ``（code font，小寫）。
- 當指 UI 的瀏覽行為時，寫「瀏覽」或「browser panel」。

### 類別命名規則

| 類別 | 說明 |
|---|---|
| `ContentDirectoryClient` | 呼叫 ContentDirectory SOAP actions |
| `SoapBrowseRequestBuilder` | 建構 Browse SOAP request body |
| `SoapBrowseResponseParser` | 解析 Browse SOAP response body |
| `DidlLiteParser` | 解析 DIDL-Lite XML |

## Consequences

- 既有文件裡的 Browse/browse 用法需要在下一次文件修訂時統一。
- Code review checklist 加入「Browse 大小寫」檢查。
