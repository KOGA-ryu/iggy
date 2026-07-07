# E167 — InputDeviceStore G3: Interaction Mode State

**STATUS: DONE — ready for review.**
Parent: `blocked/E156-inputdevicestore-bulk-move.md`.
Commit convention: `claude: planned. codex: ...`.

## Goal

Finish `InputDeviceStore` by moving the two high-collision interaction mode
fields.

Move exactly these two fields:

```text
interactionMode
interactionModeHud
```

Do not move:

```text
gamepadMenuSelectUsed
```

## Dominant Hazard

`interactionMode` is a real field on multiple foreign request/routing structs.
A broad replacement will corrupt unrelated policy. Delete the flat fields and
let the compiler identify only true `ProductAppWindowState` accesses.

Known foreign owners include controller action routing/map, mouse capture
policy, frontend routing, and top-down map overlay request shapes. Those should
not be repointed unless they are actually storing a `ProductAppWindowState`.

`interactionModeHud` also appears on projection/frame structures; those foreign
fields stay where they are.

## Scope

1. Add `interactionMode` and `interactionModeHud` to `InputDeviceStore`,
   preserving exact types/defaults.
2. Delete the two flat fields from `ProductAppWindowState`.
3. Repoint only `ProductAppWindowState` accesses to
   `<window>.inputDevice.interactionMode` or
   `<window>.inputDevice.interactionModeHud`.
4. Update `docs/god_struct_member_ownership.tsv` by deleting the two moved
   top-level rows. Keep the existing `inputDevice	InputDeviceStore` row.
5. Update `docs/god_struct_decomposition_target_map.md` and `PRIORITY.md` to
   mark InputDeviceStore complete.

## Laws

- Receipt golden must stay byte-identical.
- Compiler-guided repointing only. No broad sed, no token-wide replace.
- Do not move `gamepadMenuSelectUsed`.
- Do not touch foreign `.interactionMode` or `.interactionModeHud` fields.
- Do not launch a window.

## Required Greps

```sh
rg -n "window\.(interactionMode|interactionModeHud)\b" \
  /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests /Users/kogaryu/iggy3d/tools \
  --glob '*.cpp' --glob '*.hpp'
rg -n "\b(interactionMode|interactionModeHud)\b" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp
rg -n "^(interactionMode|interactionModeHud)\b" \
  /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
```

Also report remaining foreign `.interactionMode` / `.interactionModeHud` hits
and classify them briefly.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests
ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Also run the focused trailing-whitespace scan over touched files.

## Completion Brief

- Files changed:
  - `src/app/iggy3d/input/InputDeviceStore.hpp`
  - `src/app/iggy3d/ProductAppWindowState.hpp`
  - `src/app/iggy3d/Operations.cpp`
  - `src/app/iggy3d/ReceiptBuilder.cpp`
  - `src/app/iggy3d/automation/AutomationRoomEditing.cpp`
  - `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
  - `src/app/iggy3d/input/InteractionModeState.cpp`
  - `src/app/iggy3d/menu/ActionHandlers.cpp`
  - `src/app/iggy3d/menu/FrontendRouter.cpp`
  - `src/app/iggy3d/menu/Transitions.cpp`
  - `src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp`
  - `src/app/iggy3d/window/InputFrame.cpp`
  - `src/app/iggy3d/window/Loop.cpp`
  - focused product/creative tests that directly instantiated `ProductAppWindowState`
  - `docs/god_struct_member_ownership.tsv`
  - `docs/god_struct_decomposition_target_map.md`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
- Store move:
  - Added `ProductInteractionMode interactionMode = ProductInteractionMode::Player` and `InteractionModeHud interactionModeHud` to `ProductAppWindowState::inputDevice`.
  - Removed the flat `ProductAppWindowState::interactionMode` and `ProductAppWindowState::interactionModeHud` fields.
  - Left `gamepadMenuSelectUsed` flat on `ProductAppWindowState`, still assigned to `FrontendWindowShell`.
- Repointing policy:
  - Repointed compiler-reported `ProductAppWindowState` reads/writes to `window.inputDevice.interactionMode` and `window.inputDevice.interactionModeHud`.
  - Test repoints were limited to direct `ProductAppWindowState` fixture variables such as `window`, `harness.window`, `openWindow`, `scenario.window`, `creativeWorldWindow`, and related fixture names.
  - Did not repoint foreign request/routing structs.
- Required greps:
  - `rg -n "window\.(interactionMode|interactionModeHud)\b" ...` produced no matches.
  - `rg -n "\b(interactionMode|interactionModeHud)\b" src/app/iggy3d/ProductAppWindowState.hpp` produced no matches.
  - `rg -n "^(interactionMode|interactionModeHud)\b" docs/god_struct_member_ownership.tsv` produced no matches.
- Remaining foreign `.interactionMode` / `.interactionModeHud` hits:
  - Canonical store paths remain as `.inputDevice.interactionMode` / `.inputDevice.interactionModeHud`.
  - Foreign request/context/result fields remain intentionally: `ProductActiveSurfaceContext::interactionMode`, `TopDownMapOverlayRequest::interactionMode`, controller action map/routing request/result interaction-mode fields, `ProductMouseCapturePolicyRequest::interactionMode`.
  - Projection/render frame HUD payloads remain intentionally: `ProductGameplayProjectionFrame::interactionModeHud`, render frame/presenter `projectionFrame.interactionModeHud`, and local test frame payloads.
- Receipt/docs:
  - Receipt key-order golden stayed byte-identical: `product_receipt_key_order_tests` reported `1032 fields match golden`.
  - `god_struct_member_ownership.tsv` now has only the top-level `inputDevice	InputDeviceStore` row for this store; the moved top-level rows are gone.
  - Target map and priority now mark InputDeviceStore/E165-E167 complete.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build -j10` — pass.
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests` — pass.
  - `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests` — pass.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure` — pass, 260/260.
  - `git -C /Users/kogaryu/iggy3d diff --check` — pass.
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` — no diff.
  - Focused trailing-whitespace scan over existing touched files and the task card — no matches.
- Concerns/deferred:
  - None for E167. `gamepadMenuSelectUsed` intentionally remains outside `InputDeviceStore`.
