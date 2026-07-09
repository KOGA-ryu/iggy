# File Spec

Files:

- `src/runtime/ai/ReasoningRoute.hpp`
- `src/runtime/ai/ReasoningRoute.cpp`

Verified at: `f5f0a691`

## Owns

- Actor-free travel cost over `ReasoningEdge`.
- `PlannedRoute` packet and `planRoute(...)` least-cost route search over a `ReasoningGraph`.
- Endpoint-to-nearest-reachable-node selection and deterministic Dijkstra traversal.

## Does Not Own

- Reasoning graph construction or node vocabulary.
- Low-level occlusion verdicts.
- Guard behavior policy after a route is planned.
- Session state, movement execution, or app presentation.

## Reads

- `ReasoningGraph` nodes/edges and caller-supplied physics AABB colliders.
- `TravelCostConfig` edge-kind multipliers and neutral slope term.
- World-space route endpoints.

## Writes / Mutates

- Local adjacency, distance, predecessor, settled, and route vectors.
- Returned `PlannedRoute` only.
- No persistent cache, session state, or graph mutation.

## Calls Out To / Wires Out To

- `reasoningSegmentBlocked(...)` to test endpoint reachability.
- `travelCost(...)` for edge weights during route search.

## Called By / Entry Points

- `GuardDecision.cpp` and `Session.cpp` route-selection paths.
- `reasoning_route_tests` and `stealth_garden_tests`.
- Grep proof: `rg -n "\bplanRoute\b|\btravelCost\b|\bPlannedRoute\b" src tests/unit`.

## Invariants

- Endpoint already at a graph node bypasses occlusion because no segment exists to query.
- Empty graph, unreachable endpoint, or disconnected exit returns an empty route.
- Equal-cost traversal remains deterministic by node id.
- The graph is treated as undirected for route planning.

## Tests / Proof Commands

- `rg -n "reasoning_route_tests|stealth_garden_tests" cmake tests/unit`.
- `rg -n "reasoningSegmentBlocked|buildReasoningGraph|ReasoningGraphConfig" src/runtime/ai tests/unit`.

## Nearby Files Usually Not Touched

- `src/runtime/ai/ReasoningGraph.*` unless graph shape or occlusion adapter changes.
- `src/runtime/ai/GuardDecision.*` unless route consumption policy changes.
- `src/runtime/session/Session.cpp` unless live route fallback policy changes.

## Update When

- Route selection, travel cost, endpoint reachability, or disconnected-route behavior changes.
- Planned route packet shape changes.

## Do Not Update When

- Graph construction changes while route search contract remains stable.
- Guard AI policy changes without changing `planRoute(...)`.
