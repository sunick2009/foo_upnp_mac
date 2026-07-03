# ADR-004: UrlResolver API 設計

**狀態：** Accepted  
**日期：** 2026-07-03

## Context

UPnP device description 裡的 `controlURL` 可能是：

1. **Absolute URL**: `http://192.168.1.20:32469/ContentDirectory/Control`
2. **Root-relative path**: `/upnp/control/content_directory`
3. **Relative path**: `upnp/control` （少見但存在）

Base URL 的決定規則（依優先順序）：
1. 若 `<URLBase>` 存在且非空 → 用 URLBase。
2. 否則 → 用 rootDesc.xml URL 的 scheme+host+port（去掉 path）。

注意：`<URLBase>` 在 UPnP 2.0 已廢棄，但許多舊 server 仍使用。

## UrlResolver API

```cpp
class UrlResolver {
public:
    // Parse URLBase from device description (may be empty)
    // baseUrl: the URL used to fetch rootDesc.xml
    explicit UrlResolver(std::string rootDescUrl, std::string urlBase = "");

    // Resolve a controlURL relative to the rootDesc.xml base.
    // Returns absolute URL string.
    // Throws std::invalid_argument if input is empty or unparseable.
    std::string resolve(const std::string& controlUrl) const;

private:
    std::string baseUrl_; // effective base: URLBase or scheme+host+port of rootDesc
};
```

## 解析規則

```
Given: rootDescUrl = "http://192.168.1.10:8200/rootDesc.xml"
       urlBase = ""  (absent)

→ effectiveBase = "http://192.168.1.10:8200"

Case 1: controlUrl = "http://192.168.1.10:8200/cds/control"
→ absolute URL detected (starts with http:// or https://)
→ return as-is

Case 2: controlUrl = "/upnp/control/cds"
→ root-relative (starts with /)
→ effectiveBase + controlUrl = "http://192.168.1.10:8200/upnp/control/cds"

Case 3: controlUrl = "upnp/control/cds"
→ relative path
→ effectiveBase + "/" + controlUrl = "http://192.168.1.10:8200/upnp/control/cds"

Given: urlBase = "http://192.168.1.10:8200/upnp/"
→ effectiveBase = "http://192.168.1.10:8200/upnp/"
Case 2: controlUrl = "/cds/control"
→ effectiveBase = "http://192.168.1.10:8200" (only scheme+host+port from URLBase)
→ result = "http://192.168.1.10:8200/cds/control"
```

注意：RFC 3986 規定 root-relative URL 以 host 為 base，不以 URLBase path 為 base。

## Decision

實作 `UrlResolver` 遵循 RFC 3986 URL resolution 語義：
- Root-relative path (`/foo`) → scheme+host+port of effectiveBase + path。
- Relative path (`foo`) → effectiveBase + "/" + path（若 effectiveBase 沒有 trailing slash）。
- Absolute URL → 直接使用。

不自己寫 URL parser。使用 string operations 處理常見 case，
若需要完整 URL parsing，在 macOS 可用 `NSURL` 或 C++20 的 URL utilities（若可用）。

## Test Cases（必須覆蓋）

```
rootDesc.xml URL | URLBase | controlURL | expected result
http://a:8200/rootDesc.xml | (none) | /cds | http://a:8200/cds
http://a:8200/rootDesc.xml | (none) | http://b:9000/cds | http://b:9000/cds
http://a:8200/rootDesc.xml | http://a:8200/ | /cds | http://a:8200/cds
http://a:8200/path/rootDesc.xml | (none) | /cds | http://a:8200/cds
http://a:8200/rootDesc.xml | (none) | cds | http://a:8200/cds
```

## Consequences

- `DeviceDescriptionParser` 需要回傳 `URLBase`（空字串若缺失）。
- `ContentDirectoryClient` 在初始化時構造 `UrlResolver`。
- `UrlResolver` 的 fixture-based tests 必須包含以上所有 case。
