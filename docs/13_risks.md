# 主要風險

## 1. macOS foobar2000 SDK 生態較小

雖然 macOS component 已可行，但公開範例較少。  
可能遇到：

- component loading 問題。
- UI lifecycle 問題。
- Preferences 儲存方式不清楚。
- playlist metadata API 需要研究。

## 2. UPnP Server 相容性

UPnP / DLNA server 的實作差異很大。  
常見問題：

- DIDL-Lite 欄位缺漏。
- protocolInfo 格式不一致。
- multiple res 項目需要選擇。
- albumArtURI 可能是 relative URL。
- Browse pagination 行為不同。
- 某些 server 需要特殊 User-Agent。

## 3. SSDP Discovery

MVP 不做 discovery 是刻意決策。  
SSDP 依賴 multicast，跨 VLAN / VPN / routed network 常失效。

## 4. Resource URL 播放

foobar2000 是否能對所有 HTTP URL 正常 seek，需要實測。  
尤其是：

- HTTP Range。
- URL expire。
- 需要 authentication。
- server 回傳 transcoded resource。

## 5. Packaging

macOS `.component` bundle 發佈可能涉及：

- bundle structure。
- signing。
- quarantine。
- Intel / Apple Silicon。
- user-components 安裝路徑。
