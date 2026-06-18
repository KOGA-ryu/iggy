# 21 UI Preview Consumer Status Sync

Status: complete.

Goal: update planning docs after the first read-only Qt shell authoring preview
consumer integrated.

Scope:
- Docs/status sync only.
- Reflect `iggy_qt_shell --preview PATH` and
  `--preview-mode run|trace|check|lint`.
- Mark the first read-only Qt preview consumer milestone complete.
- Keep later source-linked diagnostics, visual trace, play shell, and editor
  mutation work gated.

Hard stops preserved:
- No production code changes.
- No editing or source mutation.
- No UI-owned parser.
- No file watcher or package scanning.
- No new gameplay semantics.

Verification:
- `git diff --check`
- `git status --short --branch`
