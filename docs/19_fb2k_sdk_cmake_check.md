# foobar2000 SDK CMake 編譯驗證（ADR-013 風險驗證點）

**日期：** 2026-07-03
**結論：✅ 通過（含 runtime 載入）** — foobar2000 SDK (SDK-2025-03-07)
可在 CMake + CLT（無完整 Xcode）下完整編譯並 link 成 loadable
component bundle，且真實的 foobar2000 v2.25.8 (macOS) 能載入並執行它。
ADR-013 的 fallback 條件**沒有觸發**，Option B（CMake 全程）成立。

## 驗證了什麼

- SDK 五個 Xcode target 的原始碼全部以 CMake static lib 編譯成功：
  `fb2k_pfc`（35 檔）、`fb2k_sdk`（65 檔）、`fb2k_shared`（5 檔）、
  `fb2k_sdk_helpers`（33 檔）、`fb2k_component_client`（1 檔）。
- 檔案清單、prefix header、C++ 標準（gnu++17 / client 為 gnu++20）、
  ARC、header search paths（`..`、`../..`）皆從 SDK 內附的
  `.xcodeproj` 逐一移植，見 `component_macos/fb2k_sdk.cmake`。
- Link 驗證：`foo_dms_sdk_check.component` bundle 仿照
  `foo_sample.xcodeproj` 的 link line（五個 static lib + Cocoa）
  成功產出，`nm` 確認匯出 `_foobar2000_get_interface` 入口符號。
- Ad-hoc code signing（`codesign -s -`）對產出的 `.component` 可用。
- 預設 build 與 36 個既有測試不受影響（option 預設 OFF）。

## 重現方式

```bash
# 1. 取得 SDK（不要 commit 進 repo，路徑已在 .gitignore）
mkdir -p third_party/fb2k_sdk && cd third_party/fb2k_sdk
curl -sSLO https://www.foobar2000.org/downloads/SDK-2025-03-07.7z
tar -xf SDK-2025-03-07.7z   # macOS bsdtar 可直接解 7z
cd ../..

# 2. 編譯驗證 target
cmake -B build-sdk-check -DCMAKE_BUILD_TYPE=Debug -DENABLE_FB2K_SDK_CHECK=ON
cmake --build build-sdk-check --target foo_dms_sdk_check -j 8
```

成功輸出：
`build-sdk-check/component_macos/foo_dms_sdk_check.component`

SDK 路徑可用 `-DFB2K_SDK_ROOT=/path/to/sdk` 覆寫。

## 移植時對齊的 Xcode 設定

| Xcode 設定 | CMake 對應 |
|---|---|
| `CLANG_CXX_LANGUAGE_STANDARD = gnu++17/20` | `CXX_STANDARD` + `CXX_EXTENSIONS ON` |
| `GCC_PREFIX_HEADER`（每個 target 不同） | `-include <header>` compile option |
| `CLANG_ENABLE_OBJC_ARC = YES` | `-fobjc-arc`（僅 OBJCXX 檔） |
| `GCC_PREPROCESSOR_DEFINITIONS = DEBUG=1`（Debug） | `$<$<CONFIG:Debug>:DEBUG=1>` |
| `HEADER_SEARCH_PATHS = .., ../..` | include dirs：SDK root 與 `foobar2000/` |
| `MACOSX_DEPLOYMENT_TARGET = 11.0` | 尚未固定（見下方注意事項） |

## 注意事項與後續

- **Deployment target 未固定**：目前用本機預設。發佈前應設
  `CMAKE_OSX_DEPLOYMENT_TARGET=11.0` 對齊 SDK，並考慮 universal
  binary（`CMAKE_OSX_ARCHITECTURES=arm64;x86_64`）——留給 M6 packaging。
- `foo_dms_sdk_check` 是拋棄式驗證 target；正式 component 會是
  獨立 target，link `upnp_core` + SDK libs。
- **Runtime 載入已驗證（2026-07-03）**：component 含一個 `initquit`
  service，`on_init` 時寫 marker 檔（configure 時以
  `-DFB2K_SDK_CHECK_MARKER_FILE=<path>` 指定）。把 ad-hoc 簽章後的
  bundle 放到 `~/Library/foobar2000-v2/user-components/`（扁平放置，
  執行檔以 `foo_*.component` glob 掃描），重啟 foobar2000 v2.25.8
  後 marker 檔出現，內容含 fb2k 版本字串。載入後也可在
  Preferences → Components 看到「DMS Browser SDK Check」。
- SDK 授權見 `third_party/fb2k_sdk/sdk-license.txt`（隨 SDK 解壓取得）。
