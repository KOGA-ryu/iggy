# File Spec

Files: `src/app/iggy3d/world/ProductNewWorldLaunch.hpp`, `src/app/iggy3d/world/ProductNewWorldLaunch.cpp`

Verified at: `eebd1820`

## Owns

- Product New World launch from a `WorldSetupDraft`.
- Draft validation through world setup routing before launch.
- ASCII-room backed new-world session creation and fallback package new-world creation.
- Initial product save write request and window proof recording for created worlds.

## Does Not Own

- World setup UI model editing.
- ASCII parser/grid/room asset internals.
- Durable save bridge internals.
- Continue/load save selection behavior.
- CreativeDocument new-world launch behavior.

## Reads

- `ProductAppOptions`, `WorldSetupDraft`, frontend state, optional active session, and product window state.
- Built-in dungeon catalog facts for world setup proof.
- Product world template from options.

## Writes / Mutates

- Writes world setup and world creation proof fields under `window.creativeAuthoring`.
- May create or replace `activeSession`.
- Mutates active room and active room collision freshness for ASCII-backed worlds.
- Writes initial save proof into `window.saveSession`.
- Sets player interaction mode and enters product gameplay after successful initial save.

## Calls Out To / Wires Out To

- `routeWorldSetupAction(...)` and `prepareProductWorldCreation(...)`.
- `productWorldSetupAuthoringRequest(...)`, `buildProductAsciiRoomAuthoring(...)`, and `recordProductAsciiRoomPreview(...)`.
- `makeProductAsciiRoomPackage(...)`, `createProductSessionFromPackage(...)`, and `createProductSession(...)`.
- `writeProductWorldInitialSaveDurably(...)`, `clearProductGameplayLaunchState(...)`, and `enterProductGameplayTransition(...)`.

## Called By / Entry Points

- `src/app/iggy3d/AppKernel.cpp` app startup/new-world path.
- `src/app/iggy3d/menu/ActionHandlers.cpp` Create and Enter route.
- Tests call `launchProductNewWorld(...)` directly.
- Focused proof: `rg -n "launchProductNewWorld|writeProductWorldInitialSaveDurably|makeProductAsciiRoomPackage" src/app tests/unit`.

## Invariants

- Draft must be accepted by world setup routing before session creation.
- ASCII-room new worlds must author, preview-record, package, create session, set active room, and refresh collision before initial save.
- Initial save failure clears product gameplay launch state.
- Successful launch records active product save id and enters gameplay as player.
- World identity is minted through world creation/save seams, not handwritten in this file.

## Tests / Proof Commands

- `rg -n "product_world_creation_tests|product_starter_menu_action_tests|product_creative_world_launch_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "launchProductNewWorld|initialSaveWritten|opening_menu_new_world_failed" tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ascii_room/*` unless ASCII authoring/package contracts change.
- `src/app/iggy3d/world/Creation.*` unless creation or initial-save planning changes.
- `src/app/iggy3d/world/ProductSessionLaunch.*` unless package/session creation side effects change.

## Update When

- New-world launch ordering, draft validation, ASCII-room session creation, initial save proof, or gameplay-entry side effects change.

## Do Not Update When

- Only built-in ASCII room text, frontend labels, or lower-level save codec internals change without changing new-world launch contracts.
