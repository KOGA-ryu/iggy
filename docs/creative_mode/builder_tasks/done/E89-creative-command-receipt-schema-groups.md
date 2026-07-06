# E89: Creative Command Receipt Schema Groups

## Objective

Reduce Creative command receipt-test boilerplate by introducing grouped expected
receipt-field schema helpers without changing public receipt keys or production
behavior.

## Problem

E87 found that `product_creative_ui_command_receipt_tests.cpp` still contains
hundreds of repeated `creative_*` key literals. These tests pin the public
receipt schema, but the current shape makes every new field a long manual tuple
edit.

The next safe step is test/schema organization only: group the expected command
receipt fields by diagnostic family and use the grouped schema in assertions.

## Required Reads

- `docs/creative_mode/post_claude_architecture_review_tally.md`
- `docs/creative_mode/builder_tasks/done/E87-receipt-builder-creative-command-boilerplate-audit.md`
- `src/app/iggy3d/ReceiptBuilder.hpp`
- `src/app/iggy3d/ReceiptBuilder.cpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.hpp`
- `tests/unit/product_creative_ui_command_receipt_tests.cpp`
- `tests/unit/product_creative_ui_command_frame_tests.cpp`

## Scope

Allowed:

- Create test-local grouped schema helpers for the public
  `creative_ui_command*` receipt keys.
- Group fields by command-core, mutation, create, delete, undo, room-shell, and
  baked-room-refresh families.
- Replace long repeated expected-field chains with calls to the grouped helpers.
- Keep behavior-backed command samples intact.
- If a tiny production-side constant list is clearly safer than test-local
  helpers, stop and explain first; do not widen into production schema ownership
  unless the change is obviously mechanical.

## Do Not

- Do not rename public receipt keys.
- Do not add or remove receipt keys.
- Do not change `ReceiptBuilder` output.
- Do not change command routing or command receipt source structs.
- Do not use this card to migrate away from `ProductAppWindowState` mirrors.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Acceptance

- Public receipt output is unchanged.
- The command receipt test file has fewer duplicated key literals or shorter
  default-field assertions.
- The grouped schema makes the existing intentional asymmetry explicit:
  auto-refresh has `cleared_active_room`; manual command baked-room refresh does
  not.
- Focused command receipt tests pass.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target product_creative_ui_command_receipt_tests product_creative_ui_command_frame_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_command_receipt_tests|product_creative_ui_command_frame_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Behavior changed:
- Schema groups added:
- Key literal / assertion reduction:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief - 2026-07-06

- Files changed:
  - `tests/unit/product_creative_ui_command_receipt_tests.cpp`
  - builder queue bookkeeping files for claim/done movement.
- Behavior changed:
  - No production behavior changed.
  - No public receipt keys were renamed, added, or removed.
  - Command routing, command receipt source structs, and `ReceiptBuilder` output
    were not changed.
- Schema groups added:
  - Test-local default schema groups:
    - `kDefaultCommandCoreReceiptFields`
    - `kDefaultCommandMutationReceiptFields`
    - `kDefaultCommandCreateReceiptFields`
    - `kDefaultCommandDeleteReceiptFields`
    - `kDefaultCommandUndoReceiptFields`
    - `kDefaultCommandRoomShellReceiptFields`
    - `kDefaultCommandBakedRoomRefreshReceiptFields`
  - Added `expectDefaultCommandReceiptSchema(...)` to assert those groups as a
    single named schema contract.
  - Added an array overload of `expectReceiptFields(...)` so grouped schemas can
    stay constexpr and test-local.
  - The baked-room refresh group now documents the intentional asymmetry:
    manual command refresh does not expose `cleared_active_room`; the separate
    auto-refresh namespace owns that key.
- Key literal / assertion reduction:
  - `tests/unit/product_creative_ui_command_receipt_tests.cpp` changed by
    `507 insertions(+), 786 deletions(-)`, net -279 lines.
  - The default command receipt assertion body was reduced from seven long
    inline expected-field chains to `expectDefaultCommandReceiptSchema(receipt)`.
  - The quoted `creative_ui_command*` key count remains 221 by design because
    this slice grouped the public schema in test-local constants rather than
    replacing public key literals with production constants.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target product_creative_ui_command_receipt_tests product_creative_ui_command_frame_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_command_receipt_tests|product_creative_ui_command_frame_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Focused trailing-whitespace scan over
    `tests/unit/product_creative_ui_command_receipt_tests.cpp`, this card, and
    `PRIORITY.md`.
- Concerns/deferred:
  - This keeps schema ownership test-local. The next receipt-shape reduction is
    still a production/schema descriptor decision, not something to sneak into a
    test cleanup.
  - Behavior-backed command samples remain intentionally explicit; this card
    only compressed the default schema proof.
