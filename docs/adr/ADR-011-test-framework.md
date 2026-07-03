# ADR-011: Test Framework 選擇

**狀態：** Accepted  
**日期：** 2026-07-03

## Context

需要一個 C++ test framework 來跑 fixture-based parser tests 和 unit tests。

| 選項 | 優點 | 缺點 |
|---|---|---|
| **Catch2 v3** | header-friendly，BDD 風格，FetchContent 容易，active | 需要 v3（v2 API 不同）|
| **Google Test** | 業界最廣泛，豐富 matcher | 需要 Google Mock 才有 mock，build 較慢 |
| **doctest** | 最輕量，single-header | 功能較少，社群較小 |
| **自己寫** | 零依賴 | 維護負擔，不值得 |

## Decision

**選 Catch2 v3。**

理由：
- `TEST_CASE` 和 `SECTION` 的 fixture 風格適合 parser tests。
- `REQUIRE`, `CHECK`, `REQUIRE_THROWS_AS` 足夠清楚。
- FetchContent 整合乾淨（ADR-010）。
- 若日後需要 mock（例如 mock `HttpClient`），可以自己用 virtual interface，不需要 Google Mock。

### 測試結構規範

```cpp
// tests/parser/test_didl_lite_parser.cpp
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include "upnp/DidlLiteParser.hpp"

TEST_CASE("DidlLiteParser parses container", "[parser][didl]") {
    std::ifstream f("fixtures/didl_lite/containers.xml");
    REQUIRE(f.is_open());
    std::string xml((std::istreambuf_iterator<char>(f)), {});

    auto objects = upnp::DidlLiteParser::parse(xml);
    REQUIRE(objects.size() == 2);
    CHECK(objects[0].type == upnp::UpnpObjectType::Container);
    CHECK(objects[0].title == "Music");
    CHECK(objects[0].childCount == 120);
}
```

### 測試目錄結構

```text
tests/
  CMakeLists.txt
  fixtures/
    device_descriptions/
    soap_responses/
    didl_lite/
  parser/
    test_device_description_parser.cpp
    test_soap_browse_parser.cpp
    test_didl_lite_parser.cpp
    test_url_resolver.cpp
    test_resource_selector.cpp
  integration/
    test_content_directory_client.cpp  (mock HTTP server)
```

### Fixture 路徑設定

測試時 fixture 路徑需要可設定，不能寫死 absolute path：

```cmake
# tests/CMakeLists.txt
target_compile_definitions(upnp_tests PRIVATE
    FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)
```

```cpp
// 在 test 裡使用
std::string fixturePath = std::string(FIXTURE_DIR) + "/didl_lite/containers.xml";
```

## Consequences

- `tests/CMakeLists.txt` 使用 `FetchContent` 取得 Catch2 v3。
- 所有 test target 加上 Catch2 main（`Catch2::Catch2WithMain`）。
- `FIXTURE_DIR` macro 讓 CI 和本機都能找到 fixture 檔案。
- Mock HTTP server 的方案在 integration test ADR 裡另行決定（Phase 0 先不做）。
