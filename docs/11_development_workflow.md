# 開發工作流

## 原則

1. 先 CLI，後 component。
2. 先 parser tests，後 UI。
3. 每個 MR 只解決一個 issue。
4. 不接受大型一次性 commit。
5. 所有網路錯誤必須可診斷。
6. 所有相容性宣稱必須有測試證據。

## MR 流程

```text
Issue ready
→ branch from main
→ implement small change
→ add tests
→ update docs if needed
→ open MR
→ review
→ merge
```

## Branch 命名

```text
feature/issue-003-http-client
fix/issue-012-didl-unicode
docs/issue-010-phase0-usage
```

## Commit 訊息

```text
feat(core): add URL resolver
test(upnp): add MiniDLNA rootDesc fixture
docs(cli): document Phase 0 browse command
```

## CI 建議

Phase 0 可先用 Linux runner：

- configure
- build
- test
- format check
- lint

Phase 2 再加入 macOS runner：

- build component
- package component
- smoke test bundle structure
