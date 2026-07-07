# E165 — InputDeviceStore G1: Device + Last Input State

**STATUS: READY — claim next.**
Parent: `blocked/E156-inputdevicestore-bulk-move.md`.
Commit convention: `claude: planned. codex: ...`.

## Goal

Create `InputDeviceStore` and move the first low-risk input-device fields off
`ProductAppWindowState` while keeping the tree fully green.

Move exactly these five fields:

```text
gamepadAvailable
gamepadName
gamepadMapping
lastInputAction
lastInputAccepted
```

Do not move in this card:

```text
mouseCapture
controllerModeToggle
controllerAction
interactionMode
interactionModeHud
gamepadMenuSelectUsed
```

`gamepadMenuSelectUsed` belongs to FrontendWindowShell, not InputDeviceStore.

## Scope

1. Add `src/app/iggy3d/input/InputDeviceStore.hpp`.
2. Add `InputDeviceStore inputDevice;` to `ProductAppWindowState`.
3. Move only the five fields listed above into `InputDeviceStore`, preserving
   exact types/defaults.
4. Repoint production and test callers from `<window>.field` to
   `<window>.inputDevice.field`.
5. Update `docs/god_struct_member_ownership.tsv`:
   - delete the five moved top-level rows;
   - add `inputDevice	InputDeviceStore` if it is not already present.
6. Update `docs/god_struct_decomposition_target_map.md` and `PRIORITY.md` to
   mark G1 complete, with E166 still blocked.

## Laws

- Receipt golden must stay byte-identical.
- Do not use broad replace-all. Compiler-guided repointing only.
- Do not touch `interactionMode` or `interactionModeHud` in this card.
- Do not move `gamepadMenuSelectUsed`.
- Do not rename receipt keys.
- Do not launch a window.

## Required Greps

After implementation, these should return no stale flat window hits for the five
moved fields:

```sh
rg -n "window\.(gamepadAvailable|gamepadName|gamepadMapping|lastInputAction|lastInputAccepted)\b" \
  /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests /Users/kogaryu/iggy3d/tools \
  --glob '*.cpp' --glob '*.hpp'
rg -n "\b(gamepadAvailable|gamepadName|gamepadMapping|lastInputAction|lastInputAccepted)\b" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp
rg -n "^(gamepadAvailable|gamepadName|gamepadMapping|lastInputAction|lastInputAccepted)\b" \
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

## Completion Brief

Report:

- exact files changed;
- which five fields moved;
- receipt golden result;
- ownership coverage result;
- required grep results;
- full CTest result;
- confirmation that E166/E167 remain deferred.

---

## Completion Brief

- Files changed:
  - `src/app/iggy3d/input/InputDeviceStore.hpp`
  - `src/app/iggy3d/ProductAppWindowState.hpp`
  - `src/app/iggy3d/window/InputFrame.cpp`
  - `src/app/iggy3d/menu/InputRouter.cpp`
  - `src/app/iggy3d/automation/AutomationGameplay.cpp`
  - `src/app/iggy3d/automation/AutomationDispatch.cpp`
  - `src/app/iggy3d/automation/AutomationRoomEditing.cpp`
  - `src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp`
  - `src/app/iggy3d/receipt/FeedbackSurfaceAutomationVulkanFields.cpp`
  - `tests/unit/product_window_input_frame_tests.cpp`
  - `docs/god_struct_member_ownership.tsv`
  - `docs/god_struct_decomposition_target_map.md`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
  - `docs/creative_mode/builder_tasks/done/E165-inputdevicestore-g1-device-action-state.md`
- Fields moved:
  - Added `InputDeviceStore inputDevice` to `ProductAppWindowState`.
  - Moved exactly `gamepadAvailable`, `gamepadName`, `gamepadMapping`, `lastInputAction`, and `lastInputAccepted` into `InputDeviceStore` with the original types/defaults.
  - Repointed product source/tests to `window.inputDevice.*` for those five fields.
  - Did not move `mouseCapture`, `controllerModeToggle`, `controllerAction`, `interactionMode`, `interactionModeHud`, or `gamepadMenuSelectUsed`.
  - `gamepadMenuSelectUsed` remains flat and is accounted to `FrontendWindowShell` in the ownership TSV.
- Receipt golden result:
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests` passed.
  - Output: `receipt key-order oracle: 1032 fields match golden (order + values)`.
  - `git diff -- tests/golden/product_receipt_key_order.golden` produced no diff.
- Ownership coverage result:
  - `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests` passed.
  - Output: `god-struct ownership coverage: assigned=132 CreativeAuthoringStore=86 DebugHudStore=5 FrontendWindowShell=16 GameplayStore=1 InputDeviceStore=6 PresentPathStore=6 RoomStore=1 SaveSessionStore=1 ViewportStore=2 app-global-remainder=7 delete=1`.
  - Deleted the five moved top-level ownership rows and added `inputDevice	InputDeviceStore`.
- Required grep results:
  - `rg -n "window\\.(gamepadAvailable|gamepadName|gamepadMapping|lastInputAction|lastInputAccepted)\\b" ...` — no matches.
  - `rg -n "\\b(gamepadAvailable|gamepadName|gamepadMapping|lastInputAction|lastInputAccepted)\\b" src/app/iggy3d/ProductAppWindowState.hpp` — no matches.
  - `rg -n "^(gamepadAvailable|gamepadName|gamepadMapping|lastInputAction|lastInputAccepted)\\b" docs/god_struct_member_ownership.tsv` — no matches.
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
  - E166/E167 remain deferred and unclaimed.
