# Domain Glossary

本 glossary 定義本專案使用的術語，以及與 UPnP / DLNA 規範術語的對應關係。
如有歧義，本 glossary 的定義優先於口語用法。

---

## A

### AudioItem
DIDL-Lite `<item>` 元素，`upnp:class` 以 `object.item.audioItem` 開頭。
在本專案的 `UpnpObject` model 裡，type == `UpnpObjectType::AudioItem`。

---

## B

### Browse（大寫 B）
UPnP ContentDirectory 規範定義的 SOAP action，用於列出指定 ObjectID 的子項目或取得 metadata。
參見：`BrowseFlag`, `BrowseDirectChildren`, `BrowseMetadata`。

### BrowseDirectChildren
`Browse` action 的 BrowseFlag 值，表示「列出指定 ObjectID 的直接子項目」。

### BrowseFlag
`Browse` action 的參數，值為 `BrowseDirectChildren` 或 `BrowseMetadata`。

### BrowseMetadata
`Browse` action 的 BrowseFlag 值，表示「取得指定 ObjectID 本身的 metadata」。

### BrowseParams
本專案 C++ struct，封裝 UPnP Browse action 所需的所有參數。
對應 UPnP 規範的 `Browse` in arguments。

### BrowseResponse
本專案 C++ struct，封裝 UPnP Browse action 的回傳資料（已解析後）。
包含 `vector<UpnpObject>`, `numberReturned`, `totalMatches`, `updateId`。

### BrowseResult
本專案內部中間型別，由 `SoapBrowseResponseParser` 回傳。
包含 unescape 後的 DIDL-Lite XML string 及數量欄位。
進一步解析後由 `DidlLiteParser` 轉換為 `BrowseResponse`。

---

## C

### Container
DIDL-Lite `<container>` 元素，代表目錄、播放清單、分類等可以包含子項目的節點。
必有 `childCount` 屬性。在本專案 model 裡，type == `UpnpObjectType::Container`。

### ContentDirectory
UPnP 服務類型：`urn:schemas-upnp-org:service:ContentDirectory:1`。
提供 `Browse`, `Search` 等 action，讓 Control Point 瀏覽 Media Server 的媒體目錄。

### ContentDirectoryClient
本專案 C++ class，負責呼叫 ContentDirectory SOAP actions（Browse, BrowseMetadata）。
是 orchestrator：呼叫 `HttpClient`，再呼叫 `SoapBrowseResponseParser` 和 `DidlLiteParser`。

### controlURL
Device description XML 裡 `<service>` 元素的 `<controlURL>`，
是呼叫 SOAP action 的 HTTP endpoint path（relative 或 absolute）。
必須透過 `UrlResolver` 轉換為 absolute URL 才能使用。

### Control Point
UPnP 術語，指能夠探索和控制 UPnP 裝置的客戶端。
本專案的 foobar2000 component 是一個 Control Point（只做 browse，不做 renderer control）。

---

## D

### Device Description
UPnP 裝置描述 XML 文件，通常放在 `rootDesc.xml`。
描述裝置的服務清單、製造商資訊、device type 等。
本專案只關心其中的 ContentDirectory service 資訊。

### DeviceDescriptionParser
本專案 C++ class，解析 device description XML，找到 ContentDirectory service，
回傳 `controlURL`, `eventSubURL`, `SCPDURL`, `URLBase`。

### DIDL-Lite
Digital Item Declaration Language — Lite。
DLNA/UPnP ContentDirectory Browse 回傳的 XML 格式，
描述 container 和 item 的 metadata 及 resource URL。

### DidlLiteParser
本專案 C++ class，解析 DIDL-Lite XML string，
回傳 `std::vector<UpnpObject>`。

### DLNA
Digital Living Network Alliance。建立在 UPnP 之上的互通性規範。
本專案以 UPnP ContentDirectory 為核心，DLNA 的 protocolInfo 格式是其延伸。

### DMS (Digital Media Server)
UPnP MediaServer 裝置，提供媒體內容供 Control Point 瀏覽和播放。
本專案名稱中的「DMS Browser」指「瀏覽 DMS 上的內容」。

---

## E

### effectiveBase
`UrlResolver` 的內部概念，是解析 relative `controlURL` 的 base URL。
若 device description 有 `<URLBase>` → effectiveBase = URLBase。
否則 → effectiveBase = rootDesc.xml URL 的 scheme+host+port。

---

## H

### HttpClient
本專案 C++ class，封裝 libcurl 的 blocking HTTP GET 和 POST。
提供 timeout, User-Agent, 自訂 header 支援。

### HttpException
本專案 exception class，當 `HttpClient` 遭遇網路錯誤或 timeout 時丟出。
HTTP 4xx / 5xx 不丟 exception，回傳 `Response` struct 讓呼叫端決定。

---

## M

### mimeType
本專案術語，指 `<res protocolInfo="http-get:*:<mimeType>:...">` 的第三個欄位，
例如 `audio/flac`, `audio/mpeg`。
由 `DidlLiteParser` 從 `protocolInfo` 解析後存入 `UpnpResource.mimeType`。

### MediaServer
UPnP 裝置角色，提供媒體 ContentDirectory 供 Control Point 存取。
**第一版不做 MediaServer 角色**（本專案是 browser，不是 server）。

---

## N

### NumberReturned
UPnP Browse action 的 out argument，本次回傳的項目數量。
≤ TotalMatches，≤ RequestedCount。

---

## O

### ObjectID
UPnP ContentDirectory 裡每個 container 和 item 的唯一識別碼（字串）。
根目錄的 ObjectID 通常是 `"0"`。

---

## P

### protocolInfo
DIDL-Lite `<res>` 的屬性，格式：`<protocol>:<network>:<contentFormat>:<additionalInfo>`。
例如：`http-get:*:audio/flac:DLNA.ORG_PN=FLAC;DLNA.ORG_OP=01`。
本專案只解析前三個欄位，`additionalInfo` 保留為原始字串。

---

## R

### RequestedCount
Browse action 的 in argument，請求的最大項目數量。
0 = 由 server 自行決定（可能回傳全部）。

### res（resource）
DIDL-Lite `<res>` 元素，描述 item 的一個可播放資源（URL + format info）。
一個 item 可以有多個 res（不同格式）。
在本專案 model 裡對應 `UpnpResource` struct。

### ResourceSelector
本專案 utility function（`upnp::selectBestResource()`），
從 `UpnpObject.resources` 裡選出最適合播放的 `UpnpResource`。
選擇依據：只考慮 `http-get` protocol，按 mimeType 優先順序排序。

### rootDesc.xml
UPnP MediaServer 的根裝置描述文件 URL。
本專案使用者手動輸入此 URL 作為 server 的入口點。
URL 的路徑名不一定是 `rootDesc.xml`（每台 server 不同），但功能一樣。

---

## S

### SCPDURL
Service Control Protocol Description URL。
Device description 裡 `<service>` 元素的子元素，指向 service 的 SCPD XML（描述 action 清單）。
本專案不需要讀取 SCPD，只需要 `controlURL`。

### SOAP
Simple Object Access Protocol。UPnP action 呼叫使用的 HTTP+XML 協定。
POST 到 controlURL，body 是 SOAP Envelope XML，header 有 `SOAPAction`。

### SoapBrowseRequestBuilder
本專案 C++ class，建構 Browse SOAP request 的 XML body 和 HTTP headers。

### SoapBrowseResponseParser
本專案 C++ class，解析 Browse SOAP response body，
回傳 `BrowseResult`（包含 DIDL-Lite XML string）或丟 `SoapFaultException`。

### SoapFaultException
本專案 exception class，當 SOAP response 包含 `<s:Fault>` 時丟出。
包含 `faultCode`（UPnP error code, e.g. 701）和 `faultString`（描述）。

### StartingIndex
Browse action 的 in argument，pagination 起始位置（0-based）。

---

## T

### TotalMatches
UPnP Browse action 的 out argument，container 裡的總項目數量。
可能大於 NumberReturned（需要翻頁才能拿到全部）。

---

## U

### UpdateID
UPnP Browse action 的 out argument，container 的版本號。
若 server 上的 container 有更新，UpdateID 會增加。
本專案 Phase 0 記錄但不使用。

### UpnpException
本專案所有 exception 的 base class，繼承 `std::runtime_error`。

### UpnpObject
本專案 C++ struct，代表 DIDL-Lite 裡的一個 container 或 item。
包含共用欄位（id, parentId, title）和 type-specific 欄位（childCount, resources 等）。

### UpnpObjectType
本專案 enum，值為 `Container`, `AudioItem`, `Unknown`。

### UpnpResource
本專案 C++ struct，代表 `<res>` 元素（一個可播放資源）。
包含 `url`, `protocolInfo`, `mimeType`, `duration`, `size`, `bitrate`。

### UrlResolver
本專案 C++ class，將 device description 裡的 relative `controlURL` 解析為 absolute URL。
支援 `URLBase`、root-relative path、relative path、absolute URL 四種情況。

### URLBase
Device description XML 裡的可選元素，指定解析 relative URL 的 base。
在 UPnP 2.0 已廢棄，但許多舊 server（MiniDLNA, Plex）仍使用。
本專案 `UrlResolver` 支援，但若缺失則從 rootDesc.xml URL 推算。

---

## X

### XmlParseException
本專案 exception class，當 XML 解析失敗（格式錯誤、缺少必要節點）時丟出。
