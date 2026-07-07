# E167 — InputDeviceStore G3: Interaction Mode State

**STATUS: BLOCKED — release only after E166 is reviewed and committed.**
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
