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
