# E174 — FrontendWindowShell G1: Scalar Menu State

**STATUS: READY.** Parent: `blocked/E164-frontendwindowshell-ruling.md`.

## Goal

Create a `FrontendWindowShell` store and move only the lower-risk scalar
menu/status fields from `ProductAppWindowState` into
`ProductAppWindowState::frontendShell`.

This is deliberately smaller than the full E173 implementation outline. Do not
move the heavier `startup` or `productVulkanMenu` state in this slice.

## Move Exactly These Fields

- `openingMenuVisible`
- `menuTextDrawn`
- `selectedRowDrawn`
- `mouseMenuSelectUsed`
- `gamepadMenuSelectUsed`
- `selectedSettingsTab`
- `launchAction`
- `launchStatus`
- `packageLoadStatus`
- `framesPresented`
- `eventPollCount`
- `menuRowCount`
- `status`

## Scope

Expected production files include, but are not limited to:

- `src/app/iggy3d/ProductAppWindowState.hpp`
- new `src/app/iggy3d/window/FrontendWindowShell.hpp`
- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/save/Flow.cpp`
- `src/app/iggy3d/window/RendererLifecycle.cpp`
- `src/app/iggy3d/window/Loop.cpp`
- `src/app/iggy3d/window/FramePresenter.cpp`
- `src/app/iggy3d/window/InputFrame.cpp`
- `src/app/iggy3d/menu/ActionHandlers.cpp`
- `src/app/iggy3d/ascii_room/Activation.cpp`
- `src/app/iggy3d/receipt/StartupProbeFields.cpp`
- `src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp`
- `src/app/iggy3d/receipt/TailFields.cpp`
- focused product window/startup/menu tests
- `docs/god_struct_member_ownership.tsv`

Use compiler-guided repoints. Do not use broad token replacement. A local
`FrontendWindowShell& shell = window.frontendShell;` alias is acceptable in
dense blocks.

## Required Shape

1. Add a new `iggy3d::FrontendWindowShell` struct under
   `src/app/iggy3d/window/FrontendWindowShell.hpp`.
2. Move the 13 listed fields with exact existing types/defaults.
3. Add `ProductAppWindowState::frontendShell`.
4. Remove the 13 flat fields from `ProductAppWindowState`.
5. Repoint only `ProductAppWindowState` storage reads/writes to
   `window.frontendShell.<field>` or an obvious local shell alias.
6. Update `docs/god_struct_member_ownership.tsv`:
   - delete the 13 moved top-level rows;
   - add `frontendShell	FrontendWindowShell`;
   - retarget `automationControl` from `FrontendWindowShell` to
     `app-global-remainder`.

## Do Not Move

Do not move:

- `startup`
- `productVulkanMenu`
- `automationControl`
- `runtimeStateHash`
- `creativeWorldEpoch`
- `requested`
- `sdlAvailable`
- `created`
- `drawable`
- existing store members such as `inputDevice`, `debugHud`, `creativeAuthoring`,
  `gameplay`, `room`, `saveSession`, `viewport`, and `presentPath`

`automationControl` remains at `window.automationControl`; this card only fixes
its ownership row.

## Required Greps

After implementation, these must produce no output:

```sh
rg -n "window\.(openingMenuVisible|menuTextDrawn|selectedRowDrawn|mouseMenuSelectUsed|gamepadMenuSelectUsed|selectedSettingsTab|launchAction|launchStatus|packageLoadStatus|framesPresented|eventPollCount|menuRowCount|status)\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'

rg -n "\b(openingMenuVisible|menuTextDrawn|selectedRowDrawn|mouseMenuSelectUsed|gamepadMenuSelectUsed|selectedSettingsTab|launchAction|launchStatus|packageLoadStatus|framesPresented|eventPollCount|menuRowCount|status)\b" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp

rg -n "^(openingMenuVisible|menuTextDrawn|selectedRowDrawn|mouseMenuSelectUsed|gamepadMenuSelectUsed|selectedSettingsTab|launchAction|launchStatus|packageLoadStatus|framesPresented|eventPollCount|menuRowCount|status)\b" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
```

Also report these checks:

```sh
rg -n "^frontendShell\\b" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
rg -n "^automationControl\\s+app-global-remainder$" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
rg -n "window\\.(startup|productVulkanMenu|automationControl|runtimeStateHash|creativeWorldEpoch)\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'
```

The last grep should still show the intentionally deferred/retained flat
members where they are used.

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
- the 13 moved fields;
- confirmation that `startup`, `productVulkanMenu`, and `automationControl`
  stayed flat;
- receipt golden result;
- ownership coverage result;
- required grep results;
- full suite result;
- any follow-up needed for `startup` / `productVulkanMenu`.

Do not stage, commit, push, or launch a window.

## Completion Brief

- Exact files changed:
  - `docs/creative_mode/builder_tasks/done/E174-frontendwindowshell-g1-scalar-menu-state.md`
  - `docs/god_struct_member_ownership.tsv`
  - `src/app/iggy3d/ProductAppWindowState.hpp`
  - `src/app/iggy3d/window/FrontendWindowShell.hpp`
  - `src/app/iggy3d/AppKernel.cpp`
  - `src/app/iggy3d/Operations.cpp`
  - `src/app/iggy3d/ascii_room/Activation.cpp`
  - `src/app/iggy3d/automation/Automation.cpp`
  - `src/app/iggy3d/automation/AutomationControl.cpp`
  - `src/app/iggy3d/gameplay/TapeRunner.cpp`
  - `src/app/iggy3d/menu/ActionHandlers.cpp`
  - `src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp`
  - `src/app/iggy3d/receipt/StartupProbeFields.cpp`
  - `src/app/iggy3d/receipt/TailFields.cpp`
  - `src/app/iggy3d/save/Flow.cpp`
  - `src/app/iggy3d/window/FramePresenter.cpp`
  - `src/app/iggy3d/window/InputFrame.cpp`
  - `src/app/iggy3d/window/Loop.cpp`
  - `src/app/iggy3d/window/RendererLifecycle.cpp`
  - `tests/unit/product_automation_dispatch_tests.cpp`
  - `tests/unit/product_creative_ui_command_receipt_tests.cpp`
  - `tests/unit/product_creative_ui_input_frame_tests.cpp`
  - `tests/unit/product_creative_ui_projection_receipt_tests.cpp`
  - `tests/unit/product_creative_viewport_pick_frame_tests.cpp`
  - `tests/unit/product_creative_wireframe_frame_tests.cpp`
  - `tests/unit/product_creative_world_launch_tests.cpp`
  - `tests/unit/product_starter_menu_action_tests.cpp`
  - `tests/unit/product_window_input_frame_tests.cpp`
- The 13 moved fields:
  - `openingMenuVisible`
  - `menuTextDrawn`
  - `selectedRowDrawn`
  - `mouseMenuSelectUsed`
  - `gamepadMenuSelectUsed`
  - `selectedSettingsTab`
  - `launchAction`
  - `launchStatus`
  - `packageLoadStatus`
  - `framesPresented`
  - `eventPollCount`
  - `menuRowCount`
  - `status`
- Store shape:
  - Added `iggy3d::FrontendWindowShell` in
    `src/app/iggy3d/window/FrontendWindowShell.hpp`.
  - Added `ProductAppWindowState::frontendShell`.
  - Removed the 13 moved flat fields from `ProductAppWindowState`.
  - Repointed ProductAppWindowState storage reads/writes to
    `window.frontendShell.<field>` or equivalent context/request access.
- Retained flat fields:
  - `startup` stayed flat.
  - `productVulkanMenu` stayed flat.
  - `automationControl` stayed flat.
  - `runtimeStateHash` and `creativeWorldEpoch` stayed flat.
- Ownership TSV:
  - Removed the 13 moved top-level rows.
  - Added `frontendShell	FrontendWindowShell`.
  - Retargeted `automationControl` to `app-global-remainder`.
- Receipt golden result:
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests`
    passed from repo root.
  - Output: `receipt key-order oracle: 1032 fields match golden (order + values)`.
  - `git diff -- tests/golden/product_receipt_key_order.golden` was empty.
- Ownership coverage result:
  - `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests`
    passed from repo root.
  - Output:
    `god-struct ownership coverage: assigned=18 CreativeAuthoringStore=1 DebugHudStore=1 FrontendWindowShell=3 GameplayStore=1 InputDeviceStore=1 PresentPathStore=1 RoomStore=1 SaveSessionStore=1 ViewportStore=2 app-global-remainder=5 delete=1`.
- Required grep results:
  - Old flat access grep for the 13 moved fields produced no output.
  - ProductAppWindowState declaration grep for the 13 moved fields produced no
    output.
  - TSV old-row grep for the 13 moved fields produced no output.
  - `rg -n "^frontendShell\\b" docs/god_struct_member_ownership.tsv`:
    `5:frontendShell	FrontendWindowShell`
  - `rg -n "^automationControl\\s+app-global-remainder$" docs/god_struct_member_ownership.tsv`:
    `16:automationControl	app-global-remainder`
  - Retained flat-member grep remained non-empty as expected. Direct occurrence
    counts:
    - `startup`: 107
    - `productVulkanMenu`: 121
    - `automationControl`: 133
    - `runtimeStateHash`: 24
    - `creativeWorldEpoch`: 23
- Full suite result:
  - `cmake --build /Users/kogaryu/iggy3d/build -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure`
    passed: 260/260.
- Static checks:
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched/new files produced no output.
- Follow-up needed:
  - `startup` and `productVulkanMenu` are still flat and still TSV-owned by
    `FrontendWindowShell`; they should remain deferred to later, smaller
    slices as requested.
  - `automationControl` is now TSV-owned by `app-global-remainder` and should
    not be moved into FrontendWindowShell.
- No stage, commit, push, or window launch.
