# E175 - FrontendWindowShell G2: Startup State

**STATUS: READY.** Parent: `blocked/E164-frontendwindowshell-ruling.md`.
Depends on completed `done/E174-frontendwindowshell-g1-scalar-menu-state.md`.

## Goal

Move only `ProductAppWindowState::startup` into the existing
`ProductAppWindowState::frontendShell`.

E174 already created `iggy3d::FrontendWindowShell` and moved the lower-risk
scalar/menu/status fields. This card should continue that same structural
FrontendWindowShell lane without touching `productVulkanMenu` or unrelated
window lifecycle fields.

## Move Exactly This Field

- `startup`

The field type and default construction must remain exactly equivalent:
`ProductStartupState startup;`

## Required Shape

1. Add `ProductStartupState startup;` to
   `src/app/iggy3d/window/FrontendWindowShell.hpp`.
2. Remove the flat `ProductAppWindowState::startup` member from
   `src/app/iggy3d/ProductAppWindowState.hpp`.
3. Repoint only `ProductAppWindowState` storage reads/writes from
   `window.startup` / `request.window.startup` / fixture-window equivalents to
   `window.frontendShell.startup`.
4. Update `docs/god_struct_member_ownership.tsv`:
   - delete the top-level `startup	FrontendWindowShell` row;
   - keep the existing `frontendShell	FrontendWindowShell` row.
5. Preserve receipt key names, receipt ordering, and values.

Use compiler-guided repoints. Do not use broad token replacement; `startup` is
too generic.

## Do Not Move

Do not move:

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

`productVulkanMenu` remains the next deferred FrontendWindowShell slice.
`automationControl` stays flat and TSV-owned by `app-global-remainder`.

## Expected Hot Files

Use the compiler and greps as truth, but expect the hottest production files to
include:

- `src/app/iggy3d/ProductAppWindowState.hpp`
- `src/app/iggy3d/window/FrontendWindowShell.hpp`
- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/window/Loop.cpp`
- `src/app/iggy3d/receipt/StartupProbeFields.cpp`
- focused product creative/world launch and wireframe tests that seed/assert
  startup receipt data

## Required Greps

After implementation, these must produce no output:

```sh
rg -n "window\\.startup\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'

rg -n "\\bstartup\\b" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp

rg -n "^startup\\b" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
```

Also report these checks:

```sh
rg -n "^frontendShell\\b" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
rg -n "^automationControl\\s+app-global-remainder$" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
rg -n "window\\.(productVulkanMenu|automationControl|runtimeStateHash|creativeWorldEpoch)\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'
```

The final grep should remain non-empty because those fields are intentionally
retained/deferred.

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
- confirmation that only `startup` moved;
- confirmation that `productVulkanMenu`, `automationControl`,
  `runtimeStateHash`, and `creativeWorldEpoch` stayed flat;
- receipt golden result;
- ownership coverage result;
- required grep results;
- full suite result;
- any follow-up needed for `productVulkanMenu`.

Do not stage, commit, push, or launch a window.

## Completion Brief

- Exact files changed:
  - `docs/creative_mode/builder_tasks/done/E175-frontendwindowshell-g2-startup-state.md`
  - `docs/god_struct_member_ownership.tsv`
  - `src/app/iggy3d/ProductAppWindowState.hpp`
  - `src/app/iggy3d/window/FrontendWindowShell.hpp`
  - `src/app/iggy3d/Operations.cpp`
  - `src/app/iggy3d/receipt/StartupProbeFields.cpp`
  - `src/app/iggy3d/window/FramePresenter.cpp`
  - `src/app/iggy3d/window/Loop.cpp`
  - `tests/unit/product_creative_wireframe_frame_tests.cpp`
  - `tests/unit/product_creative_world_launch_tests.cpp`
- Moved field:
  - Only `startup` moved.
  - Added `ProductStartupState startup;` to
    `ProductAppWindowState::frontendShell`.
  - Removed flat `ProductAppWindowState::startup`.
  - Repointed ProductAppWindowState storage reads/writes from
    `window.startup` / `request.window.startup` to
    `window.frontendShell.startup`.
- Retained flat fields:
  - `productVulkanMenu` stayed flat.
  - `automationControl` stayed flat.
  - `runtimeStateHash` stayed flat.
  - `creativeWorldEpoch` stayed flat.
- Ownership TSV:
  - Removed the top-level `startup	FrontendWindowShell` row.
  - Kept `frontendShell	FrontendWindowShell`.
  - Kept `automationControl	app-global-remainder`.
- Receipt golden result:
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests`
    passed from repo root.
  - Output: `receipt key-order oracle: 1032 fields match golden (order + values)`.
  - `git diff -- tests/golden/product_receipt_key_order.golden` was empty.
- Ownership coverage result:
  - `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests`
    passed from repo root.
  - Output:
    `god-struct ownership coverage: assigned=17 CreativeAuthoringStore=1 DebugHudStore=1 FrontendWindowShell=2 GameplayStore=1 InputDeviceStore=1 PresentPathStore=1 RoomStore=1 SaveSessionStore=1 ViewportStore=2 app-global-remainder=5 delete=1`.
- Required grep results:
  - `rg -n "window\\.startup\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'`
    produced no output.
  - `rg -n "\\bstartup\\b" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp`
    produced no output.
  - `rg -n "^startup\\b" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv`
    produced no output.
  - `rg -n "^frontendShell\\b" docs/god_struct_member_ownership.tsv`:
    `5:frontendShell	FrontendWindowShell`
  - `rg -n "^automationControl\\s+app-global-remainder$" docs/god_struct_member_ownership.tsv`:
    `15:automationControl	app-global-remainder`
  - Retained flat-member grep stayed non-empty as expected. Direct occurrence
    counts:
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
  - `productVulkanMenu` remains the only deferred field still TSV-owned by
    `FrontendWindowShell`.
  - `automationControl` remains flat and TSV-owned by `app-global-remainder`.
- No stage, commit, push, or window launch.
