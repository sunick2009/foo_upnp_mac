# foobar2000 mac layout 範例

foobar2000 mac 的 layout 是縮排文字 DSL（app 內建預設存於
`foobar2000.app/Contents/Resources/Layout-*.txt`）。規則：

- 每層縮排一個空格；父節點是 `splitter` 或 `tabs`，葉節點是 element。
- `splitter horizontal` 上下堆疊、`splitter vertical` 左右並排。
- `tabs position=top` 的每個子節點用 `tab-name="..."` 命名分頁。
- 已知內建 element：`playlist`、`playlist-picker`、`albumlist`、
  `albumart type="front cover"`、`playback-controls`、`searchbox`、
  `filters`、`selection-properties`、`console`。
- 第三方 element（如本 component 的 `DMS Browser`）以
  `ui_element_mac::match_name` 的名稱引用。

套用方式：foobar2000 → Layout → Edit Layout...，貼上檔案內容。

## dms_browser_sidebar.txt

左側 tab（DMS Browser / Playlists）＋中間 playlist ＋右側封面＋
底部播放控制列。

若「DMS Browser」一行報 `Unknown element`，改成加引號的
`"DMS Browser"` 再試一次。
