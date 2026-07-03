# ADR-002: protocolInfo 解析策略

**狀態：** Accepted  
**日期：** 2026-07-03

## Context

DIDL-Lite `<res>` 元素的 `protocolInfo` 屬性格式為：

```
<protocol>:<network>:<contentFormat>:<additionalInfo>
```

例如：
```
http-get:*:audio/flac:DLNA.ORG_PN=FLAC;DLNA.ORG_OP=01;DLNA.ORG_FLAGS=01700000000000000000000000000000
http-get:*:audio/mpeg:*
```

目前 `UpnpResource` 有兩個獨立欄位 `mimeType` 和 `protocolInfo`，
但 `mimeType` 就是 `protocolInfo` 的第三個欄位，這造成資訊重複。

## Options

### Option A: 只存 protocolInfo，mimeType lazy-computed

```cpp
struct UpnpResource {
    std::string protocolInfo;  // raw: "http-get:*:audio/flac:*"
    // mimeType() 是 method，不是欄位
    std::string mimeType() const;
};
```

- 優點：沒有資訊重複；protocolInfo 是 source of truth。
- 缺點：每次存取 mimeType 都要 parse；若 protocolInfo 格式不標準，method 可能失敗。

### Option B: 存兩個欄位，DidlLiteParser 填入兩者

```cpp
struct UpnpResource {
    std::string protocolInfo;  // raw
    std::string mimeType;      // parsed from protocolInfo[2]，或空字串
};
```

- 優點：呼叫端直接存取 mimeType；protocolInfo 惡意格式只影響 parser，不影響呼叫端。
- 缺點：資訊重複，若 protocolInfo 缺失或 malformed，mimeType 可能是空字串，需要處理。

### Option C: 解析 protocolInfo 成結構

```cpp
struct ProtocolInfo {
    std::string protocol;        // "http-get"
    std::string network;         // "*"
    std::string contentFormat;   // "audio/flac" (= mimeType)
    std::string additionalInfo;  // "DLNA.ORG_PN=FLAC;..."
};
```

- 優點：最完整；DLNA.ORG_PN 等 additional info 可進一步解析。
- 缺點：Phase 0 用不到 additional info；過度設計。

## Decision

**選 Option B** — 存兩個欄位，`DidlLiteParser` 在 parse 時填入兩者。

理由：
- Phase 0 呼叫端只需要 mimeType（決定音訊格式）和 protocolInfo（原始值，debug 用）。
- Option C 的 DLNA additional info 解析超出 Phase 0 範圍（ADR 明確不做 DLNA profile）。
- Option A 的 lazy parsing 在呼叫端使用時容易忽略 parse 失敗。

實作規則：
- 若 protocolInfo 能正確 split，`mimeType = split[2]`。
- 若 protocolInfo 格式不符（沒有三個 `:`），`mimeType = ""` 並 log warning。
- 若 `<res>` 沒有 `protocolInfo` 屬性，兩個欄位都設為 `""`。

## Consequences

- `DidlLiteParser` 負責解析 protocolInfo 的前三個欄位。
- `ResourceSelector`（ADR-007）依據 mimeType 選最佳 resource，不需要 re-parse protocolInfo。
- 未來若需要 DLNA.ORG_PN，可以加一個 `protocolInfoAdditional` 欄位或改用 Option C。
