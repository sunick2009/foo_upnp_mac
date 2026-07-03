# ADR-001: UpnpObject 型別設計 — union struct vs tagged types

**狀態：** Accepted  
**日期：** 2026-07-03

## Context

ContentDirectory Browse 回傳兩種東西：
- **Container**：目錄，有 `childCount`，沒有 `resources`。
- **Item**：音訊檔案，有 `resources`，沒有 `childCount`。

目前 `UpnpObject` 使用 `UpnpObjectType enum + optional<>` 的 union struct。

```cpp
struct UpnpObject {
    UpnpObjectType type;    // Container or AudioItem
    std::string id;
    std::string parentId;
    std::string title;
    std::optional<std::string> artist;   // item only
    std::optional<std::string> album;    // item only
    std::vector<UpnpResource> resources; // item only
    // container childCount: missing from current design!
};
```

## Options

### Option A: 保持 union struct（目前方向）
所有欄位在同一個 struct，型別由 enum 決定。

- 優點：簡單、傳遞容易、std::vector<UpnpObject> 直觀。
- 缺點：呼叫端必須 check type 才能存取 item-only 欄位；容易犯錯；childCount 目前缺漏。

### Option B: std::variant<Container, AudioItem>

```cpp
struct Container { std::string id, parentId, title; uint32_t childCount; };
struct AudioItem { std::string id, parentId, title; std::vector<UpnpResource> resources; /* metadata */ };
using UpnpObject = std::variant<Container, AudioItem>;
```

- 優點：型別安全，無法存取不屬於自己的欄位。
- 缺點：std::visit 在 Phase 0 稍嫌繁瑣；variant 的 JSON 序列化需要多寫程式。

### Option C: 繼承（polymorphism）
- 優點：OO 自然。
- 缺點：需要 heap allocation、virtual dtor、dynamic_cast；與 C++17 idioms 不一致。

## Decision

**選 Option A，但補上缺漏欄位，並加強 API contract 文件。**

理由：
- Phase 0 CLI 主要是輸出 JSON，不需要複雜的型別安全。
- `std::vector<UpnpObject>` 序列化直接。
- 在 code review 時加上 assertion 防止誤用。

補充設計：
```cpp
struct UpnpObject {
    UpnpObjectType type;
    std::string id;
    std::string parentId;
    std::string title;
    // Container fields (valid only when type == Container)
    std::optional<uint32_t> childCount;
    // Item fields (valid only when type == AudioItem or Unknown)
    std::optional<std::string> artist;
    std::optional<std::string> album;
    std::optional<std::string> genre;
    std::optional<std::string> albumArtUri;
    std::vector<UpnpResource> resources;
};
```

## Consequences

- `DidlLiteParser` 必須在解析 container 時填入 `childCount`。
- 呼叫端存取 item-only 欄位時，有責任先確認 `type`。
- 若 Phase 1 需要型別安全，可以重構為 `std::variant`，現在不過度設計。

## Review Trigger

若 Phase 1 component 的 UI code 出現超過 3 處 `if (obj.type == AudioItem)` 的 guard，
重新考慮改用 Option B。
