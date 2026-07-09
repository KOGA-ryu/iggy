# File Spec

Files: `src/app/iggy3d/debug/DevCollisionOverlayState.hpp`, `src/app/iggy3d/debug/NpcBehaviorDebugHudState.hpp`, `src/app/iggy3d/debug/TopDownMapState.hpp`

Verified at: `24df6b99`

## Owns

- App-side debug HUD state packets embedded by `DebugHudStore`.
- Dev collision overlay visible/status/reason proof fields.
- NPC behavior debug HUD visible/debug-available/line-count/status/reason/unresolved-profile proof fields.
- Top-down map overlay visible/purpose/size/status/reason/item-count proof fields.

## Does Not Own

- Overlay toggle command routing.
- NPC behavior debug line generation.
- Top-down map overlay construction.
- Drawing, Vulkan projection, or receipt formatting.
- Runtime collision, AI, or map truth.

## Reads

- These headers define packets only.
- Readers include projection refresh, action handlers, menu transitions, receipt appenders, and tests.

## Writes / Mutates

- No functions in these headers mutate state.
- `ActionHandlers.cpp` and `Transitions.cpp` mutate dev collision overlay state.
- `ProjectionRefresh.cpp` copies NPC behavior HUD and top-down map overlay facts into these packets.

## Calls Out To / Wires Out To

- Included by `DebugHudStore.hpp`.
- Receipt appenders emit `dev_collision_overlay_*`, `npc_behavior_debug_hud_*`, and top-down map fields.
- Window/frame presentation reads projected overlay facts separately from these state packets.

## Called By / Entry Points

- `ProductDevCollisionOverlayState`.
- `ProductNpcBehaviorDebugHudState`.
- `ProductTopDownMapState`.
- Focused proof: `rg -n "devCollisionOverlay|npcBehaviorDebugHud|topDownMap" src/app tests`.

## Invariants

- Defaults represent hidden or not-requested debug surfaces.
- These packets are app/window observability mirrors, not runtime collision, AI, or map owners.
- Status and reason strings are receipt-facing contract values.
- Field additions must identify the producer and receipt consumer.
- Keep debug packet ownership separate from draw-list and projection algorithms.

## Tests / Proof Commands

- `rg -n "product_window_input_frame_tests|product_npc_behavior_debug_hud_tests|product_top_down_map_overlay_tests|product_menu_transitions_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "dev_collision_overlay|npc_behavior_debug_hud|top_down_map" src/app/iggy3d/receipt tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/debug/DebugHudStore.hpp` unless embedding changes.
- `src/app/iggy3d/debug/NpcBehaviorDebugHud.*` unless NPC HUD projection changes.
- `src/app/iggy3d/debug/TopDownMapOverlay.*` unless map overlay facts change.
- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless producer copy paths change.
- `src/app/iggy3d/menu/ActionHandlers.*` unless dev overlay toggle behavior changes.

## Update When

- Debug state fields, defaults, producer ownership, receipt-facing status strings, or app-side mirror boundaries change.

## Do Not Update When

- Only runtime collision/AI/map algorithms, overlay drawing, or receipt field ordering changes without changing these packet contracts.
