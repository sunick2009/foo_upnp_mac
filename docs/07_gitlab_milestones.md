# GitLab Milestones

## Milestone 0：Project Bootstrap

目標：建立 repo 與最小開發基礎。

交付：

- repo structure
- README / LICENSE / docs
- GitLab labels
- issue templates
- MR template
- CMake skeleton

## Milestone 1：Phase 0 CLI PoC

目標：完成可執行 CLI，驗證手動 rootDesc.xml URL 的 UPnP Browse 流程。

交付：

- HTTP client
- device description parser
- URL resolver
- SOAP Browse request builder
- SOAP response parser
- DIDL-Lite parser
- CLI browse command

## Milestone 2：Core Library

目標：將 CLI PoC 抽象成可重用 core library。

交付：

- `src/core/`
- clean API
- parser tests
- mockable network layer
- error model

## Milestone 3：foobar2000 macOS MVP

目標：建立 macOS component，能設定 server、瀏覽、加入播放清單。

交付：

- Xcode project
- component skeleton
- Preferences page
- Browser panel
- playlist integration

## Milestone 4：Metadata and Playback

目標：改善播放項目 metadata 與 resource 選擇。

交付：

- metadata mapping
- duration parser
- protocolInfo parser
- multiple resource selection
- playback validation

## Milestone 5：Compatibility and Reliability

目標：提高實用性與相容性。

交付：

- MiniDLNA compatibility
- Jellyfin DLNA compatibility
- Plex DLNA compatibility
- pagination
- async loading
- debug logging

## Milestone 6：Discovery and Release Polish

目標：補 SSDP discovery 與 release 文件。

交付：

- M-SEARCH
- discovery UI
- release packaging
- compatibility docs
- troubleshooting docs
- v0.1.0 release
