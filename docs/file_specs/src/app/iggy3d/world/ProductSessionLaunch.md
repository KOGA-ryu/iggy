# File Spec

Files: `src/app/iggy3d/world/ProductSessionLaunch.hpp`, `src/app/iggy3d/world/ProductSessionLaunch.cpp`

Verified at: `eebd1820`

## Owns

- Product runtime session creation from package load results.
- Product session creation from app options and package path lookup.
- Continue and load-save selected-slot launch into gameplay.
- Save-load result recording and saved authored-room marker binding during load.

## Does Not Own

- Durable save scanning or save file mutation.
- New-world initial save planning.
- CreativeDocument launch/open behavior.
- Runtime session tick behavior after launch.

## Reads

- Package load result, app options, product world template, active/deleted save bridge result, frontend state, optional active session, and window state.
- Selected save slot facts and compatibility status from save browser projections.

## Writes / Mutates

- Mutates startup/package/runtime-session proof fields in `window.frontendShell`.
- Replaces `activeSession` after successful session creation.
- Resets and rebuilds active room and active room collision state.
- Records save load result, active product save id, selected save id, and saved marker binding proof.
- Sets interaction mode to player and enters product gameplay on successful continue/load.

## Calls Out To / Wires Out To

- `loadPackage(...)`, `buildProductPackageSessionSeed(...)`, and `Session::create(...)`.
- `selectProductContinueSave(...)`, `initializeSelectedProductSaveSlot(...)`, and save-slot lookup helpers.
- `loadProductSessionSave(...)`, `buildProductActiveRoomFromSavedAuthoredRoom(...)`, and `bindSavedRoomMarkersToSession(...)`.
- `ensureActiveRoomCollisionFresh(...)` and `enterProductGameplayTransition(...)`.

## Called By / Entry Points

- `src/app/iggy3d/menu/ActionHandlers.cpp` for Continue and Load selection.
- `src/app/iggy3d/world/ProductNewWorldLaunch.cpp` for new-world package/session creation.
- Focused proof: `rg -n "createProductSessionFromPackage|createProductSession\\(|launchProductContinueSave|launchProductLoadSaveSelection" src/app tests/unit`.

## Invariants

- Package load failure records startup failure facts and does not create a session.
- Failed save load clears product gameplay launch state.
- Authored-room loads must bind saved room markers before entering gameplay.
- Continue uses catalog newest-compatible policy, not ad-hoc save ordering.
- Successful launch sets player interaction mode and enters gameplay exactly once.

## Tests / Proof Commands

- `rg -n "product_starter_menu_action_tests|product_save_catalog_tests|product_creative_world_launch_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "launchProductContinueSave|launchProductLoadSaveSelection|activeProductSaveId|savedMarkerBind" tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/save/SaveBridge.*` unless load result contracts change.
- `src/app/iggy3d/save/SaveSlotOperations.*` unless selected-slot policy changes.
- `src/runtime/session/*` unless session creation contracts change.

## Update When

- Continue/load routing, package session creation, save-load proof recording, authored-room binding, or gameplay-entry side effects change.

## Do Not Update When

- Only save browser labels, package asset contents, or runtime tick behavior changes without changing product session launch contracts.
