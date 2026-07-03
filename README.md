# foo_dms_browser_mac

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
| M3 foobar2000 macOS MVP | ⬜ 未開始 |

Phase 0 CLI 已對 foobar2000 UPnP Media Server (foo_upnp 0.99.49) 完整驗證：
root/child browse、BrowseMetadata、pagination、中日文 metadata、
multi-res 選擇、album art URL。詳見 `docs/10_compatibility_plan.md`。

## Build

```bash
brew install curl cmake        # macOS one-time setup
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j 8
ctest --test-dir build --output-on-failure
```

pugixml 與 Catch2 由 FetchContent 自動下載（首次 configure 需網路）。
Linux 需要 `cmake libcurl4-openssl-dev python3`（見 `.gitlab-ci.yml`）。

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
docs/adr/             12 個 Architecture Decision Records（全部 Accepted）
docs/15_domain_glossary.md   UPnP 術語表
docs/phase0-cli.md    CLI build / usage / e2e 測試指南
gitlab/               GitLab issues / labels / milestones 草稿
```

## Roadmap

- **M3**：Xcode project + foobar2000 macOS SDK、Preferences page、
  Browser panel、playlist 整合（`docs/06`）
- **M4**：metadata 與播放體驗改善
- **M5**：MiniDLNA / Jellyfin / Plex 相容性、async loading
- **M6**：SSDP discovery、release packaging（`docs/12`）
