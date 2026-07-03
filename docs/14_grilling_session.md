# Grilling Session: Phase 0 Design Review

## 目的

這份文件是針對 `foo_dms_browser_mac` Phase 0 設計的 grilling session 紀錄。
每個問題都是真正需要決策的設計點，不是可以「之後再說」的事。
每個決策會產生對應的 ADR。

---

## Round 1：Domain Model

### Q1. `UpnpObject` 是 Container + AudioItem 的 union，還是分開的型別？

**問題所在：**
目前設計用 `UpnpObjectType enum + optional<string>` 的 union struct。
Container 不會有 `resources`，AudioItem 不會有 `childCount`。
你現在是混在一起，還是分開？

**風險：**
若用 union struct，呼叫端必須先 check type 才能存取欄位，容易犯錯。
若用 variant / 繼承，API 更清楚但稍複雜。

→ **ADR-001** 決策所在。

---

### Q2. `UpnpResource` 的 `mimeType` 和 `protocolInfo` 是兩個欄位還是一個？

**問題所在：**
UPnP `protocolInfo` 的格式是 `<protocol>:<network>:<contentFormat>:<additionalInfo>`，
例如 `http-get:*:audio/flac:DLNA.ORG_PN=FLAC;DLNA.ORG_OP=01`。
`mimeType` 是從 `protocolInfo` 的第三個欄位解析出來的。

目前你有兩個獨立欄位。這表示：
1. 誰來解析 `protocolInfo`？`DidlLiteParser`？還是 `UpnpResource` constructor？
2. 若 `protocolInfo` 缺失，`mimeType` 如何處理？

→ **ADR-002** 決策所在。

---

### Q3. 「Browse」這個詞有三個不同意思，你混用了嗎？

1. UPnP SOAP action: `Browse` (ContentDirectory action)
2. CLI command: `upnp-browser-cli browse`
3. UI action: 使用者在 panel 裡瀏覽

若 `ContentDirectoryClient::Browse()` 這個方法名稱是 UPnP SOAP action，
`BrowseFlag` 是 UPnP 的 `BrowseDirectChildren` 或 `BrowseMetadata`，
那 CLI 的 `browse` subcommand 就是 wrapper。
**你需要在 code 和文件裡統一大小寫規則。**

→ **ADR-003** 決策所在（命名規範）。

---

### Q4. `UrlResolver` 的 input 是什麼？

目前文件說「`rootDesc.xml` URL 與 relative `controlURL` 合成 absolute URL，支援 `URLBase`」。

但 UPnP 的 `URLBase` 是選用欄位，且在 UPnP 2.0 已廢棄。
現實中常見情況：

```text
rootDesc.xml: http://192.168.1.10:8200/rootDesc.xml
controlURL:   /upnp/control/content_directory   ← root-relative
URLBase:      (absent)
→ 正確 absolute URL: http://192.168.1.10:8200/upnp/control/content_directory

另一台 server:
rootDesc.xml: http://192.168.1.20:32469/DeviceDescription.xml
controlURL:   http://192.168.1.20:32469/ContentDirectory/Control  ← 已是 absolute
URLBase:      (absent)
→ 直接使用 controlURL
```

**你的 `UrlResolver` 的 API 長什麼樣？**
必須能處理：absolute controlURL、root-relative controlURL、URLBase 覆蓋。

→ **ADR-004** 決策所在。

---

## Round 2：Error Handling

### Q5. 你的 error model 是什麼？

文件說「網路錯誤必須有可診斷錯誤訊息」，但沒有說怎麼傳遞。

選項：
1. `std::exception` hierarchy
2. `std::expected<T, Error>` (C++23 / 自己實作)
3. error code + out param
4. 混用：exception for 致命錯誤，expected for expected failures

這個決策影響：
- `HttpClient` 的回傳型別
- `DeviceDescriptionParser::parse()` 的回傳型別
- CLI 的錯誤印出

→ **ADR-005** 決策所在。

---

### Q6. SOAP Fault 是 error 還是 normal response？

ContentDirectory Browse 可以回傳 SOAP Fault，例如：
- `401 Invalid Action`
- `402 Invalid Args`
- `501 Action Failed`
- `701 No such object`
- `720 Cannot process the request`

這是 HTTP 200 + 含有 `<s:Fault>` 的 SOAP body，不是 HTTP error。
`SoapBrowseResponseParser` 要怎麼處理這個？
它回傳 `Error`？還是丟 exception？還是有一個 `SoapResult<T>` wrapper？

→ **ADR-005** 延伸。

---

## Round 3：Core Library 邊界

### Q7. `SoapBrowseResponseParser` 解析 DIDL-Lite 的哪一層？

UPnP Browse response 的結構：

```xml
<!-- SOAP Envelope -->
<s:Envelope>
  <s:Body>
    <u:BrowseResponse>
      <Result>                          ← 這是 XML-escaped string
        &lt;DIDL-Lite&gt;...&lt;/DIDL-Lite&gt;
      </Result>
      <NumberReturned>10</NumberReturned>
      <TotalMatches>120</TotalMatches>
    </u:BrowseResponse>
  </s:Body>
</s:Envelope>
```

`Result` 欄位是一個 XML-escaped 的 DIDL-Lite XML 字串，需要先 unescape 再 parse。

**問題：**
- `SoapBrowseResponseParser` 只解析到取出 `Result` 字串為止？
- 還是它直接呼叫 `DidlLiteParser` 並回傳 `vector<UpnpObject>`？
- 誰負責兩層 XML parse 的 orchestration？`ContentDirectoryClient`？

→ **ADR-006** 決策所在。

---

### Q8. `ResourceSelector` 在哪裡？

`tests/09_testing_strategy.md` 的 unit test 表格裡有 `ResourceSelector`，
但 `05_core_library_design.md` 的 module 清單裡沒有。

**這是遺漏的 module，還是打算嵌在 `DidlLiteParser` 裡？**

Audio item 可能有多個 `<res>` (FLAC + MP3 + transcoded)。
誰來挑最好的那個？優先順序是什麼？

→ **ADR-007** 決策所在。

---

## Round 4：HTTP Layer

### Q9. 同步 vs 非同步？

目前文件沒有說 `HttpClient` 是同步還是非同步。

Phase 0 CLI：同步是合理的，簡單。
Phase 1 component：若呼叫 Browse 要等 2-3 秒，UI 會凍結。

**你的 `HttpClient` 是同步的 blocking call，還是 callback/future-based？**

這個決策現在就影響 interface，不能留到 Phase 1 才決定。

→ **ADR-008** 決策所在。

---

### Q10. libcurl vs Apple URLSession，誰來做 HTTP？

文件說「libcurl 或輕量 HTTP wrapper」，沒有決定。

| | libcurl | Apple URLSession |
|---|---|---|
| macOS native | No | Yes |
| 跨平台 | Yes | No |
| async | callback-based | async/await (Swift) / delegate (ObjC) |
| CLI 整合 | 直接用 | 需要 run loop |
| component 整合 | 額外 dep | 可直接用 |
| CA bundle | 需要設定 | 系統自動 |
| HTTPS | Yes | Yes |
| Phase 0 可用 | Yes | 需要 run loop 或 semaphore |

Phase 0 CLI 選 libcurl 比較直接。
Phase 1 可以換，但若 core library 已寫成 `HttpClient` abstraction，換起來容易。

→ **ADR-009** 決策所在。

---

## Round 5：Build & Dependencies

### Q11. 怎麼取得 pugixml / libcurl？

目前 CMakeLists.txt 沒有 dependency management。

選項：
- FetchContent (CMake 3.11+，已要求 3.20)
- Homebrew + find_package
- git submodule
- vcpkg / conan

**你要用哪個？這影響 CI 設定和貢獻者的 onboarding。**

→ **ADR-010** 決策所在。

---

### Q12. Test framework 是什麼？

`tests/CMakeLists.txt` 目前是空的 skeleton。
你說「fixture-based tests」，但沒說用哪個 framework：

- Google Test (gtest)
- Catch2
- doctest
- 自己寫

→ **ADR-011** 決策所在。

---

## Round 6：Phase 邊界

### Q13. Phase 0 的「完成」定義是什麼？

文件說「Phase 0 驗收標準：可對至少一台真實 UPnP server 執行 browse」。

但需要具體說明：
1. 要用哪台真實 server 測試？(MiniDLNA? Jellyfin?)
2. 驗收測試是手動跑 CLI 還是自動化？
3. fixture tests 有幾個才夠？

→ 這不需要 ADR，但需要在 issue 裡定義 acceptance criteria。

---

### Q14. Pagination 怎麼處理？

Browse 的 `StartingIndex` 和 `RequestedCount` 是 UPnP 標準的 pagination。
很多 server 的 `TotalMatches` 比 `NumberReturned` 大。

**Phase 0 CLI 只拿第一頁，還是自動翻頁？**

自動翻頁涉及 loop + 多次 HTTP request，在 Phase 0 可能不值得做。
但 `ContentDirectoryClient` 的 API 要能支援 pagination。

→ **ADR-012** 決策所在。

---

## 開放問題清單

這些問題是 grilling 後剩下的未決事項，需要在下一次 session 或 issue 裡處理：

| # | 問題 | 重要程度 | 影響 |
|---|---|---|---|
| O1 | 有沒有 UPnP server 會要求 HTTP authentication？要支援嗎？ | 中 | HttpClient API |
| O2 | User-Agent header 要設定成什麼？某些 server 會 block 不認識的 UA | 低 | HttpClient |
| O3 | `albumArtURI` 若是 relative URL，誰解析？ | 低 | DidlLiteParser / UrlResolver |
| O4 | DLNA-specific `res@protocolInfo` additional info (DLNA.ORG_PN) 要解析嗎？ | 低 | UpnpResource |
| O5 | ContentDirectory 1 vs 2 vs 3：只支援 v1 嗎？ | 中 | DeviceDescriptionParser |
| O6 | Jellyfin/Plex 的 SOAP Browse endpoint 是否需要任何 header workaround？ | 中 | 相容性測試 |
