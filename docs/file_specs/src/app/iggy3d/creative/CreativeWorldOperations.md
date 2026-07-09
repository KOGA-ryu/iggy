# File Spec

Files: `src/app/iggy3d/creative/CreativeWorldOperations.hpp`, `src/app/iggy3d/creative/CreativeWorldOperations.cpp`

Verified at: `d40a476b`

## Owns

- Product-level creative world launch, open, and current-world save orchestration.
- `ProductCreativeNewWorldLaunchResult`, `ProductCreativeOpenWorldLaunchResult`, and `ProductCreativeCurrentWorldSaveResult` receipt packets.
- Blank creative session entry, creative facade document install, interaction mode switch, frontend gameplay transition, active creative save identity recording, undo reset, and baked active-room refresh request.

## Does Not Own

- Creative world ID minting or durable save/load implementation.
- Creative document mutation internals.
- Starter/pause menu dispatch.
- Creative UI hit testing, command execution, or overlay presentation.

## Reads

- `ProductAppOptions`, launch/open requests, frontend state, active session, product window state, and `creative::CreativeAppState`.
- Creative world service results and facade document install receipts.

## Writes / Mutates

- Frontend gameplay state, active session, product window launch/status/input mode fields, creative undo state, and active creative identity.
- Creative document state through `Facade::installDocument(...)` and `documentForPersistence()` for save.

## Calls Out To / Wires Out To

- `createCreativeWorld(...)`, `openCreativeWorld(...)`, and `saveCreativeWorld(...)`.
- Creative blank session creation, product gameplay transition helpers, active creative identity recording, and baked active-room refresh.

## Called By / Entry Points

- Starter menu creative actions call launch/open through `ActionHandlers.cpp`.
- Save flow calls `saveProductCurrentCreativeWorld(...)` when CreativeDocument editor is active.
- Grep proof: `rg -n "launchProductCreative(New|Open)World|saveProductCurrentCreativeWorld|createCreativeWorld|openCreativeWorld|saveCreativeWorld|installDocument\\(" src/app/iggy3d tests cmake`.

## Invariants

- Product creative launch uses a blank creative stage, not product demo-room session creation.
- Successful launch/open installs a document before entering Creative interaction mode.
- Failed install clears product gameplay launch state.
- Active creative identity must be recorded after accepted launch/open/save.
- Saving current creative world requires Creative interaction mode, a save id, and a valid document id.

## Tests / Proof Commands

- `rg -n "product_creative_world_launch_tests|product_starter_menu_action_tests|product_creative_no_window_bake_scenario_tests" cmake tests`.
- `rg -n "saveProductCurrentCreativeWorld|recordActiveCreativeSaveIdentity" src tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/world/WorldService.*` unless durable creative world service rules change.
- `src/app/iggy3d/menu/ActionHandlers.*` unless starter/pause launch routing changes.
- `src/app/iggy3d/creative/Facade.*` unless document install semantics change.

## Update When

- Creative launch/open/save result packets, active identity recording, blank-session transition, facade install handling, interaction mode, or baked-room refresh behavior changes.

## Do Not Update When

- Only creative UI command or draw-list behavior changes after launch.
