# AppShell Shell Refactor Audit v0.1

## Objective

Make `src/app/iggy3d/AppShell.cpp` a shell.

That means `AppShell` should own only:

- process/app lifecycle;
- option parsing result handling;
- top-level state objects;
- dependency wiring;
- window/no-window loop entry;
- calling product controllers;
- applying typed controller results;
- final receipt emission.

It should not own:

- save/load policy;
- Continue/Load/Delete/Recover selection behavior;
- automation key behavior;
- frontend/menu screen behavior;
- world setup and dungeon-draft behavior;
- room editing command behavior;
- projection/debug/render proof copying;
- receipt field-by-field projection;
- render geometry policy.

The fast path is not a rewrite. The fast path is extraction by ownership:

1. identify an AppShell branch corridor;
2. find the existing lower-level owner file;
3. add the missing flow/controller file beside that owner;
4. move one corridor behind a typed request/result boundary;
5. prove exact receipt and smoke parity;
6. add a branch-gate rule blocking that corridor from returning to AppShell.

## Hard Rules

- No feature expansion during shell-refactor slices.
- No behavior change unless the slice explicitly documents and proves a repair.
- No save/load rewrite. Reuse `SaveBridge`, `ProductSaveCatalog`, and `ProductAppOperations`.
- No room editor rewrite. Reuse `ProductRoomEditingState`, `ProductRoomEditorCursor`, and `ProductRoomEditorActionController`.
- No renderer/Vulkan backend rewrite. Move product projection orchestration only.
- No NPC behavior work.
- ASCII remains map/layout authoring only.
- Visual demo stays retired.
- Branch gate must run on source slices.
- New AppShell branches require explicit controller/user approval and an audit note naming why AppShell is the correct owner.

## Current AppShell Clusters

This audit treats the current product spine as these clusters:

| AppShell cluster | Current responsibility | Correct owner |
| --- | --- | --- |
| `applyOpeningMenuAction(...)` | menu/screen actions, pause save, Continue, Load Save, New World, delete confirm, settings/dev tools | `ProductFrontendFlowController`, `ProductSaveFlowController`, `ProductWorldLaunchController`, `ProductRoomEditingFlowController` |
| `routeOpeningMenuInput(...)` | input route request assembly and receipt/result copying | `ProductFrontendFlowController` |
| automation parser helpers | command file parsing, bool/float/csv parsing, text-to-action mapping | `ProductAutomationParser`, `ProductAutomationCommandRegistry` |
| `applyProductAutomationCommand(...)` | automation dispatch and domain command behavior | `ProductAutomationController` plus domain controllers |
| room editing record/copy helpers | copying room editing results into window, active room, collision, editor cursor receipts | `ProductRoomEditingFlowController` |
| world setup draft helpers | world title/dungeon draft status, cursor, draft paint/move/selection | `ProductWorldSetupFlowController` |
| save browser automation | select/delete/show-deleted/recover behavior | `ProductSaveFlowController` |
| gameplay tape helpers | parse/run tape and copy proof fields | `ProductGameplayTapeController` or `ProductGameplayProofController` |
| projection metrics helpers | scene/debug/primitive/frame/render bridge assembly and field copying | `ProductProjectionPipeline` |
| run loop assembly | polling, window frame, render submit, refreshes | stays partly in `AppShell`, but calls controllers |
| final runProductApp flow | option handling, initial scan, automation/tape/window/receipt sequence | stays in `AppShell`, but save scan and flow calls move behind controllers |

## Existing Owner Files

These files already exist and should be reused.

### Save And Load

Existing files:

- `src/app/iggy3d/SaveBridge.hpp`
- `src/app/iggy3d/SaveBridge.cpp`
- `src/app/iggy3d/ProductSaveCatalog.hpp`
- `src/app/iggy3d/ProductSaveCatalog.cpp`
- `src/app/iggy3d/ProductAppOperations.hpp`
- `src/app/iggy3d/ProductAppOperations.cpp`

What they already own:

- active save scan;
- deleted save scan;
- durable session save write;
- product save load;
- soft delete;
- recover;
- current-session save helper;
- selected active/deleted slot helpers;
- Continue and Load Save launch helpers;
- New World launch helper.

What is missing:

- a flow-level save controller that owns the frontend/window decisions around those helpers.

Add:

- `src/app/iggy3d/ProductSaveFlowController.hpp`
- `src/app/iggy3d/ProductSaveFlowController.cpp`
- `tests/unit/product_save_flow_controller_tests.cpp`

The new controller should own:

- pause Save;
- pause Save And Exit;
- Continue;
- Load Save selector confirm;
- Load Save row movement;
- Delete confirmation open/cancel/execute;
- Show Deleted;
- deleted row select;
- Recover;
- save scan refresh result projection.

AppShell after extraction:

- passes `ProductSaveFlowRequest`;
- applies `ProductSaveFlowResult`;
- does not inspect save-slot policy.

### Frontend And Menu Routing

Existing files:

- `src/app/iggy3d/ProductFrontendRouter.hpp`
- `src/app/iggy3d/ProductFrontendRouter.cpp`
- `src/app/iggy3d/ProductMenuTransitions.hpp`
- `src/app/iggy3d/ProductMenuTransitions.cpp`
- `src/app/frontend/StarterScreen.hpp`
- `src/app/frontend/PauseMenu.hpp`
- `src/app/frontend/WorldSetupModel.hpp`
- `src/app/frontend/SaveBrowser.hpp`
- `src/app/frontend/SettingsMenu.hpp`
- `src/app/frontend/DevToolsMenu.hpp`

What they already own:

- frontend owner selection;
- pure route summary;
- starter screen model;
- pause menu model;
- world setup route result;
- save browser route result;
- settings/dev tools models.

What is missing:

- a flow controller that applies a routed frontend action to product state.

Add:

- `src/app/iggy3d/ProductFrontendFlowController.hpp`
- `src/app/iggy3d/ProductFrontendFlowController.cpp`
- `tests/unit/product_frontend_flow_controller_tests.cpp`

The new controller should own:

- SystemPause behavior;
- Pause action behavior;
- Settings/DevTools open/close behavior;
- New World child-screen behavior;
- Load Save child-screen behavior;
- starter action behavior;
- close-request decisions.

It should call narrower controllers for save/world/room editor actions instead of implementing their policy inline.

### Automation

Existing files:

- `src/app/iggy3d/ProductAutomationCommandRegistry.hpp`
- `src/app/iggy3d/ProductAutomationCommandRegistry.cpp`

What it already owns:

- canonical command keys;
- aliases;
- category names;
- value-kind names;
- basic command metadata.

What is missing:

- a parser module for reading command files;
- a controller that dispatches canonical commands by category;
- typed automation value parsing descriptors.

Add:

- `src/app/iggy3d/ProductAutomationParser.hpp`
- `src/app/iggy3d/ProductAutomationParser.cpp`
- `src/app/iggy3d/ProductAutomationController.hpp`
- `src/app/iggy3d/ProductAutomationController.cpp`
- `tests/unit/product_automation_parser_tests.cpp`
- `tests/unit/product_automation_controller_tests.cpp`

The new controller should own:

- command file load result;
- duplicate key handling;
- owner checks;
- parse failure versus domain command failure;
- `markAutomationApplied` equivalent;
- dispatch to frontend/save/world/room/editor/gameplay/settings/dev-tools/system controllers.

AppShell after extraction:

- calls `applyProductAutomationControl(...)`;
- receives typed automation result;
- does not know individual key strings.

### World Setup And Map/Dungeon Draft

Existing files:

- `src/app/frontend/WorldSetupModel.hpp`
- `src/app/frontend/WorldSetupModel.cpp`
- `src/app/iggy3d/ProductDungeonDraft.hpp`
- `src/app/iggy3d/ProductDungeonDraft.cpp`
- `src/app/iggy3d/ProductBuiltinDungeon.hpp`
- `src/app/iggy3d/ProductBuiltinDungeon.cpp`
- `src/app/iggy3d/ProductWorldCreation.hpp`
- `src/app/iggy3d/ProductWorldCreation.cpp`

What they already own:

- world setup validation;
- world title/seed/create request;
- dungeon draft model;
- draft cursor move/paint;
- built-in dungeon selection;
- product world creation request/result;
- initial save write request/result.

What is missing:

- a product flow controller that owns New World screen state, draft edit mode, built-in dungeon selection, and create-world launch orchestration.

Add:

- `src/app/iggy3d/ProductWorldSetupFlowController.hpp`
- `src/app/iggy3d/ProductWorldSetupFlowController.cpp`
- `src/app/iggy3d/ProductWorldLaunchController.hpp`
- `src/app/iggy3d/ProductWorldLaunchController.cpp`
- `tests/unit/product_world_setup_flow_controller_tests.cpp`
- `tests/unit/product_world_launch_controller_tests.cpp`

The launch controller should keep using:

- `ProductWorldCreation`;
- `ProductPackageSessionSeed`;
- `ProductAppOperations::launchProductNewWorld` until that helper can be narrowed or moved.

### Room Editing

Existing files:

- `src/app/iggy3d/ProductRoomEditingState.hpp`
- `src/app/iggy3d/ProductRoomEditingState.cpp`
- `src/app/iggy3d/ProductRoomAuthoringController.hpp`
- `src/app/iggy3d/ProductRoomAuthoringController.cpp`
- `src/app/iggy3d/ProductRoomEditorCursor.hpp`
- `src/app/iggy3d/ProductRoomEditorCursor.cpp`
- `src/app/iggy3d/ProductRoomEditorActionController.hpp`
- `src/app/iggy3d/ProductRoomEditorActionController.cpp`
- `src/app/iggy3d/ProductRoomEditorOverlay.hpp`
- `src/app/iggy3d/ProductRoomEditorOverlay.cpp`

What they already own:

- editing state;
- edit command application;
- undo/redo;
- cursor movement;
- tool selection;
- editor action-to-command behavior;
- overlay model.

What is missing:

- a product room editing flow controller that owns the window/active-room/collision projection after edit operations.

Add:

- `src/app/iggy3d/ProductRoomEditingFlowController.hpp`
- `src/app/iggy3d/ProductRoomEditingFlowController.cpp`
- `tests/unit/product_room_editing_flow_controller_tests.cpp`

The new controller should own:

- start from ASCII;
- start from active room;
- apply script edit;
- apply cursor edit;
- undo/redo;
- copying result into active room/collision/editor cursor receipt fields.

AppShell after extraction:

- passes a room editing flow request;
- applies result;
- does not call `applyProductRoomEditingCommand` directly.

### Projection And Render Proof

Existing files:

- `src/projection/scene/SceneProjection.hpp`
- `src/projection/debug/DebugProjection.hpp`
- `src/app/iggy3d/ProductPrimitiveDrawList.hpp`
- `src/app/iggy3d/ProductPrimitiveDrawList.cpp`
- `src/app/iggy3d/ProductRenderBridge.hpp`
- `src/app/iggy3d/ProductRenderBridge.cpp`
- `src/app/iggy3d/ProductViewportFraming.hpp`
- `src/app/iggy3d/ProductViewportFraming.cpp`
- `src/app/iggy3d/ProductMovementDebugHud.hpp`
- `src/app/iggy3d/ProductNpcBehaviorDebugHud.hpp`

What they already own:

- scene projection;
- debug projection;
- product primitive draw list;
- render bridge frame;
- viewport framing;
- debug HUD models.

What is missing:

- one product projection pipeline that assembles those models and returns a single result/patch.

Add:

- `src/app/iggy3d/ProductProjectionPipeline.hpp`
- `src/app/iggy3d/ProductProjectionPipeline.cpp`
- `tests/unit/product_projection_pipeline_tests.cpp`

The new pipeline should own:

- inactive gameplay projection clearing;
- active room pointer selection;
- scene projection call;
- debug projection call;
- room editor overlay model;
- primitive draw list;
- viewport frame;
- render bridge;
- product window proof-field patch.

AppShell after extraction:

- calls `refreshProductProjectionPipeline(...)`;
- does not copy draw-list/frame/bridge fields one by one.

### Receipt Projection

Existing files:

- `src/app/iggy3d/ReceiptBuilder.hpp`
- `src/app/iggy3d/ReceiptBuilder.cpp`

What it already owns:

- final receipt formatting from `ProductAppWindowState` and save scan result.

What is missing:

- descriptor-driven field appenders;
- small result-to-window projectors so AppShell does not hand-copy proof fields.

Add later:

- `src/app/iggy3d/ProductReceiptProjection.hpp`
- `src/app/iggy3d/ProductReceiptProjection.cpp`

This is lower priority than moving flow decisions.

## Aggressive Refactor Method

Use extraction in this order:

1. wrap existing code without changing behavior;
2. add exact parity tests;
3. move one branch corridor at a time;
4. shrink AppShell call sites;
5. run branch gate;
6. add a guard rule blocking that corridor from being re-added.

Do not start by making abstractions. Start by moving existing policy to the owner that should already have owned it.

## Slice Ladder

### Slice 0 - Audit Lock

Allowed files:

- docs only;
- optional audit script only.

DoD:

- existing owner files listed;
- missing files listed;
- AppShell clusters listed;
- no behavior change;
- no source dispatch.

### Slice 1 - ProductSaveFlowController Model Only

Allowed files:

- `src/app/iggy3d/ProductSaveFlowController.hpp`
- `src/app/iggy3d/ProductSaveFlowController.cpp`
- `tests/unit/product_save_flow_controller_tests.cpp`
- CMake test registration.

DoD:

- defines request/result/status enums for save flow;
- describes commands for PauseSave, PauseSaveAndExit, Continue, LoadSelected, SelectRow, OpenDeleteConfirm, CancelDeleteConfirm, ExecuteSoftDelete, OpenDeletedBrowser, SelectDeletedRow, RecoverDeleted;
- does not touch AppShell;
- tests prove request/result defaults and status names.

### Slice 2 - Move Pause Save And Save And Exit

Allowed files:

- `ProductSaveFlowController.*`;
- `AppShell.cpp`;
- focused pause/save smokes.

DoD:

- AppShell no longer calls `writeProductCurrentSessionSave(...)` directly for pause Save or Save And Exit;
- controller calls existing `ProductAppOperations` helper;
- receipt values remain byte-for-byte compatible for existing smokes;
- branch gate passes;
- no save schema/bridge/runtime changes.

### Slice 3 - Move Continue And Load Save Confirm

Allowed files:

- `ProductSaveFlowController.*`;
- `AppShell.cpp`;
- save/load smokes.

DoD:

- AppShell no longer branches on Continue save-slot compatibility;
- AppShell no longer calls `launchProductContinueSave(...)` or `launchProductLoadSaveSelection(...)` directly;
- controller owns disabled/no-compatible status;
- existing Continue/Load Save smokes pass unchanged.

### Slice 4 - Move Save Browser Delete/Recover Flow

Allowed files:

- `ProductSaveFlowController.*`;
- `AppShell.cpp`;
- save browser smokes.

DoD:

- AppShell no longer calls delete/recover/select deleted-save helpers directly;
- controller owns active/deleted selection state transitions;
- soft-delete/recover receipts remain unchanged;
- deleted save browser smokes pass.

### Slice 5 - ProductFrontendFlowController Model Only

Allowed files:

- `ProductFrontendFlowController.*`;
- tests only.

DoD:

- request/result model can represent system pause, menu input, starter confirm, child-screen input, and close request;
- result carries frontend patch intent, window patch intent, and delegated action kind;
- no AppShell integration yet.

### Slice 6 - Move Pause/Overlay Routing

Allowed files:

- `ProductFrontendFlowController.*`;
- `AppShell.cpp`;
- pause/frontend tests/smokes.

DoD:

- AppShell no longer owns pause Resume/EditRoom/Settings/DevTools/ReturnToTitle/ExitGame branches;
- save commands delegate to `ProductSaveFlowController`;
- room edit command delegates to `ProductRoomEditingFlowController` when available, or a temporary adapter function with a removal note.

### Slice 7 - Move Starter/NewWorld/LoadSave Routing

Allowed files:

- `ProductFrontendFlowController.*`;
- `ProductWorldSetupFlowController.*` if needed;
- `AppShell.cpp`;
- focused smokes.

DoD:

- AppShell no longer contains starter selected-action corridor;
- New World open/back/create behavior moves to controller;
- Load Save input behavior delegates to save flow controller;
- existing world creation and save-load smokes pass.

### Slice 8 - ProductAutomationParser

Allowed files:

- `ProductAutomationParser.*`;
- parser tests.

DoD:

- file reading, duplicate-key detection, malformed line detection, and loaded/error status leave AppShell;
- no command execution yet;
- exact existing status strings preserved.

### Slice 9 - ProductAutomationController Dispatch

Allowed files:

- `ProductAutomationController.*`;
- `AppShell.cpp`;
- automation tests/smokes.

DoD:

- AppShell no longer knows individual automation key strings;
- registry category dispatch is table-driven;
- unsupported commands return unknown_key;
- valid parse/domain failures preserve loaded semantics;
- no feature behavior changes.

### Slice 10 - ProductRoomEditingFlowController

Allowed files:

- `ProductRoomEditingFlowController.*`;
- `AppShell.cpp`;
- room editing tests/smokes.

DoD:

- AppShell no longer copies room editing result fields directly;
- active room/collision refresh happens inside room editing flow result;
- cursor and overlay proof remain intact;
- edit/save/continue smokes pass.

### Slice 11 - ProductProjectionPipeline

Allowed files:

- `ProductProjectionPipeline.*`;
- `AppShell.cpp`;
- projection/render bridge tests.

DoD:

- AppShell no longer owns `applyGameplayProjectionMetrics(...)`;
- projection pipeline returns one result/patch;
- product draw/render bridge fields remain unchanged;
- room mesh CPU geometry proof remains unchanged.

### Slice 12 - AppShell Shrink Gate

Allowed files:

- `AppShell.cpp`;
- `tools/check_branch_gate.py`;
- branch-gate docs/tests.

DoD:

- `AppShell.cpp` is below an agreed line budget;
- no save/load key strings in AppShell;
- no room editor command key strings in AppShell;
- no frontend selected-action branch corridor in AppShell;
- no projection field-copy corridor in AppShell;
- branch gate fails if these corridors are reintroduced.

## Fast Correctness Gates

For every extraction slice:

- run the relevant focused unit tests;
- run the relevant smoke;
- run `git diff --check`;
- run `tools/check_branch_gate.py`;
- run `tools/check_branch_gate.py --diff HEAD~1..HEAD` after commit when committed;
- compare receipt keys and high-value receipt values before/after when moving product flow.

## What Remains In AppShell

After the refactor, AppShell can still contain direct guards for:

- option parse help/failure;
- window requested versus no-window mode;
- renderer requested versus null renderer;
- SDL window creation and shutdown;
- top-level loop exit;
- final receipt printing;
- controller result application.

Those are lifecycle responsibilities, not product policy.

## What Must Leave AppShell

These must move:

- `writeProductCurrentSessionSave(...)` call sites;
- `launchProductContinueSave(...)` call sites;
- `launchProductLoadSaveSelection(...)` call sites;
- save row select/delete/recover call sites;
- individual automation key handling;
- world setup draft edit-mode and paint/move policy;
- room edit command application;
- product draw/projection proof copying;
- frontend selected-action corridors.

## Review Checklist

Before dispatching a refactor slice, ask:

- Which AppShell corridor is being removed?
- Which existing owner file already owns the lower-level behavior?
- Which missing controller file is being added?
- What receipt fields must remain unchanged?
- Which smoke proves user-visible parity?
- What branch-gate rule prevents the corridor from coming back?

## Stop Rules

Stop and repair the plan if:

- a slice needs to change save schema;
- a slice needs to change room geometry behavior;
- a slice needs to change user-facing flow status strings without a repair note;
- a slice adds new feature behavior;
- a slice makes AppShell larger except for a temporary adapter with a same-slice or next-slice removal gate.

