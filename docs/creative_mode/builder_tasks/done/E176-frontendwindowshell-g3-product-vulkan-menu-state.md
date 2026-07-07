# E176 - FrontendWindowShell G3: Product Vulkan Menu State

**STATUS: READY.** Parent: `blocked/E164-frontendwindowshell-ruling.md`.
Depends on completed `done/E174-frontendwindowshell-g1-scalar-menu-state.md`
and `done/E175-frontendwindowshell-g2-startup-state.md`.

## Goal

Move only `ProductAppWindowState::productVulkanMenu` into the existing
`ProductAppWindowState::frontendShell`.

This is the final planned FrontendWindowShell field move. E174 moved scalar
menu/status state, and E175 moved `startup`. Keep this card scoped to the
remaining `productVulkanMenu` state plus final FrontendWindowShell docs.

## Move Exactly This Field

- `productVulkanMenu`

The field type and default construction must remain exactly equivalent:
`ProductVulkanMenuState productVulkanMenu;`

## Required Shape

1. Add `ProductVulkanMenuState productVulkanMenu;` to
   `src/app/iggy3d/window/FrontendWindowShell.hpp`.
2. Remove the flat `ProductAppWindowState::productVulkanMenu` member from
   `src/app/iggy3d/ProductAppWindowState.hpp`.
3. Repoint only `ProductAppWindowState` storage reads/writes from
   `window.productVulkanMenu` / `request.window.productVulkanMenu` /
   fixture-window equivalents to `window.frontendShell.productVulkanMenu`.
4. Update `docs/god_struct_member_ownership.tsv`:
   - delete the top-level `productVulkanMenu	FrontendWindowShell` row;
   - keep the existing `frontendShell	FrontendWindowShell` row.
5. Update `docs/god_struct_decomposition_target_map.md`,
   `docs/creative_mode/builder_tasks/PRIORITY.md`, and the parent E164 card to
   mark FrontendWindowShell complete after gates pass.
6. Preserve receipt key names, receipt ordering, and values.

Use compiler-guided repoints. Do not use broad token replacement.

## Do Not Move

Do not move:

- `automationControl`
- `runtimeStateHash`
- `creativeWorldEpoch`
- `requested`
- `sdlAvailable`
- `created`
- `drawable`
- existing store members such as `inputDevice`, `debugHud`, `creativeAuthoring`,
  `gameplay`, `room`, `saveSession`, `viewport`, and `presentPath`

`automationControl` stays flat and TSV-owned by `app-global-remainder`.

## Expected Hot Files

Use the compiler and greps as truth, but expect the hottest production files to
include:

- `src/app/iggy3d/ProductAppWindowState.hpp`
- `src/app/iggy3d/window/FrontendWindowShell.hpp`
- `src/app/iggy3d/window/RendererLifecycle.cpp`
- `src/app/iggy3d/receipt/FeedbackSurfaceAutomationVulkanFields.cpp`
- focused creative UI receipt/window-frame tests that seed or assert Vulkan
  menu fields
- `tests/unit/product_window_renderer_lifecycle_tests.cpp`

## Required Greps

After implementation, these must produce no output:

```sh
rg -n "window\\.productVulkanMenu\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'

rg -n "\\bproductVulkanMenu\\b" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp

rg -n "^productVulkanMenu\\b" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
```

Also report these checks:

```sh
rg -n "^frontendShell\\b" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
rg -n "^automationControl\\s+app-global-remainder$" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
rg -n "window\\.(automationControl|runtimeStateHash|creativeWorldEpoch)\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'
```

The final grep should remain non-empty because those fields are intentionally
retained/deferred outside FrontendWindowShell.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests
ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

`git diff -- tests/golden/product_receipt_key_order.golden` must be empty.

## Completion Brief

Report:

- exact files changed;
- confirmation that only `productVulkanMenu` moved;
- confirmation that `automationControl`, `runtimeStateHash`, and
  `creativeWorldEpoch` stayed flat;
- receipt golden result;
- ownership coverage result;
- required grep results;
- full suite result;
- whether FrontendWindowShell is now marked complete in target-map/priority
  docs.

Do not stage, commit, push, or launch a window.

## Completion Brief

- Files changed:
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
  - `docs/creative_mode/builder_tasks/blocked/E164-frontendwindowshell-ruling.md`
  - `docs/god_struct_decomposition_target_map.md`
  - `docs/god_struct_member_ownership.tsv`
  - `src/app/iggy3d/ProductAppWindowState.hpp`
  - `src/app/iggy3d/receipt/FeedbackSurfaceAutomationVulkanFields.cpp`
  - `src/app/iggy3d/window/FrontendWindowShell.hpp`
  - `src/app/iggy3d/window/RendererLifecycle.cpp`
  - `tests/unit/product_creative_ui_command_receipt_tests.cpp`
  - `tests/unit/product_creative_ui_frame_tests.cpp`
  - `tests/unit/product_creative_ui_input_frame_tests.cpp`
  - `tests/unit/product_creative_ui_projection_receipt_tests.cpp`
  - `tests/unit/product_creative_ui_window_frame_tests.cpp`
  - `tests/unit/product_creative_viewport_pick_frame_tests.cpp`
  - `tests/unit/product_creative_wireframe_frame_tests.cpp`
  - `tests/unit/product_window_renderer_lifecycle_tests.cpp`
  - this task card after move to `done/`
- Move scope:
  - Moved only `ProductAppWindowState::productVulkanMenu` into `ProductAppWindowState::frontendShell.productVulkanMenu`.
  - Added `ProductVulkanMenuState productVulkanMenu` to `FrontendWindowShell`.
  - Removed the flat `ProductAppWindowState::productVulkanMenu` member and its direct include.
  - Repointed existing production/test accesses from `window.productVulkanMenu` to `window.frontendShell.productVulkanMenu`.
- Explicit non-moves:
  - `automationControl`, `runtimeStateHash`, and `creativeWorldEpoch` stayed flat.
  - SDL/window lifecycle fields stayed flat.
  - `presentPath` and other existing stores were not moved or reshaped.
- Receipt golden:
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests` passed.
  - Output: `receipt key-order oracle: 1032 fields match golden (order + values)`.
  - `git diff -- tests/golden/product_receipt_key_order.golden` was empty.
- Ownership coverage:
  - `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests` passed.
  - Output: `god-struct ownership coverage: assigned=16 CreativeAuthoringStore=1 DebugHudStore=1 FrontendWindowShell=1 GameplayStore=1 InputDeviceStore=1 PresentPathStore=1 RoomStore=1 SaveSessionStore=1 ViewportStore=2 app-global-remainder=5 delete=1`.
- Required grep results:
  - `rg -n "window\\.productVulkanMenu\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'` produced no output.
  - `rg -n "\\bproductVulkanMenu\\b" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp` produced no output.
  - `rg -n "^productVulkanMenu\\b" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv` produced no output.
  - `rg -n "^frontendShell\\b" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv` reported `frontendShell	FrontendWindowShell`.
  - `rg -n "^automationControl\\s+app-global-remainder$" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv` reported `automationControl	app-global-remainder`.
  - Retained/deferred flat-field scan remained non-empty as expected: `automationControl` 133 hits, `creativeWorldEpoch` 23 hits, `runtimeStateHash` 24 hits.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure` passed: 260/260.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files passed.
- Docs/state:
  - `docs/god_struct_member_ownership.tsv` no longer has a `productVulkanMenu` top-level row; ownership is via `frontendShell	FrontendWindowShell`.
  - `docs/god_struct_decomposition_target_map.md`, `PRIORITY.md`, and parent `E164` now mark FrontendWindowShell complete as E174-E176.
- Concerns:
  - None for E176. No stage, commit, push, or window launch performed.
