# File Spec

Files: `src/app/iggy3d/receipt/DebugHudFields.cpp`

Verified at: `cd02b03e`

## Owns

- Receipt field emission for movement, NPC behavior, and physics debug HUD state.
- Debug HUD visibility, availability, status, reason, count, warning, and selected numeric proof fields.

## Does Not Own

- Debug HUD model construction.
- Runtime physics, movement, or NPC behavior diagnostics.
- Overlay drawing.
- Dev-tools or debug-overlay settings policy.

## Reads

- `MovementDebugHud`, `NpcBehaviorDebugHud`, `PhysicsDebugHud`, and `ProductAppWindowState.debugHud` fields needed for debug receipt proof.

## Writes / Mutates

- Appends fields to `RenderReceipt`.
- Does not mutate HUD models or window state.

## Calls Out To / Wires Out To

- `appendReceiptField(...)`.
- `floatReceiptValue(...)` for movement speed multiplier formatting.

## Called By / Entry Points

- `buildProductAppReceipt(...)` builds debug HUD models and calls `appendProductDebugHudFields(...)`.
- Focused proof: `rg -n "appendProductDebugHudFields|movement_debug_hud_visible|npc_behavior_debug_hud_visible|physics_debug_hud_visible" src/app tests`.

## Invariants

- HUD visibility and diagnostic availability remain separate fields.
- Debug HUD receipt emission must not build or refresh runtime diagnostics.
- Physics warning state is read from the HUD model, not recomputed here.
- NPC unresolved-profile proof reads the window debug state field.

## Tests / Proof Commands

- `rg -n "movement_debug_hud_visible|npc_behavior_debug_hud_visible|physics_debug_hud_visible" tests/unit tests/smoke src/app/iggy3d/receipt`.
- `rg -n "product_vulkan_room_frame_tests|product_gameplay_controls_smoke|product_gameplay_tape_smoke" cmake/iggy3d_tests.cmake tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/debug/*` unless HUD model fields change.
- `src/runtime/physics/*`, `src/runtime/movement/*`, and `src/runtime/ai/*` unless diagnostic source contracts change.
- `src/app/iggy3d/view/*` unless visual debug overlay rendering changes.

## Update When

- Debug HUD receipt keys, HUD model fields consumed here, or debug HUD receipt grouping changes.

## Do Not Update When

- Only runtime diagnostic generation or HUD drawing changes without changing receipt field emission.
