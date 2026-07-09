# File Spec

Files: `src/app/iggy3d/CreativeReasoningActivation.hpp`, `src/app/iggy3d/CreativeReasoningActivation.cpp`

Verified at: `1ae0d0d9`

## Owns

- Product activation hook that installs a reasoning graph after a creative document is baked into an active room.
- Wiring from baked `RoomAsset` plus authored creative document patrol routes into `buildReasoningGraph(...)`.
- Session-level reasoning graph installation for creative baked-room activation.

## Does Not Own

- Creative document bake logic.
- Patrol route extraction details.
- Reasoning graph construction internals.
- Session tick, AI behavior, or reasoning route execution.
- Receipt field emission.

## Reads

- Baked `RoomAsset` from creative room bake.
- `creative::CreativeDocument` authored path objects through `patrolRouteWaypointsFromDocument(...)`.
- `Session` graph installation API.

## Writes / Mutates

- Mutates the provided `Session` by calling `setReasoningGraph(...)`.
- Does not mutate the creative document or baked room asset.

## Calls Out To / Wires Out To

- Calls `patrolRouteWaypointsFromDocument(...)`.
- Calls `buildReasoningGraph(...)`.
- Calls `Session::setReasoningGraph(...)`.
- Used as the default `ProductCreativeBakedRoomActivationHook` by creative baked active-room refresh.

## Called By / Entry Points

- `activateCreativeReasoningGraph(...)`.
- `creative/BakedActiveRoomRefresh.cpp` assigns this hook when the caller does not provide one.
- Focused proof: `rg -n "activateCreativeReasoningGraph|ProductCreativeBakedRoomActivationHook|refreshProductCreativeBakedActiveRoom" src/app tests`.

## Invariants

- The hook is product/app wiring, not a reusable runtime AI kernel.
- Graph construction remains deterministic because it delegates to pure reasoning graph and patrol waypoint extraction surfaces.
- Authored patrol route waypoints must be supplied to graph construction in document order.
- The creative document and room asset remain input truth; this file only bridges them to session AI state.

## Tests / Proof Commands

- `rg -n "activateCreativeReasoningGraph|bakedActiveRoomRefresh|buildReasoningGraph" tests/unit/product_creative_world_launch_tests.cpp src/app`.
- `rg -n "product_creative_world_launch_tests|patrol_route_waypoints_tests|reasoning_graph_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/PatrolRouteWaypoints.*` unless authored waypoint extraction changes.
- `src/runtime/ai/ReasoningGraph.*` unless graph construction contract changes.
- `src/runtime/session/Session.*` unless graph installation API changes.
- `src/app/iggy3d/creative/BakedActiveRoomRefresh.*` unless activation-hook selection changes.

## Update When

- Creative baked-room activation, reasoning graph installation, patrol route handoff, or default activation hook behavior changes.

## Do Not Update When

- Only reasoning graph internals, creative document storage, or runtime AI behavior changes without changing this product activation wiring.
