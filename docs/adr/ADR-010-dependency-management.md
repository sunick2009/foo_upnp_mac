# ADR-010: CMake Dependency Management

**狀態：** Accepted  
**日期：** 2026-07-03

## Context

Phase 0 需要以下 C++ dependencies：

| Dependency | 用途 |
|---|---|
| libcurl | HTTP client |
| pugixml | XML parsing |
| test framework (Catch2 / gtest) | unit tests |

選項：
1. **FetchContent** — CMake 在 build 時自動下載
2. **Homebrew + find_package** — 系統已裝，CMake 尋找
3. **git submodule** — 直接在 repo 裡
4. **vcpkg / conan** — package manager，需要額外 setup

## Decision

**混合策略：**
- **libcurl**: `find_package(CURL REQUIRED)` → Homebrew（ADR-009）
- **pugixml**: CMake `FetchContent` — header-only 或小型 library，FetchContent 最乾淨
- **Catch2** (test framework): CMake `FetchContent`

理由：
- libcurl 有 native deps，自己 build 麻煩，用 Homebrew 版本最可靠。
- pugixml 是 single-header 或兩個檔案，FetchContent 不會下載太多東西，且固定版本。
- Catch2 透過 FetchContent 固定版本，CI 與開發者環境一致，不依賴 Homebrew 版本。

### CMake 設定（概念）

```cmake
include(FetchContent)

# pugixml
FetchContent_Declare(
    pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG        v1.14
)
FetchContent_MakeAvailable(pugixml)

# Catch2
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.6.0
)
FetchContent_MakeAvailable(Catch2)

# libcurl (via Homebrew)
find_package(CURL REQUIRED)
```

## 關於 pugixml vs tinyxml2

**選 pugixml。**

| | pugixml | tinyxml2 |
|---|---|---|
| XPath 支援 | Yes | No |
| 速度 | 較快 | 較慢 |
| API 設計 | 較完整 | 較簡單 |
| 維護狀況 | 活躍 | 較少更新 |

SOAP response 和 DIDL-Lite 都有複雜的 namespace，XPath 很有用。

→ 這個決策也記錄在此 ADR。

## Consequences

- 首次 `cmake --build` 會下載 pugixml 和 Catch2（需要網路）。
- 若要離線 build，需要先跑過一次讓 CMake cache FetchContent。
- `CONTRIBUTING.md` 說明：只需要 `brew install curl`，其他 deps 自動下載。
- CI pipeline 需要 git 和網路連線。
- 所有 dependency 的版本都在 CMakeLists.txt 裡固定（GIT_TAG pinned）。
