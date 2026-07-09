# File Spec

Files: `src/app/iggy3d/save/Flow.hpp`, `src/app/iggy3d/save/Flow.cpp`

Verified at: `a200d76e`

## Owns

- Pause-menu save and save-and-exit orchestration.
- Product session save versus CreativeDocument save path selection.
- Pause-save result packet status, launch status, return-to-title, and session-reset facts.
- Save-and-exit transition to title after accepted durable save.

## Does Not Own

- Durable save file mutation internals.
- Creative document serialization or document mutation rules.
- Frontend action dispatch, draw-list layout, or input routing.
- Product session construction or load/continue behavior.

## Reads

- `ProductPauseSaveFlowKind`, `ProductAppOptions`, `FrontendState`, optional active `Session`, `ProductAppWindowState`, optional `FrontendSettings`, and optional `creative::CreativeAppState`.
- Creative editor activity through `productCreativeDocumentEditorActiveForSource(...)`.

## Writes / Mutates

- Updates `FrontendState.status`.
- Updates `ProductAppWindowState.frontendShell.launchStatus`.
- Resets the optional active session only for successful save-and-exit.
- Clears creative identity when a successful creative save-and-exit returns to title.
- Uses title-transition helpers that mutate frontend/window/settings state.

## Calls Out To / Wires Out To

- `writeProductCurrentSessionSave(...)` for normal product session saves.
- `saveProductCurrentCreativeWorld(...)` for active CreativeDocument saves.
- `returnProductToTitleTransition(...)` for successful save-and-exit transitions.
- `clearProductGameplayOnlyModes(...)` after settings-aware title return.

## Called By / Entry Points

- `src/app/iggy3d/menu/ActionHandlers.cpp` pause save and pause save-and-exit handlers.
- Focused proof: `rg -n "executeProductPauseSaveFlow|ProductPauseSaveFlowKind|ProductPauseSaveFlowResult" src/app/iggy3d/save src/app/iggy3d/menu tests/unit`.

## Invariants

- Save-and-exit returns to title only when the selected save path reports success.
- Creative save path requires an active CreativeDocument editor source.
- Missing creative app facade records `product_creative_save_facade_missing`.
- Product session save and CreativeDocument save status strings stay distinct.
- Normal save keeps the active session alive.

## Tests / Proof Commands

- `rg -n "pause_creative_save_written|product_creative_save_facade_missing|product_save_session_missing" tests/unit src/app/iggy3d/save`.
- `rg -n "product_creative_world_launch_tests|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/save/SaveBridge.*` unless durable write result contracts change.
- `src/app/iggy3d/creative/CreativeWorldOperations.*` unless creative current-world save behavior changes.
- `src/app/iggy3d/menu/ActionHandlers.*` unless user-facing pause action routing changes.

## Update When

- Pause save status strings, result fields, creative-vs-product path selection, title-return behavior, or session reset rules change.

## Do Not Update When

- Only save codec internals, UI labels, or lower-level file-store behavior change without changing pause-save flow contracts.
