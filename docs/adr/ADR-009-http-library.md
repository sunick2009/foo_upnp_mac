# ADR-009: HTTP 實作 — libcurl vs Apple URLSession

**狀態：** Accepted  
**日期：** 2026-07-03

## Context

`HttpClient` 需要一個 HTTP 實作。兩個主要候選：

| 比較項目 | libcurl | Apple URLSession |
|---|---|---|
| macOS native | No（第三方 dep） | Yes（系統框架） |
| 跨平台 | Yes（可測試 Linux CI）| No |
| Phase 0 CLI 使用 | 直接 blocking API | 需要 run loop 或 dispatch semaphore |
| C++ 整合 | 直接 include | 需要 Objective-C++ 或 C wrapper |
| CA bundle / TLS | 需要設定 | 系統自動，支援 Keychain |
| Homebrew 可用 | Yes（`brew install curl`）| 系統內建 |
| 靜態鏈結 | 可以 | 不可以（動態 framework）|
| dependency 管理 | FetchContent 或 brew | N/A |

## Decision

**Phase 0 使用 libcurl。**

理由：
- Phase 0 CLI 不需要 run loop，libcurl synchronous API (`curl_easy_perform`) 最直接。
- `HttpClient` 設計為 C++ class，不需要 Objective-C++ runtime。
- CI 可以跑 Linux（GHA Ubuntu runner + libcurl）。
- 若未來 Phase 1 需要 URLSession（例如系統 proxy 設定、Keychain），
  可以透過 `HttpClient` abstraction 換掉實作。

### libcurl 取得方式

**CMake `find_package(CURL REQUIRED)`，搭配 Homebrew pre-installed。**

```cmake
find_package(CURL REQUIRED)
target_link_libraries(upnp_core PRIVATE CURL::libcurl)
```

開發者 setup：
```bash
brew install curl
# macOS 系統已有 /usr/bin/curl，但 Homebrew 版本較新
```

CI：
```yaml
# .github/workflows 或 GitLab CI
- run: brew install curl  # macOS runner
```

不使用 FetchContent 下載 libcurl source，
因為 libcurl 有許多 native deps（OpenSSL, zlib, etc.）自行 build 過於複雜。

## Consequences

- `CMakeLists.txt` 加入 `find_package(CURL REQUIRED)`。
- `HttpClient.cpp` 使用 `curl_easy_*` API。
- `HttpClient` 需要在 constructor 裡 `curl_global_init(CURL_GLOBAL_ALL)`（或在 main 裡做一次）。
- 開發者需要先 `brew install curl` 才能 build。
- 這個決策在 `CONTRIBUTING.md` 裡說明。

## Phase 1 換 URLSession 的觸發條件

若出現以下情況，考慮改用 URLSession：
- 需要系統 proxy 自動偵測。
- 需要 Keychain 整合（server authentication）。
- libcurl 的 TLS 設定與 macOS 系統 CA 不一致。
