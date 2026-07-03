# macOS Toolchain Smoke Test

本文件對應 `ADR-013` 的第一個驗證點：先確認本機僅用
Command Line Tools（CLT）與 CMake，是否能編譯最小的 ObjC++ Cocoa bundle。

## 目的

- 驗證 `enable_language(OBJCXX)` 在本機可用。
- 驗證 `add_library(... MODULE)` 可產出 macOS bundle。
- 驗證可連結 `Cocoa` framework。
- 刻意**不**引入 foobar2000 SDK，避免把 SDK 問題與 toolchain 問題混在一起。

## Target

- CMake option: `ENABLE_MACOS_BUNDLE_SMOKE_TEST=ON`
- CMake target: `foo_dms_browser_macos_smoke`
- Source: `component_macos/smoke/Fb2kSmokeComponent.mm`

此 target 只做一件事：編譯一個最小 ObjC++ 動態 bundle，並引用
`NSBundle` / `NSApplication` 以確認 Cocoa headers 與 runtime link 正常。

## 執行方式

```bash
cmake -B build-macos-smoke -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_MACOS_BUNDLE_SMOKE_TEST=ON
cmake --build build-macos-smoke --target foo_dms_browser_macos_smoke -j 8
```

若成功，輸出應位於：

```text
build-macos-smoke/component_macos/foo_dms_browser_macos_smoke.bundle
```

## 解讀

- 若 configure 或 compile 失敗，先懷疑本機 CLT / SDK 安裝，而不是 foobar2000 SDK。
- 若此 smoke test 成功，下一步才有資格導入 foobar2000 macOS SDK。
- 若後續 SDK 無法在 CMake 下編譯，依 `ADR-013` 啟動 fallback，而不是回頭質疑 CLT 是否能編 ObjC++。
