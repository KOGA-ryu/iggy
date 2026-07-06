# E54: Active Creative Identity Mirror Boundary

## Objective

Make `creative::CreativeActiveIdentity` the practical source of truth for active
Creative save/world/document identity, while preserving the existing
`ProductAppWindowState` and RenderReceipt scalar fields as projection/mirror
output.

## Problem

Active Creative identity is currently written in two places on every update:
the creative container identity and the flat window god-struct mirror. The code
explicitly documents this as a temporary lockstep state, but the duplication has
already forked into a pause-save-flow helper.

Evidence:

- `src/app/iggy3d/Operations.cpp:592` says every write lands in both
  `window` and `CreativeActiveIdentity`.
- `src/app/iggy3d/Operations.cpp:597` clears both containers by assigning every
  flat window field.
- `src/app/iggy3d/Operations.cpp:619` records identity by assigning identity
  fields and then assigning the same values into `window`.
- `src/app/iggy3d/Operations.cpp:654` records save results by assigning status,
  dirty flags, and timestamps into both containers, then repeats those fields
  again after accepted saves.
- `src/app/iggy3d/save/Flow.cpp:69` has a separate
  `clearPauseFlowActiveCreativeIdentity(...)` helper with the same flat window
  field reset logic.
- `src/app/iggy3d/ReceiptBuilder.hpp:252` still exposes the active Creative
  identity as many unrelated flat `ProductAppWindowState` fields.

The result is a drift-prone feature seam: every future launch/open/save/return
path has to remember both owners, and tests can pass by asserting the mirror
without proving the source identity is coherent.

## Required Reads

- `src/app/iggy3d/creative/CreativeAppState.hpp`
- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/save/Flow.cpp`
- `src/app/iggy3d/ReceiptBuilder.hpp`
- `src/app/iggy3d/ReceiptBuilder.cpp`
- `tests/unit/product_creative_world_launch_tests.cpp`
- `tests/unit/product_starter_menu_action_tests.cpp`
- `tests/unit/product_save_bridge_tests.cpp`

## Scope

- Introduce a single helper boundary for active Creative identity projection,
  for example:
  - clear identity once, then mirror it to `ProductAppWindowState`;
  - record/update identity once, then mirror it to `ProductAppWindowState`;
  - copy save-result status/dirty/timestamp fields through one shared helper.
- Reuse that boundary from both `Operations.cpp` and pause save flow.
- Keep all externally emitted RenderReceipt keys stable.
- Keep launch/open/save/return-to-title behavior stable.
- Add or update tests so source identity and window mirror cannot drift silently.

## Acceptance

- There is one implementation of active Creative identity clear/record/save-result
  mirroring, shared by launch/open/save and pause-save flow.
- `CreativeActiveIdentity` is the place where identity values are assembled; the
  window fields are treated as receipt/projection mirrors rather than a second
  hand-maintained owner.
- No behavior changes to creative launch, creative open, current-world save,
  pause save flow, active creative identity clearing, or RenderReceipt keys.
- Tests prove both the creative container identity and the window mirror after:
  launch, open, accepted save, failed save/precondition failure, and clear/return
  paths.
- The duplicated clear helper in `save/Flow.cpp` is gone or reduced to calling
  the shared identity boundary.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_starter_menu_action_tests product_save_bridge_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_starter_menu_action_tests|product_save_bridge_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not remove RenderReceipt fields or rename public receipt keys.
- Do not move save identity into `ProductAppWindowState` as the source of truth.
- Do not change save/open/launch semantics or drain dirty flags differently.
- Do not combine this with command-diagnostic field cleanup from E47.

## Completion Brief

Status: done.

Files modified:
- `src/app/iggy3d/Operations.hpp`
- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/save/Flow.cpp`
- `src/app/iggy3d/menu/ActionHandlers.cpp`
- `tests/unit/product_creative_world_launch_tests.cpp`

Implementation:
- Added shared active Creative identity projection helpers:
  - `mirrorProductActiveCreativeIdentity(...)`
  - `clearProductActiveCreativeIdentity(...)`
- Changed launch/open/save identity recording to assemble values on
  `creative::CreativeActiveIdentity` first and mirror those values to
  `ProductAppWindowState`.
- Changed current Creative save to read save id/path/world id from
  `CreativeActiveIdentity`, not the flat window mirror.
- Removed the duplicated pause-save-flow active Creative identity clear helper;
  pause save-and-exit now calls the shared clear boundary.
- Plain pause Return To Title now clears the active Creative identity source and
  window mirror through the shared helper while preserving the existing undo
  clear behavior.
- RenderReceipt/window field names remain unchanged.

Tests:
- Added focused assertions that `CreativeActiveIdentity` and the window mirror
  match after:
  - Creative launch,
  - Creative open,
  - accepted current Creative save,
  - rejected current Creative save preconditions,
  - pause Creative save,
  - pause Creative save-and-exit clear,
  - pause Creative save-and-exit failure,
  - plain pause Return To Title.
- Updated failure setups that intentionally remove the active save id to mutate
  `CreativeActiveIdentity` first, then mirror it to the window.

Verification:
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing-whitespace scan over touched files
- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_starter_menu_action_tests product_save_bridge_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_starter_menu_action_tests|product_save_bridge_tests)$' --output-on-failure`

All verification passed. The focused build surfaced an existing warning in
`tests/unit/product_starter_menu_action_tests.cpp` for an unused local
`facade`; this slice did not modify that file.

Concerns:
- `launchProductNewWorld(...)` still has no `CreativeAppState` parameter, so it
  can clear only the window mirror when leaving Creative for a Product world.
  Paths with a live Creative app available now use the shared source+mirror clear
  helper.
