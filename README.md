# foo_dms_browser_mac Project Planning Pack

本壓縮包是 `foo_dms_browser_mac` 的專案規劃與 GitLab 啟動資料包。

## 專案定位

`foo_dms_browser_mac` 是一個規劃中的 foobar2000 macOS 原生 component。  
目標是讓使用者可以手動新增 UPnP / DLNA Media Server 的 `rootDesc.xml` URL，瀏覽遠端 ContentDirectory，並將音樂項目加入 foobar2000 playlist 播放。

第一版不是完整 `foo_upnp` clone，而是輕量的 Digital Media Server Browser。

## 核心策略

第一階段不建立 Xcode project，也不整合 foobar2000 SDK。  
先開發 Phase 0 CLI PoC，驗證以下流程：

```text
rootDesc.xml URL
→ device description parser
→ ContentDirectory service resolver
→ SOAP Browse request
→ DIDL-Lite parser
→ JSON / table output
```

等 CLI PoC 與 core library 穩定後，再導入 foobar2000 macOS component 與 Xcode project。

## 目錄說明

```text
docs/                         專案規劃文件
gitlab/issues/                第一批 GitLab issue 草稿
gitlab/labels/                labels 定義
gitlab/milestones/            milestones 定義
.gitlab/issue_templates/      GitLab issue templates
.gitlab/merge_request_templates/ GitLab MR template
repo-skeleton/                建議 repo 骨架
prompts/                      給 Claude Fable 5 的開發 prompt
```
