# E87: Receipt Builder Creative Command Boilerplate Audit

## Objective

Audit the current Creative command receipt copy/append pipeline and identify the
smallest safe helper extraction that reduces boilerplate without changing the
public receipt schema.

## Dependency

Do this after E86 unless planner explicitly redirects. This is read-only unless
the audit finds an obviously mechanical helper extraction with no public receipt
key changes.

## Problem

The normalized tally ranks receipt mirrors and command receipt boilerplate as a
top pressure surface. Current risk is drift between:

- command-frame receipt fields,
- `ProductAppWindowState` mirror fields,
- `recordProductCreativeUiCommandFrame(...)`,
- render receipt append keys,
- default-field tests,
- behavior-backed command tests.

A new command or diagnostic field can require touching too many places.

## Required Reads

- `docs/creative_mode/post_claude_architecture_review_tally.md`
- `src/app/iggy3d/ReceiptBuilder.hpp`
- `src/app/iggy3d/ReceiptBuilder.cpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.hpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.cpp`
- `src/app/iggy3d/creative/bridge/UiCommandCatalog.hpp`
- `tests/unit/product_creative_ui_command_receipt_tests.cpp`
- `tests/unit/product_creative_ui_command_frame_tests.cpp`
- `tests/unit/product_creative_ui_input_frame_tests.cpp`

## Scope

Read-only first:

- Count Creative command receipt fields.
- Identify copy functions and append blocks.
- Identify repeated field groups that could become named helpers.
- Separate public receipt schema from internal mirror plumbing.
- Identify weak-green tests that only prove default schema rather than
  production write paths.

If and only if there is a trivial helper extraction with no schema change,
builder may implement it. Otherwise leave a completion brief with the exact
helper extraction recommendation and stop.

## Do Not

- Do not rename public receipt keys.
- Do not remove receipt fields.
- Do not change command routing, command semantics, or sticky receipt behavior.
- Do not rewrite tests broadly.
- Do not start `ProductAppWindowState` ownership surgery.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Acceptance

- Completion brief contains a current field/copy/append tally.
- It names the first safe helper extraction with exact files/functions.
- If code is changed, public receipt output remains unchanged and focused
  command receipt tests pass.

## Suggested Verification

For audit-only:

```sh
git -C /Users/kogaryu/iggy3d diff --check
```

If code changes:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target product_creative_ui_command_receipt_tests product_creative_ui_command_frame_tests product_creative_ui_input_frame_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_command_receipt_tests|product_creative_ui_command_frame_tests|product_creative_ui_input_frame_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Files inspected:
- Files changed, if any:
- Receipt field/copy/append tally:
- Weak-green tests identified:
- First safe helper extraction:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief - 2026-07-06

- Files inspected:
  - `docs/creative_mode/post_claude_architecture_review_tally.md`
  - `src/app/iggy3d/ReceiptBuilder.hpp`
  - `src/app/iggy3d/ReceiptBuilder.cpp`
  - `src/app/iggy3d/creative/bridge/UiCommandFrame.hpp`
  - `src/app/iggy3d/creative/bridge/UiCommandFrame.cpp`
  - `src/app/iggy3d/creative/bridge/UiCommandCatalog.hpp`
  - `tests/unit/product_creative_ui_command_receipt_tests.cpp`
  - `tests/unit/product_creative_ui_command_frame_tests.cpp`
  - `tests/unit/product_creative_ui_input_frame_tests.cpp`
- Files changed, if any:
  - `src/app/iggy3d/ReceiptBuilder.cpp`
  - builder queue bookkeeping files for claim/done movement
- Receipt field/copy/append tally:
  - `ProductCreativeUiCommandFrameReceipt` currently exposes 83 flat source
    fields in `UiCommandFrame.hpp`.
  - `ProductAppWindowState` mirrors command diagnostics through grouped
    structs in `ReceiptBuilder.hpp`: mutation 16 fields, create 12, delete 13,
    undo 14, room shell 13, baked-room refresh 13, plus command-core fields.
  - `ReceiptBuilder.cpp` has one copy chain from command receipt to window
    mirror (`copyProductCreativeUiCommandDiagnostics`) and one append chain from
    mirror to render receipt (`appendProductCreativeUiCommandFields`).
  - Public command receipt append keys currently total 94
    `creative_ui_command*` keys: 14 core, 16 mutation, 12 create, 13 delete, 14
    undo, 13 room shell, and 12 manual baked-room refresh keys. Auto baked-room
    refresh is a separate 13-key group under `creative_baked_room_auto_refresh*`.
  - `product_creative_ui_command_receipt_tests.cpp` still contains 221 quoted
    `creative_ui_command*` key occurrences and 236 quoted `creative_*` keys.
- Weak-green tests identified:
  - `defaultWindowReceiptCarriesNotRequestedFields()` is mostly schema/default
    proof; it pins fields exist but does not prove production writers set them.
  - `defaultCommandReceiptRecordsSafely()` and
    `recorderPreservesNeighboringFields()` exercise direct recorder calls, not
    live input routing.
  - The behavior-backed samples do route command-frame receipts before building
    render receipts for toggle/create/delete/undo/room-shell, but receipt-output
    coverage is still centralized in a long expected-key list rather than a
    grouped schema descriptor.
- First safe helper extraction:
  - Implemented the smallest private helper in `ReceiptBuilder.cpp`:
    `ProductCreativeBakedRoomRefreshReceiptKeySet` plus
    `appendProductCreativeBakedRoomRefreshDiagnosticFields(...)`.
  - This consolidates the duplicate manual-command baked-room refresh append
    block and auto-refresh append block without renaming public receipt keys.
  - The helper preserves the existing intentional asymmetry: auto-refresh still
    emits `creative_baked_room_auto_refresh_cleared_active_room`; manual command
    baked-room refresh still does not emit a `cleared_active_room` key.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target
    product_creative_ui_command_receipt_tests
    product_creative_ui_command_frame_tests
    product_creative_ui_input_frame_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R
    '^(product_creative_ui_command_receipt_tests|product_creative_ui_command_frame_tests|product_creative_ui_input_frame_tests)$'
    --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Focused trailing-whitespace scan over `ReceiptBuilder.cpp`, this card, and
    `PRIORITY.md`.
- Concerns/deferred:
  - The largest remaining tax is the tri-copy schema: source receipt field,
    window mirror field, append key/test expectation. The next reduction should
    be a grouped command receipt schema descriptor that can generate append-key
    expectations and centralize default values before any public field changes.
  - `ProductCreativeBakedRoomRefreshDiagnostics::clearedActiveRoom` is present
    in the shared diagnostics struct but only public on the auto-refresh receipt
    namespace. That is intentional today, but it should be named in any future
    schema descriptor so the asymmetry is explicit.
