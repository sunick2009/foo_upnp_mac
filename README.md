# foo_dms_browser_mac

[![ci](https://github.com/sunick2009/foo_upnp_mac/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/sunick2009/foo_upnp_mac/actions/workflows/ci.yml)

一個開發中的 foobar2000 macOS 原生 component：手動新增 UPnP / DLNA
Media Server 的 device description URL，瀏覽 ContentDirectory，
並將遠端音樂加入 foobar2000 playlist 播放。

第一版不是完整 `foo_upnp` clone，而是輕量的 Digital Media Server Browser。
不做 MediaServer / Renderer / 轉碼 / SSDP discovery（見 `docs/02`）。

## 目前狀態

| Milestone | 狀態 |
|---|---|
| M0 Project Bootstrap | ✅ 完成 |
| M1 Phase 0 CLI PoC | ✅ 完成 — 已對真實 server 驗證 |
| M2 Core Library | ✅ 完成 — clean API、fixture tests、mockable transport、CI |
| M3 foobar2000 macOS MVP | ✅ 完成 — 真實 server 瀏覽→加入→播放全流程驗收通過（docs/21） |
| M4 Browser 體驗與 metadata | 🔄 進行中 — 主要功能已實作，驗收收尾中（docs/21） |

Phase 0 CLI 已對 foobar2000 UPnP Media Server (foo_upnp 0.99.49) 完整驗證：
root/child browse、BrowseMetadata、pagination、中日文 metadata、
multi-res 選擇、album art URL。詳見 `docs/10_compatibility_plan.md`。

### M4 進度（component 版本 `0.2.0-dev`）

已實作並經真機驗證：

- 「DMS Browser」layout element（`ui_element_mac`）整合，可放入自訂 layout
- browser 底部 metadata / resource 摘要與 album art 預覽
- container「加入直接子項曲目」與「遞迴加入所有曲目」（含取消）
- playlist / Now Playing 封面（`album_art_fallback` 下載）
- 真實 server 播放與 seek（foo_upnp 0.99.49，HTTP Range）

剩餘驗收與已知缺陷（逐項狀態見 `docs/21_manual_test_checklist.md`）：

- browser 回應性與 Preferences 變更即時反映
- 遞迴加入的掃描進度顯示、上限與略過數量回報
- 錯誤列重試、URL 修正後重新套用、SOAP fault 顯示
- Preferences URL 欄位在編輯中關閉視窗只存 `http://` 的缺陷

## Install（使用者）

發佈的套件為 `foo_dms_browser-<版本>-arm64.fb2k-component`
（`.component` bundle 的 zip 封裝，附 `.sha256` 校驗檔）。
目前僅提供 Apple Silicon (arm64)、macOS 11.0+；
Intel Mac 請依下節從原始碼編譯。

取得方式（正式 Release 發佈前，可從 CI 下載每次 main 的建置）：

1. [GitHub Releases](https://github.com/sunick2009/foo_upnp_mac/releases)
   （v0.2.0 beta 起），或
2. [Actions](https://github.com/sunick2009/foo_upnp_mac/actions/workflows/ci.yml)
   → 選最新綠色 run → Artifacts → `foo_dms_browser-fb2k-component`。

安裝：foobar2000 → Preferences → Components → Install… 選取
`.fb2k-component`；若該路徑不可用，手動解壓後把
`foo_dms_browser.component` 放到
`~/Library/foobar2000-v2/user-components/`，重啟 foobar2000。
驗證下載：`shasum -a 256 -c foo_dms_browser-<版本>-arm64.fb2k-component.sha256`。

## Build

```bash
brew install curl cmake        # macOS one-time setup
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j 8
ctest --test-dir build --output-on-failure
```

pugixml 與 Catch2 由 FetchContent 自動下載（首次 configure 需網路）。
Linux 需要 `cmake libcurl4-openssl-dev python3`（見 `.gitlab-ci.yml`）。

CI：GitHub Actions（`.github/workflows/ci.yml`）跑 macOS component
build gate + Linux core build；自架 GitLab 另跑 `.gitlab-ci.yml` 的
Linux pipeline。

M3 的 macOS toolchain 驗證另有一個獨立 smoke test，見
`docs/17_macos_toolchain_smoke_test.md`。它會在 `ENABLE_MACOS_BUNDLE_SMOKE_TEST=ON`
時額外編譯最小 ObjC++ Cocoa bundle，不影響既有 CLI/core build。

foobar2000 SDK 的 CMake 編譯驗證（ADR-013 風險驗證點，已通過）用
`ENABLE_FB2K_SDK_CHECK=ON`，SDK 取得方式與細節見
`docs/19_fb2k_sdk_cmake_check.md`。SDK 不進 repo。

## Usage

```bash
./build/upnp-browser-cli browse \
  --server http://192.168.1.10:8200/rootDesc.xml \
  --object-id 0 \
  --output table
```

完整參數與 mock server 測試方式見 `docs/phase0-cli.md`。

## Architecture

```text
src/cli/            upnp-browser-cli（thin wrapper）
src/core/http/      HttpClient — blocking libcurl，virtual 可注入 fake
src/core/upnp/      UrlResolver / DeviceDescriptionParser /
                    SoapBrowseRequestBuilder / SoapBrowseResponseParser /
                    DidlLiteParser / ResourceSelector /
                    ContentDirectoryClient（orchestrator）
tests/              Catch2 unit + integration + scripted e2e（mock server）
tools/              python mock UPnP server
```

Core library 不依賴 foobar2000 SDK / Cocoa / Objective-C runtime，
CLI 與未來的 component 共用同一套邏輯。設計決策見 `docs/adr/`。

## Documentation

```text
docs/                 規劃文件（goal / scope / architecture / testing / risks）
docs/adr/             13 個 Architecture Decision Records（全部 Accepted）
docs/15_domain_glossary.md   UPnP 術語表
docs/phase0-cli.md    CLI build / usage / e2e 測試指南
gitlab/               GitLab issues / labels / milestones 草稿
```

## Roadmap

- **M3（✅ 完成）**：CMake macOS bundle + foobar2000 macOS SDK、
  Preferences page、Browser panel、playlist 整合（`docs/06`、
  `docs/adr/ADR-013`）
- **M4（🔄 進行中）**：browser metadata/album art、遞迴加入、layout
  element 已實作；剩餘為驗收收尾與缺陷修復（見上方「M4 進度」與
  `docs/21_manual_test_checklist.md`）
- **M5**：MiniDLNA / Jellyfin / Plex 相容性（`docs/10`）
- **M6**：SSDP discovery、release packaging（`docs/12`）
