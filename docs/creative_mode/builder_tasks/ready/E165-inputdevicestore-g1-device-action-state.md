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
