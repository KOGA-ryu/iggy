# File Spec

Files: `src/app/iggy3d/world/Creation.hpp`, `src/app/iggy3d/world/Creation.cpp`

Verified at: `266066cb`

## Owns

- Product world creation input/request/result packets.
- Validation and normalization of new-world creation requests.
- Initial save plan construction for accepted worlds.
- Durable initial save execution through the product save bridge.

## Does Not Own

- Frontend world setup route decisions.
- Runtime session creation.
- ASCII room authoring or package construction.
- Save file-store internals or save id allocation.

## Reads

- `WorldSetupCreateRequest`, `ProductWorldTemplate`, save root, requested timestamp, generated world id, optional session state, optional authored room, and attempt token.

## Writes / Mutates

- Returns creation and initial-save result packets.
- Does not mutate frontend, window, session, or active-room state directly.
- Writes durable save files only through `writeProductSessionSaveDurably(...)`.

## Calls Out To / Wires Out To

- `writeProductSessionSaveDurably(...)` with world id, titles, timestamps, save type, session state, and optional authored room.

## Called By / Entry Points

- `src/app/iggy3d/world/ProductNewWorldLaunch.cpp` prepares new-world creation and writes the initial save.
- Tests call `makeProductWorldCreationInput(...)`, `prepareProductWorldCreation(...)`, and `writeProductWorldInitialSaveDurably(...)` directly.
- Focused proof: `rg -n "makeProductWorldCreationInput|prepareProductWorldCreation|writeProductWorldInitialSaveDurably" src/app tests/unit`.

## Invariants

- Creation request must be explicitly requested.
- Generated world id must be non-empty and reject path separators or traversal tokens.
- Package id, scenario id, and requested timestamp must be present.
- Initial save requires an accepted creation, requested save plan, and non-null session state.
- Successful initial save sets route after create to gameplay.

## Tests / Proof Commands

- `rg -n "product_world_creation_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "world_creation_missing_world_id|world_creation_initial_save_written|world_creation_session_missing" tests/unit src/app/iggy3d/world`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/world/ProductNewWorldLaunch.*` unless launch orchestration changes.
- `src/app/iggy3d/save/SaveBridge.*` unless durable write contracts change.
- `src/app/frontend/WorldSetupModel.*` unless create request fields change.

## Update When

- Creation packet fields, validation rules, initial save planning, or durable initial-save request behavior changes.

## Do Not Update When

- Only world setup UI labels, package contents, or lower-level save codec internals change without changing creation contracts.
