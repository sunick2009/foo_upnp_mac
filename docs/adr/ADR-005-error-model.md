# ADR-005: Error Model — Exception vs Expected vs Error Code

**狀態：** Accepted  
**日期：** 2026-07-03

## Context

系統有兩類錯誤需要處理：

1. **Expected failures**（預期中的失敗）：
   - HTTP timeout
   - HTTP 4xx / 5xx
   - XML parse error（malformed response）
   - SOAP Fault（server 回傳 Browse 失敗）
   - ContentDirectory service not found
   - No playable resources in item

2. **Programming errors**（不應發生的 bug）：
   - Null pointer dereference
   - Invalid argument to a function
   - Violated preconditions

## Options

### Option A: Exception hierarchy

```cpp
class UpnpError : public std::runtime_error { ... };
class HttpError : public UpnpError { int statusCode; };
class XmlParseError : public UpnpError { ... };
class SoapFaultError : public UpnpError { int faultCode; std::string faultString; };
class ServiceNotFoundError : public UpnpError { ... };
```

- 優點：呼叫端可以用 try/catch；錯誤自然向上傳播；C++ 標準慣用。
- 缺點：exception 是 invisible control flow；在 async/performance-critical 路徑有開銷。

### Option B: std::expected<T, Error> (C++23)

```cpp
using UpnpResult<T> = std::expected<T, UpnpError>;
auto result = client.browse(objectId);
if (!result) { /* handle error */ }
```

- 優點：錯誤是明確的回傳值；呼叫端必須處理；符合 Rust/Haskell 風格。
- 缺點：C++23（CMakeLists.txt 目前設 C++20）；error 型別需要設計；容易被 ignore。

### Option C: 混用 — exception for unexpected，expected for expected

- Exceptions：programming errors, internal bugs。
- `std::expected` 或 error code：HTTP, XML, SOAP failures。
- 優點：區分程度最清楚。
- 缺點：兩套 error handling，呼叫端需要理解兩種模式。

## Decision

**選 Option A（exception hierarchy），Phase 0 使用。**

理由：
- Phase 0 CLI 的錯誤模型簡單：catch → print → exit(1)。
- C++17 已有 exception；不需要 C++23。
- 錯誤在 CLI 場景只需要向上傳至 `main()`，exception 最省力。
- Phase 1 component 若需要 async，可以在 boundary 處 catch 並轉換為 callback error。

### Exception 類別設計

```cpp
// base
class UpnpException : public std::runtime_error {
public:
    explicit UpnpException(const std::string& msg) : std::runtime_error(msg) {}
};

// HTTP layer
class HttpException : public UpnpException {
public:
    int statusCode;  // -1 if network error (timeout, connection refused)
    HttpException(const std::string& msg, int code = -1);
};

// XML parse failures
class XmlParseException : public UpnpException {
public:
    explicit XmlParseException(const std::string& msg);
};

// SOAP Fault response (HTTP 200 but contains s:Fault)
class SoapFaultException : public UpnpException {
public:
    int faultCode;
    std::string faultString;
    SoapFaultException(int code, const std::string& str);
};

// ContentDirectory service missing from device description
class ServiceNotFoundException : public UpnpException {
public:
    explicit ServiceNotFoundException(const std::string& msg);
};

// No playable resource found in item
class NoResourceException : public UpnpException {
public:
    explicit NoResourceException(const std::string& msg);
};
```

### SOAP Fault 處理

`SoapBrowseResponseParser` 在 parse 時若發現 `<s:Fault>` 元素，
丟出 `SoapFaultException`，而不是回傳正常的 `BrowseResult`。

CLI 的 `main()` 捕捉所有 `UpnpException` 並印出訊息：

```
Error [SoapFault 701]: No such object
Error [Http 404]: rootDesc.xml not found at http://...
Error [Timeout]: Request to http://... timed out after 10s
```

## Consequences

- 所有 public API（`HttpClient`, parser classes）丟 exception，不回傳 error code。
- `ContentDirectoryClient::browse()` 的回傳型別是 `BrowseResult`（非 optional）。
- Phase 1 component 呼叫 `browse()` 時需要在 background thread 包 try/catch。
- 若 Phase 1 需要 `std::expected`，在 ADR-005v2 裡重新決策。
