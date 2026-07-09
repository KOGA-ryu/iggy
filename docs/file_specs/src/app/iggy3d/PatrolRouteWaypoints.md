# File Spec

Files: `src/app/iggy3d/PatrolRouteWaypoints.hpp`, `src/app/iggy3d/PatrolRouteWaypoints.cpp`

Verified at: `1ae0d0d9`

## Owns

- Extraction of authored creative `PatrolRoute` path points into flattened `Vec3` waypoints.
- Counts for patrol routes seen, routes contributing at least one waypoint, and skipped non-finite or out-of-float-range points.
- Product/app bridge shape expected by reasoning graph patrol waypoint input.

## Does Not Own

- Creative document mutation or path editing.
- Patrol route object descriptor policy.
- Runtime NPC patrol validation or movement.
- Reasoning graph construction.
- Save/load encoding of creative path objects.

## Reads

- `creative::CreativeDocument::objects()`.
- `creative::CreativeObjectKind::PatrolRoute`.
- `CreativeObject.pathPoints` and each path point position.

## Writes / Mutates

- Returns `PatrolRouteWaypoints`.
- Does not mutate the creative document, creative objects, or runtime AI state.

## Calls Out To / Wires Out To

- Converts finite creative double-precision positions into runtime `Vec3`.
- `CreativeReasoningActivation.cpp` passes extracted waypoints into `buildReasoningGraph(...)`.
- Unit tests call the extractor directly.

## Called By / Entry Points

- `patrolRouteWaypointsFromDocument(...)`.
- `activateCreativeReasoningGraph(...)`.
- Focused proof: `rg -n "PatrolRouteWaypoints|patrolRouteWaypointsFromDocument|PatrolRoute" src/app tests`.

## Invariants

- Only creative objects with kind `PatrolRoute` contribute.
- Waypoints are flattened in document object order and path-point order.
- A route with no usable waypoints still increments route count but not contributing route count.
- Non-finite and out-of-float-range points are skipped and counted.
- No transforms are applied; authored path points are treated as world-space positions.

## Tests / Proof Commands

- `rg -n "patrol_route_waypoints_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "patrolRouteWaypointsFromDocument|skippedNonFinitePointCount|contributingRouteCount" tests/unit/patrol_route_waypoints_tests.cpp src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/CreativeReasoningActivation.*` unless graph handoff changes.
- `src/app/iggy3d/creative/document/Object.*` unless creative object/path storage changes.
- `src/app/iggy3d/creative/mutation/*` unless patrol route mutation payload semantics change.
- `src/runtime/ai/ReasoningGraph.*` unless waypoint input contract changes.

## Update When

- Patrol route extraction, waypoint ordering, finite/range filtering, counters, or reasoning graph handoff semantics change.

## Do Not Update When

- Only UI editing, descriptor labels, save/load formatting, or runtime NPC patrol execution changes without changing waypoint extraction.
