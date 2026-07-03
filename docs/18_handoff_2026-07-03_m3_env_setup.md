# Handoff: M3 environment setup after ADR-013 smoke test

**日期**: 2026-07-03  
**Repo**: `/Users/susu/Code/foo_upnp_mac`  
**交接對象**: 下一個 coding agent (`codex`)  
**本次焦點**: 把 M3 的本機建置前提整理到可直接開始導入 foobar2000 macOS SDK 與後續 component 開發。

## 一句話狀態

`CLT + CMake` 編譯最小 macOS `ObjC++` bundle 已在本機成功驗證。
下一步不應再重做 toolchain 探索，而應直接驗證 foobar2000 macOS SDK 是否能在同一條 CMake 路徑下編譯。

## 本次完成事項

- 讀過並採納 `docs/adr/ADR-013-macos-build-without-xcode.md` 的建置策略。
- 讀過 `docs/06_foobar2000_component_plan.md`，並修正文內殘留的 `.xcodeproj` 預設假設。
- 在 root `CMakeLists.txt` 新增可選開關 `ENABLE_MACOS_BUNDLE_SMOKE_TEST`。
- 新增 `component_macos/` 目錄與最小 smoke target：
  - `component_macos/CMakeLists.txt`
  - `component_macos/smoke/Fb2kSmokeComponent.mm`
- 新增文件 `docs/17_macos_toolchain_smoke_test.md`，說明 smoke test 目的、執行方式與解讀。
- 更新 `README.md`，讓後續 agent 知道 M3 前有獨立的 macOS toolchain smoke test。

## 實測結果

本機環境：

- `xcode-select -p` => `/Library/Developer/CommandLineTools`
- `clang --version` => Apple clang 17.0.0

已成功執行：

```bash
cmake -B build-macos-smoke -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_MACOS_BUNDLE_SMOKE_TEST=ON
cmake --build build-macos-smoke --target foo_dms_browser_macos_smoke -j 8
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j 8
ctest --test-dir build --output-on-failure
```

結果：

- CMake 可啟用 `OBJCXX`
- 可連結 `Cocoa`
- 可產出 `.bundle`
- smoke target 成功編譯
- 既有預設 build 未受影響
- 36/36 tests 通過

## 重要結論

- 「本機只有 CLT，能不能編 ObjC++ macOS bundle」這個問題已經有答案：**可以**。
- 因此，若後續導入 foobar2000 SDK 失敗，應先懷疑 SDK 本身的編譯設定、headers、linkage、project 假設，而不是回頭質疑 CLT / CMake 是否支援 ObjC++。
- `docs/06` 仍是功能規劃文件，不是最新建置策略來源。建置策略請以 `ADR-013` 為準。

## 下一個 agent 應直接做的事

1. 取得 foobar2000 macOS SDK，但不要 commit 進 repo。
2. 在 `component_macos/` 下建立最小 SDK 編譯驗證 target。
3. 驗證 SDK 是否能在 `CMake + CLT` 下成功編譯，先不要急著做完整 UI。
4. 若 SDK 可編譯，再整理 SDK 取得方式與 build 前提到文件，例如 `CONTRIBUTING` 或新的 setup doc。
5. 若 SDK 不可編譯，精確記錄卡點，依 `ADR-013` 判斷是否要進入 fallback。

## 不建議下一個 agent 重做的事

- 不要重新研究是否必須安裝完整 Xcode，這個問題在目前機器上沒有證據支持。
- 不要直接跳去做 `PreferencesPanel` 或 `BrowserPanel`，如果 SDK 連最小 target 都無法編譯，UI 工作會建立在錯誤前提上。
- 不要把 smoke test 與 SDK 驗證混成同一個問題。兩者的失敗面不同，混在一起只會模糊診斷。

## 相關檔案

- `docs/adr/ADR-013-macos-build-without-xcode.md`
- `docs/06_foobar2000_component_plan.md`
- `docs/17_macos_toolchain_smoke_test.md`
- `CMakeLists.txt`
- `component_macos/CMakeLists.txt`
- `component_macos/smoke/Fb2kSmokeComponent.mm`
- `README.md`

## Working tree 注意事項

目前工作樹包含以下與本次任務相關的未提交變更：

- `CMakeLists.txt`
- `README.md`
- `docs/06_foobar2000_component_plan.md`
- `component_macos/`
- `docs/17_macos_toolchain_smoke_test.md`

另有一個既存未追蹤檔案：

- `docs/16_handoff_2026-07-03.md`

沒有替它做內容修改，但後續 agent 需要注意不要誤判為本次新產物。

## Suggested skills

- `/grill-with-docs`: 在正式導入 foobar2000 macOS SDK 前，把 component registration、bundle layout、測試策略問清楚並補 ADR。
- `/code-review`: 完成 SDK 接入或第一個 component target 後立即 review，避免在錯誤建置模型上累積技術債。
- `/handoff`: 若在 SDK 驗證中途遇到卡點或 context 變長，再次交接。
