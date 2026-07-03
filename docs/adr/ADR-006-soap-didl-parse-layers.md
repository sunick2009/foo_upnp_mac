# ADR-006: SOAP Browse Response 解析層次設計

**狀態：** Accepted  
**日期：** 2026-07-03

## Context

ContentDirectory Browse 的 HTTP response 結構是三層巢狀：

```
Layer 1: HTTP response body
  └── Layer 2: SOAP Envelope / Body / BrowseResponse
        └── <Result> (XML-escaped string)
              └── Layer 3: DIDL-Lite XML
                    ├── <container>
                    └── <item>
                          └── <res>
```

`<Result>` 欄位是一個 **XML-escaped string**，不是嵌入的 XML 節點。
必須先 unescape（`&lt;` → `<` 等），再對 DIDL-Lite 進行第二次 XML parse。

## 問題：誰負責哪一層？

目前設計有：
- `SoapBrowseResponseParser`
- `DidlLiteParser`
- `ContentDirectoryClient`

需要明確定義每個類別的輸入輸出。

## Decision

### 層次劃分

```
ContentDirectoryClient::browse(objectId, flag, startIndex, count)
  │
  ├── 呼叫 HttpClient::post(url, soapBody, headers)
  │     → HTTP response body (string)
  │
  ├── 呼叫 SoapBrowseResponseParser::parse(httpBody)
  │     → BrowseResult { resultXml: string, numberReturned, totalMatches, updateId }
  │
  └── 呼叫 DidlLiteParser::parse(browseResult.resultXml)
        → vector<UpnpObject>
```

### BrowseResult 中間型別

```cpp
struct BrowseResult {
    std::string resultXml;      // unescaped DIDL-Lite XML string
    uint32_t numberReturned;
    uint32_t totalMatches;
    uint32_t updateId;
};
```

`SoapBrowseResponseParser` 負責：
1. 解析 SOAP Envelope。
2. 確認是 `<u:BrowseResponse>`（不是 `<s:Fault>`）。
3. 提取 `<Result>` 的文字內容（此時已由 XML parser 自動 unescape）。
4. 提取 `NumberReturned`, `TotalMatches`, `UpdateID`。
5. 回傳 `BrowseResult`（或丟 `SoapFaultException`）。

`DidlLiteParser` 負責：
1. 接受 unescaped DIDL-Lite XML string。
2. 解析所有 `<container>` 和 `<item>`。
3. 解析每個 `<item>` 的 `<res>` 元素。
4. 回傳 `std::vector<UpnpObject>`。

### 為何 `<Result>` 不需要手動 unescape？

標準 XML parser（pugixml, tinyxml2）在 parse SOAP 時，
會自動把 `&lt;DIDL-Lite&gt;` 轉換成 `<DIDL-Lite>` 作為 text content。
只需要讀取 `<Result>` 節點的 text content，再傳給 `DidlLiteParser`。

```cpp
// in SoapBrowseResponseParser
auto resultNode = doc.select_node("//Result");
std::string didlXml = resultNode.node().child_value(); // already unescaped
```

## Test Strategy

### SoapBrowseResponseParser 的 fixture

```xml
<!-- tests/fixtures/soap_responses/browse_root_response.xml -->
<s:Envelope xmlns:s="..." xmlns:u="...">
  <s:Body>
    <u:BrowseResponse>
      <Result>&lt;DIDL-Lite ...&gt;...&lt;/DIDL-Lite&gt;</Result>
      <NumberReturned>2</NumberReturned>
      <TotalMatches>2</TotalMatches>
      <UpdateID>0</UpdateID>
    </u:BrowseResponse>
  </s:Body>
</s:Envelope>
```

- 驗證 `BrowseResult.resultXml` 是有效的 DIDL-Lite XML（非 escaped）。
- 驗證 `numberReturned` 和 `totalMatches`。

### SoapFault fixture

```xml
<!-- tests/fixtures/soap_responses/soap_fault.xml -->
<s:Envelope ...>
  <s:Body>
    <s:Fault>
      <faultcode>s:Client</faultcode>
      <detail>
        <UPnPError>
          <errorCode>701</errorCode>
          <errorDescription>No such object</errorDescription>
        </UPnPError>
      </detail>
    </s:Fault>
  </s:Body>
</s:Envelope>
```

- 驗證 `SoapFaultException` 被丟出，`faultCode == 701`。

## Consequences

- `ContentDirectoryClient` 是 orchestrator，不做 XML parsing。
- `SoapBrowseResponseParser` 和 `DidlLiteParser` 各自獨立可測試。
- 因為用標準 XML parser 自動 unescape，不需要自己寫 HTML unescape。
- `BrowseResult` 的 `resultXml` 若為空字串，`DidlLiteParser` 應回傳空 vector，不丟 exception。
