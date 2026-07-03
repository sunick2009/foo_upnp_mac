# Implement SOAP Browse request builder for ContentDirectory


## Description

建立 ContentDirectory Browse action 的 SOAP request builder，支援 ObjectID、BrowseFlag、Filter、StartingIndex、RequestedCount、SortCriteria。

## Acceptance Criteria

- 可產生 BrowseDirectChildren request。
- 可產生 BrowseMetadata request。
- XML namespace 正確。
- SOAPAction header 正確。
- 有 snapshot tests 或 fixture tests。

## Labels

`type::feature`, `area::upnp`, `priority::p1`
