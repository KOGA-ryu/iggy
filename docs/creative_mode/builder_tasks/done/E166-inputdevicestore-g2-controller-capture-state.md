# E166 — InputDeviceStore G2: Controller + Mouse Capture State

**STATUS: READY — claim next.**
Parent: `blocked/E156-inputdevicestore-bulk-move.md`.
Commit convention: `claude: planned. codex: ...`.

## Goal

Continue the `InputDeviceStore` move after E165 by moving the controller and
mouse-capture fields.

Move exactly these three fields:

```text
mouseCapture
controllerModeToggle
controllerAction
```

Do not move in this card:

```text
interactionMode
interactionModeHud
gamepadMenuSelectUsed
```

`mouseCapture.inputOwner` stays inside `mouseCapture`. Never replace bare
`inputOwner`.

## Scope

1. Add the three fields to `InputDeviceStore`, preserving exact types/defaults.
2. Delete the three flat fields from `ProductAppWindowState`.
3. Repoint production and test callers from `<window>.field` to
   `<window>.inputDevice.field`.
4. Update `docs/god_struct_member_ownership.tsv` by deleting the three moved
   top-level rows. Keep the existing `inputDevice	InputDeviceStore` row.
5. Update `docs/god_struct_decomposition_target_map.md` and `PRIORITY.md` to
   mark G2 complete.

## Laws

- Receipt golden must stay byte-identical.
- Compiler-guided repointing only.
- Do not touch `interactionMode` or `interactionModeHud`.
- Do not move `gamepadMenuSelectUsed`.
- Do not replace bare `inputOwner`.
- Do not launch a window.

## Required Greps

```sh
rg -n "window\.(mouseCapture|controllerModeToggle|controllerAction)\b" \
  /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests /Users/kogaryu/iggy3d/tools \
  --glob '*.cpp' --glob '*.hpp'
rg -n "\b(mouseCapture|controllerModeToggle|controllerAction)\b" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp
rg -n "^(mouseCapture|controllerModeToggle|controllerAction)\b" \
  /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
```

Remaining same-name hits are allowed only if they are not flat
`ProductAppWindowState` fields/accesses.

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

---

## Completion Brief

- Files changed:
  - `src/app/iggy3d/input/InputDeviceStore.hpp`
  - `src/app/iggy3d/ProductAppWindowState.hpp`
  - `src/app/iggy3d/input/InteractionModeState.cpp`
  - `src/app/iggy3d/input/ControllerActionRouting.cpp`
  - `src/app/iggy3d/window/InputFrame.cpp`
  - `src/app/iggy3d/window/Loop.cpp`
  - `src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp`
  - `tests/unit/product_window_renderer_lifecycle_tests.cpp`
  - `tests/unit/product_mouse_capture_policy_tests.cpp`
  - `tests/unit/product_interaction_mode_state_tests.cpp`
  - `tests/unit/product_controller_action_routing_tests.cpp`
  - `tests/unit/product_window_input_frame_tests.cpp`
  - `docs/god_struct_member_ownership.tsv`
  - `docs/god_struct_decomposition_target_map.md`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
  - `docs/creative_mode/builder_tasks/done/E166-inputdevicestore-g2-controller-capture-state.md`
- Fields moved:
  - Moved exactly `mouseCapture`, `controllerModeToggle`, and `controllerAction` into `ProductAppWindowState::inputDevice`.
  - Preserved the original member types/defaults: `ProductMouseCaptureState`, `ProductControllerModeToggleState`, and `ProductControllerActionState`.
  - Repointed product source/tests to `window.inputDevice.mouseCapture`, `window.inputDevice.controllerModeToggle`, and `window.inputDevice.controllerAction`.
  - Did not move `interactionMode`, `interactionModeHud`, or `gamepadMenuSelectUsed`.
  - Did not replace bare `inputOwner`; `inputOwner` remains only as existing route/policy fields or inside `inputDevice.mouseCapture.inputOwner`.
- Receipt golden result:
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests` passed.
  - Output: `receipt key-order oracle: 1032 fields match golden (order + values)`.
  - `git diff -- tests/golden/product_receipt_key_order.golden` produced no diff.
- Ownership coverage result:
  - `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests` passed.
  - Output: `god-struct ownership coverage: assigned=129 CreativeAuthoringStore=86 DebugHudStore=5 FrontendWindowShell=16 GameplayStore=1 InputDeviceStore=3 PresentPathStore=6 RoomStore=1 SaveSessionStore=1 ViewportStore=2 app-global-remainder=7 delete=1`.
  - Deleted the three moved top-level ownership rows and kept the existing `inputDevice	InputDeviceStore` row.
- Required grep results:
  - `rg -n "window\\.(mouseCapture|controllerModeToggle|controllerAction)\\b" ...` — no matches.
  - `rg -n "\\b(mouseCapture|controllerModeToggle|controllerAction)\\b" src/app/iggy3d/ProductAppWindowState.hpp` — no matches.
  - `rg -n "^(mouseCapture|controllerModeToggle|controllerAction)\\b" docs/god_struct_member_ownership.tsv` — no matches.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build -j10` — passed.
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests` — passed.
  - `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests` — passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure` — passed, 260/260.
  - `git -C /Users/kogaryu/iggy3d diff --check` — passed.
  - Focused trailing-whitespace scan over touched files — no hits.
- Concerns/deferred:
  - No stage, commit, push, or window launch.
  - Receipt keys/order stayed unchanged.
  - E167 remains deferred and unclaimed.
