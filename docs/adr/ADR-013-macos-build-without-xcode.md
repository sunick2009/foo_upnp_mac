# ADR-013: macOS Component Build — 不依賴完整 Xcode

**狀態：** Accepted  
**日期：** 2026-07-03

## Context

原規劃（docs/06）是「Phase 2 建立 Xcode project」來編譯 foobar2000
macOS component。但有兩個現實限制：

1. **本地磁碟空間**：完整 Xcode 安裝後佔 40GB+。
   開發機已有 Command Line Tools（CLT，約 3GB，clang 17），
   Phase 0/1 的所有工作都是用 CLT + CMake 完成的。
2. **CI 環境**：需要一條不依賴本地環境的 component 編譯路徑。

關鍵事實：

- CLT 的 macOS SDK 已包含 AppKit / Cocoa / Foundation headers，
  clang 可直接編譯 Objective-C++。
- CMake 可以直接產出 macOS bundle（`add_library(... MODULE)` +
  bundle 屬性），不需要 `.xcodeproj`。
- 只有 `xcodebuild` 和 Xcode IDE 需要完整 Xcode；
  CMake + Ninja/Makefiles 不用。
- Ad-hoc code signing（`codesign -s -`）CLT 就能做。
- GitHub Actions 的 `macos-14` / `macos-15` runner 預裝完整 Xcode，
  public repo 免費不限分鐘數；GitLab SaaS macOS runner 需要 Premium tier。

## Options

### Option A: 依原計畫 — 本地安裝完整 Xcode + .xcodeproj
- 優點：與 foobar2000 SDK 官方範例的開發流程一致。
- 缺點：本地 40GB+；`.xcodeproj` 與現有 CMake build 分裂成兩套 build system。

### Option B: CMake 全程 — CLT 本地編譯 + CI 守門
- M3 的 component 繼續用 CMake 編譯（ObjC++ 源檔 + Cocoa framework link
  + bundle 輸出），本地只需 CLT。
- GitHub Actions macOS runner 作為 CI 守門員，每個 MR 驗證 component 可編譯。
- 優點：單一 build system；本地零額外磁碟成本；CI 可重現。
- 缺點：foobar2000 SDK 若硬性依賴 Xcode project template / xcconfig，
  需要手動移植編譯設定；偏離官方範例流程，遇到問題時社群參考較少。

### Option C: 純 CI 編譯（本地不編 component）
- 優點：本地完全不需要 macOS toolchain 考量。
- 缺點：迭代迴圈每次數分鐘；UI 開發不可行 — component 必須在本地
  foobar2000 內載入測試，光編譯過不代表能用。

## Decision

**選 Option B：CMake 全程，CLT 本地編譯，GitHub Actions macOS CI 守門。**

具體策略：

1. **M3 開工的第一個 task**：用 CLT + CMake 試編一個最小的
   ObjC++ bundle（先不含 foobar2000 SDK），驗證 toolchain 可行。
2. **第二個 task**：引入 foobar2000 macOS SDK，確認 SDK 原始碼
   能在 CMake + CLT 下編譯。這是本 ADR 的**風險驗證點**。
3. CI：mirror 到 GitHub 跑 `macos-14` runner job（public repo 免費），
   GitLab 留作主要工作流與 Linux CI。
4. foobar2000 SDK 不 commit 進 repo（授權條款保守處理）——
   CI 在 build step 下載，本地開發者依 CONTRIBUTING 說明自行取得。
5. Code signing：開發期用 ad-hoc（`codesign -s -`）；
   正式發佈的 Developer ID signing 延後到 M6 release 階段決策。

## Fallback 條件

若步驟 2 失敗（SDK 硬性依賴 Xcode 特有機制，例如：

- SDK 只能以 `.xcodeproj` 形式編譯，手動移植成本過高
- 依賴 Xcode 特有的 build phase / asset catalog 處理

則退回 **Option A 的 CI 變體**：`.xcodeproj` 進 repo，
編譯全部走 GitHub Actions macOS runner（那裡有完整 Xcode），
本地維持 CLT 只開發 core library。此時接受較慢的 component 迭代迴圈。

## Consequences

- docs/06「Phase 2 建立 Xcode project」的規劃由本 ADR 修正：
  預設不建 `.xcodeproj`，CMake 一路到底。
- `component_macos/` 的目錄結構仍照 docs/06，但 build 定義是
  CMakeLists.txt 而非 xcodeproj。
- 需要新增 GitHub mirror + Actions workflow（M3 第一批 task）。
- CONTRIBUTING 需說明 foobar2000 SDK 的取得方式。
- CMake 的 Xcode generator（`cmake -G Xcode`）保留為 escape hatch：
  未來若有貢獻者想用 Xcode IDE debug，可從同一份 CMakeLists 產生
  project，不需要維護兩套 build 定義。
