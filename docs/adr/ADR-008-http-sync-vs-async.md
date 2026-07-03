# ADR-008: HttpClient — 同步 vs 非同步設計

**狀態：** Accepted  
**日期：** 2026-07-03

## Context

`HttpClient` 是所有 UPnP 網路操作的基礎層。
Phase 0 CLI 和 Phase 1 foobar2000 component 對 HTTP 的需求不同：

| 場景 | 需求 |
|---|---|
| Phase 0 CLI | 同步、blocking，簡單即可 |
| Phase 1 component | 不能 block UI thread，需要 background thread 或 async |

若 Phase 0 把 `HttpClient` 設計成 blocking synchronous，
Phase 1 就必須在 background thread 包一層，或重寫 `HttpClient`。

## Decision

**Phase 0 設計同步 API，預留 Phase 1 非同步的插槽。**

### Phase 0 HttpClient API（同步）

```cpp
class HttpClient {
public:
    struct Response {
        int statusCode;
        std::string body;
        std::map<std::string, std::string> headers;
    };

    struct Options {
        int timeoutSeconds = 10;
        std::string userAgent = "foo_dms_browser_mac/0.1";
        std::map<std::string, std::string> extraHeaders;
    };

    explicit HttpClient(Options opts = {});

    // Blocking GET. Throws HttpException on error.
    Response get(const std::string& url);

    // Blocking POST. Throws HttpException on error.
    Response post(const std::string& url,
                  const std::string& body,
                  const std::string& contentType);
};
```

**丟 exception 條件：**
- Network error / connection refused → `HttpException(msg, -1)`
- Timeout → `HttpException("Request timed out", -1)`
- HTTP 4xx / 5xx → **不丟 exception**，回傳 Response 讓呼叫端決定

注意：HTTP 4xx / 5xx 不丟 exception，因為 SOAP Fault 是 HTTP 200，
而 HTTP 非 200 的處理邏輯在 `ContentDirectoryClient` 裡。

### Phase 1 非同步方向（不實作，只預留）

Phase 1 的 component 會在 background thread 呼叫同步 `HttpClient`，
用 `std::async` 或 GCD（Grand Central Dispatch）包裝：

```cpp
// Phase 1 pattern (not implemented in Phase 0)
std::future<BrowseResult> ContentDirectoryClient::browseAsync(objectId);
```

`HttpClient` 本身不需要改，只是在更高層次加 async wrapper。

## Consequences

- Phase 0 `ContentDirectoryClient::browse()` 是 blocking synchronous call。
- CLI 的 `main()` 直接呼叫，無需 thread management。
- Phase 1 需要在 UI thread 之外呼叫 `browse()`，用 `std::async` 或 GCD。
- `HttpClient` 的 destructor 必須安全（若在 thread 裡被 destroy）。
- 測試時可以透過 mock HTTP server 或 dependency injection 換掉 `HttpClient`。
