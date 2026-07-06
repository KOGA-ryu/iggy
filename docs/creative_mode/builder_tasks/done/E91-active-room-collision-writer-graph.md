# E91: Active Room And Collision Writer Graph

## Objective

Create a concrete writer/read graph for `window.activeRoom` and
`window.activeRoomCollision` before any active-room ownership migration.

## Problem

E88 classified active room and collision as derived runtime state that currently
lives on `ProductAppWindowState`. The risk is drift: a path can update room
without collision, or collision can stay ready for a different room.

This needs a graph before a service extraction.

## Required Reads

- `docs/creative_mode/post_claude_architecture_review_tally.md`
- `docs/creative_mode/builder_tasks/done/E88-product-window-state-owner-classification.md`
- `src/app/iggy3d/ReceiptBuilder.hpp`
- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/gameplay/ActiveRoomState.hpp`
- `src/app/iggy3d/gameplay/ActiveRoomState.cpp`
- `src/app/iggy3d/gameplay/ActiveRoomCollision.hpp`
- `src/app/iggy3d/gameplay/ActiveRoomCollision.cpp`
- `src/app/iggy3d/window/InputFrame.cpp`
- `src/app/iggy3d/ascii_room/Activation.cpp`
- `src/app/iggy3d/room_editor/EditingState.cpp`
- active-room/collision focused tests.

## Scope

Read-only audit only.

Produce a completion brief that lists:

- every production writer of `window.activeRoom`,
- every production writer of `window.activeRoomCollision`,
- every production reader that depends on them being in sync,
- each install/clear path's source data,
- stale-state failure cases,
- the smallest future service boundary for an atomic room+collision install.

## Do Not

- Do not edit production code.
- Do not change active-room or collision behavior.
- Do not change RoomBake, package/ascii room activation, room-editor behavior,
  gameplay controller, or receipts.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Acceptance

- Completion brief gives exact file/function writer and reader list.
- It identifies at least two stale room/collision cases a future test should
  pin.
- It recommends one first implementation slice, but does not create that slice.

## Suggested Checks

```sh
rg -n "activeRoom|activeRoomCollision|buildProductActiveRoom|buildProductActiveRoomCollision" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests/unit
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Files inspected:
- Active-room writers:
- Active-room-collision writers:
- Sync-dependent readers:
- Stale-state cases:
- First future service boundary:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief - 2026-07-06

- Files inspected:
  - `docs/creative_mode/post_claude_architecture_review_tally.md`
  - `docs/creative_mode/builder_tasks/done/E88-product-window-state-owner-classification.md`
  - `src/app/iggy3d/ReceiptBuilder.hpp`
  - `src/app/iggy3d/ReceiptBuilder.cpp`
  - `src/app/iggy3d/Operations.cpp`
  - `src/app/iggy3d/gameplay/ActiveRoomState.hpp`
  - `src/app/iggy3d/gameplay/ActiveRoomState.cpp`
  - `src/app/iggy3d/gameplay/ActiveRoomCollision.hpp`
  - `src/app/iggy3d/gameplay/ActiveRoomCollision.cpp`
  - `src/app/iggy3d/window/InputFrame.cpp`
  - `src/app/iggy3d/ascii_room/Activation.cpp`
  - `src/app/iggy3d/room_editor/EditingState.cpp`
  - `src/app/iggy3d/gameplay/Controller.cpp`
  - `src/app/iggy3d/gameplay/TapeRunner.cpp`
  - `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
  - `src/app/iggy3d/automation/AutomationRoomEditing.cpp`
  - `src/app/iggy3d/menu/ActionHandlers.cpp`
  - active-room/collision focused tests, especially
    `product_active_room_state_tests.cpp`,
    `product_ascii_room_activation_tests.cpp`,
    `product_creative_world_launch_tests.cpp`,
    `product_room_editing_state_tests.cpp`,
    `product_gameplay_controller_tests.cpp`, and
    `product_window_input_frame_tests.cpp`.
- Active-room writers:
  - `Operations.cpp:createProductSessionFromPackage(...)`
    - Clears `window.activeRoom` and `window.activeRoomCollision`.
    - If the package has rooms, installs `buildProductActiveRoomFromPackageRoom(...)`.
  - `Operations.cpp:createCreativeBlankSession(...)`
    - Clears `window.activeRoom` and `window.activeRoomCollision` for the blank
      Creative stage.
  - `Operations.cpp:clearProductGameplayLaunchState(...)`
    - Clears `window.activeRoom` and `window.activeRoomCollision` when launch
      fails/tears down gameplay state.
  - `Operations.cpp:ProductCreativeBakedActiveRoomRefreshExecutor::handleRejectedBake(...)`
    - On `clearOnNoRenderable`, installs `clearedCreativeBakedActiveRoom(...)`.
  - `Operations.cpp:ProductCreativeBakedActiveRoomRefreshExecutor::installBakedRoom(...)`
    - Installs a `ProductActiveRoomState` from RoomBake `RoomAsset`.
  - `Operations.cpp:launchProductNewWorld(...)`
    - For ASCII-authored new worlds, installs
      `buildProductActiveRoomFromAsciiAuthoring(...)`.
  - `Operations.cpp:loadProductSessionSaveIntoRuntime(...)`
    - If a save has authored room data, installs
      `buildProductActiveRoomFromSavedAuthoredRoom(...)`.
  - `ascii_room/Activation.cpp:activateProductAsciiRoomPreview(...)`
    - Installs ASCII preview active room before package/session activation.
  - `automation/AutomationRoomEditing.cpp:copyRoomEditingStateToWindow(...)`
    - Copies `state.activeRoom` to `window.activeRoom` when room-editing state
      is ready.
  - Non-window room-editor state writer:
    `room_editor/EditingState.cpp:buildProductRoomEditingState(...)` rebuilds
    `ProductRoomEditingState::activeRoom` from room-authoring snapshots.
- Active-room-collision writers:
  - `Operations.cpp:createProductSessionFromPackage(...)`
    - Clears collision, then rebuilds from package active room plus
      `activeSession->state()`.
  - `Operations.cpp:createCreativeBlankSession(...)`
    - Clears collision for the blank Creative stage.
  - `Operations.cpp:clearProductGameplayLaunchState(...)`
    - Clears collision with active room.
  - `Operations.cpp:ProductCreativeBakedActiveRoomRefreshExecutor::handleRejectedBake(...)`
    - Rebuilds collision for the cleared/no-renderable active room using
      `activeSession_->state()`.
  - `Operations.cpp:ProductCreativeBakedActiveRoomRefreshExecutor::installBakedRoom(...)`
    - Builds collision from baked active room plus `activeSession_->state()`.
  - `Operations.cpp:launchProductNewWorld(...)`
    - Builds collision from ASCII active room plus session state.
  - `Operations.cpp:loadProductSessionSaveIntoRuntime(...)`
    - Builds collision if the loaded active room is loaded.
  - `ascii_room/Activation.cpp:activateProductAsciiRoomPreview(...)`
    - First builds collision without runtime state for preview, then rebuilds
      with `activeSession->state()` after session creation.
  - `gameplay/Controller.cpp:submitProductGameplayCommand(...)`
    - Rebuilds collision after accepted/ticked `Interact` commands, so runtime
      owned door/blocker surfaces track door state.
  - `gameplay/TapeRunner.cpp:refreshActiveRoomCollision(...)`
    - Rebuilds a supplied collision pointer from supplied active room/session
      during tape runs.
  - `automation/AutomationRoomEditing.cpp:copyRoomEditingStateToWindow(...)`
    - Copies `state.activeRoomCollision` to the window when room-editing state
      is ready.
  - Non-window room-editor state writer:
    `room_editor/EditingState.cpp:buildProductRoomEditingState(...)` builds
    `ProductRoomEditingState::activeRoomCollision` from the rebuilt active room.
- Sync-dependent readers:
  - `ReceiptBuilder.cpp` appends both `active_room_*` and
    `active_room_collision_*` fields; stale pairs produce misleading receipts.
  - `InputFrame.cpp:processProductWindowInputFrame(...)` passes
    `productActiveRoomCollisionSurfaces(window.activeRoomCollision)` into
    gameplay action application.
  - `gameplay/Controller.cpp` reads anchors and room geometry from
    `window.activeRoom.room`, and collision surfaces from
    `window.activeRoomCollision`.
  - `gameplay/ProjectionRefresh.cpp` builds scene projection from
    `window.activeRoom.room` and draw-list/open-door markers from
    `window.activeRoomCollision`.
  - `view/PrimitiveDrawList.cpp` draws active-room geometry and open-door
    markers from the room/collision pair.
  - `menu/ActionHandlers.cpp` starts room authoring from `window.activeRoom`.
  - `save/RoomMarkerBinding.cpp` binds authored room markers from active room
    into the runtime session during save load.
  - `gameplay/ScriptedDriver.cpp`, `gameplay/TapeRunner.cpp`, and
    `automation/AutomationGameplay.cpp` read collision surfaces for scripted
    action/application paths.
  - Tests in gameplay/window input construct both by hand, so a future service
    must preserve a fixture path or helper for unit setup.
- Stale-state cases:
  - Room replaced but collision not rebuilt:
    projection/receipt show the new room, while gameplay collision and
    open-door markers use surfaces from the old room. Pin with a test that
    changes room id/static meshes and asserts collision room id/query surfaces
    update in the same operation.
  - Collision rebuilt without runtime state after session creation:
    runtime-owned door blockers do not filter active/inactive door state. Pin
    with an ASCII/package room containing a runtime-owned door blocker, then
    assert `runtimeOwnedSurfaceCount`, `runtimeFilteredSurfaceCount`, and active
    blocker counts after session install and after interaction.
  - Clear path clears room but leaves old collision ready:
    no-renderable Creative clear or launch failure would render blank but still
    allow gameplay collision. Pin by installing a sentinel ready collision,
    clearing active room, and asserting `activeRoom.loaded=false`,
    `activeRoomCollision.ready=false`, and query count zero.
  - Room-editor copy path can drift if only `window.roomEditing` is updated:
    pause Edit Room or automation could show editable authored data while
    window active room/collision still point to the prior room. Pin with
    room-editor operation tests that compare state room id/collision query
    counts to the window copy after accepted operations.
- First future service boundary:
  - Add one narrow product helper, likely in an active-room-specific file, that
    returns or applies an atomic pair:
    `ProductActiveRoomInstallResult { ProductActiveRoomState activeRoom;
    ProductActiveRoomCollisionState collision; status/reason/source counts }`.
  - First implementation slice should not change policy. It should wrap the
    existing package-room and RoomBake install/clear paths in `Operations.cpp`
    only, because those already install both fields together and have focused
    tests.
  - Suggested service requests:
    - clear/unloaded active room,
    - package room + optional runtime session,
    - creative baked `RoomAsset` + required runtime session,
    - ASCII active room + runtime session,
    - saved authored room + runtime session,
    - room-editing state copy.
  - The helper should enforce: every successful install writes both fields, and
    every clear writes both fields. Collision should be built with runtime state
    whenever a live session is available.
- Tests/checks run:
  - `rg -n "activeRoom|activeRoomCollision|buildProductActiveRoom|buildProductActiveRoomCollision" src tests/unit`
  - targeted writer/reader `rg` scans for direct assignments,
    `productActiveRoomCollisionSurfaces(...)`, projection, room editing, and
    receipt readers.
  - `git -C /Users/kogaryu/iggy3d diff --check`
- Concerns/deferred:
  - `window.activeRoom` is not just a receipt mirror today; gameplay,
    projection, room editing, save marker binding, and automation all depend on
    it. Do not move ownership until a typed install pair exists.
  - Tests currently seed `window.activeRoom` and `window.activeRoomCollision`
    directly for gameplay/window input fixtures. A future service extraction
    should add a unit fixture helper before deleting or hiding the public fields.
