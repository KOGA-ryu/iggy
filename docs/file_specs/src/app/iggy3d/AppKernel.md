# File Spec

Files: `src/app/iggy3d/AppKernel.hpp`, `src/app/iggy3d/AppKernel.cpp`

Verified at: `e3d742db`

## Owns

- Product app-lifetime state bundle and top-level run lifecycle for one valid app invocation.
- Startup sequencing: world template selection, save catalog scan, frontend settings from options, creative facade reset, world setup draft initialization, starter transition, optional auto-new-world, scripted smoke setup, automation control, gameplay tape, window loop, final projection refresh, optional render receipt output, and final exit status.
- The app-level handoff between persistent frontend/window/session/save/creative/settings state and domain modules.

## Does Not Own

- Domain logic for save/load/delete, creative documents, gameplay movement, room editing, input routing, rendering, runtime session ticks, or receipt field construction.
- CLI option parsing.
- Window loop frame processing internals.
- Runtime simulation kernels.

## Reads

- `ProductAppOptions`.
- Product world template from options, save catalog, frontend settings defaults, starter state, automation control path, gameplay tape path, active session state hash, and final window loop result.
- App-lifetime members: `window`, `activeSession`, `creativeApp`, `frontend`, `saves`, `settings`, and `worldSetupDraft`.

## Writes / Mutates

- Mutates all app-lifetime members owned by `AppKernel`.
- Resets `creativeApp.facade` at startup.
- Replaces `window` and `saves` with the final `ProductWindowLoopResult`.
- Writes render receipt to `std::cout` when requested.
- Returns non-zero only for failed requested window creation as owned by current kernel policy.

## Calls Out To / Wires Out To

- `productWorldTemplateFromOptions(...)`.
- `scanProductSaves(...)`.
- `makeProductDefaultWorldSetupDraft(...)` / `makeDefaultWorldSetupDraft(...)`.
- `recordWorldSetupDraftState(...)`.
- `initializeProductStarterTransition(...)`.
- `launchProductNewWorld(...)`.
- `runScriptedProductGameplaySmoke(...)`, `applyProductCameraActions(...)`, and `runProductGameplayTapeFromOptions(...)`.
- `applyProductAutomationControl(...)` and `applyProductAutomationAppCommand(...)`.
- `runProductWindowLoop(...)`.
- `refreshProductGameplayProjectionMetrics(...)`.
- `buildProductAppReceipt(...)` and `formatRenderReceipt(...)`.

## Called By / Entry Points

- `AppShell.cpp` constructs `AppKernel` and calls `AppKernel::run(...)`.
- Focused proof: `rg -n "AppKernel::run|runProductWindowLoop|applyProductAutomationControl|runProductGameplayTapeFromOptions|buildProductAppReceipt" src/app tests`.

## Invariants

- `AppKernel` coordinates domain modules; reusable domain logic must stay outside this file.
- Initial save scan happens before starter initialization; final receipt uses the window-loop result catalog rather than an exit-time rescan.
- Automation command execution runs before gameplay tape and window loop.
- Creative facade is reset before launch/open paths can install documents.
- Projection metrics are refreshed after the window loop before receipt output.
- App-lifetime members are explicit future extraction seams; do not hide new long-lived state in local static globals.

## Tests / Proof Commands

- `rg -n "product_automation_dispatch_tests|product_app_options_tests|product_gameplay_controls_smoke|product_menu_usefulness_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "IGGY3D_PRODUCT_APP_PATH|runProductCase|runProductReceiptCase" tests/smoke cmake/iggy3d_tests.cmake`.
- `rg -n "AppKernel::run|runProductWindowLoop|automation_close_requested" src/app/iggy3d`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/window/Loop.*` unless frame-loop orchestration changes.
- `src/app/iggy3d/automation/*` unless automation scheduling/context changes.
- `src/app/iggy3d/world/*Launch*` unless launch sequencing changes.
- `src/app/iggy3d/ReceiptBuilder.*` and `src/app/iggy3d/receipt/*` unless receipt build inputs change.

## Update When

- App startup order, app-lifetime state ownership, automation/tape/window-loop sequencing, final receipt inputs, or kernel exit policy changes.

## Do Not Update When

- Only a delegated domain module changes behavior behind the same app-kernel call boundary.
