# File Spec

Files:

- `src/runtime/ai/ReasoningGraph.hpp`
- `src/runtime/ai/ReasoningGraph.cpp`

Verified at: `f5f0a691`

## Owns

- Reasoning graph node/edge vocabulary and summary packets.
- `ReasoningGraphConfig` and pure `buildReasoningGraph(...)` graph construction from `RoomAsset` anchors and patrol waypoints.
- `reasoningSegmentBlocked(...)` as the reasoning-layer adapter over `segmentOcclusion(...)`.
- Stable graph ordering and deterministic walkable edge emission.

## Does Not Own

- Session lifetime, command ticks, or per-tick AI command enqueueing.
- Route search over an existing graph.
- Low-level physics segment/ray math.
- App/menu/notebook display of graph facts.

## Reads

- `RoomAsset` anchors and caller-supplied patrol waypoints.
- Baked physics AABB colliders from room spatial surfaces.
- `SegmentOcclusionVerdict` through `segmentOcclusion(...)`.

## Writes / Mutates

- Local `ReasoningGraph` nodes, edges, ids, source labels, and summary counts.
- No session state, save state, global state, or persistent cache.

## Calls Out To / Wires Out To

- `buildSpatialSurfaceSet(room)` for room collision surfaces.
- `bakePhysicsAabbCollidersFromSpatialSurfaces(...)` for graph-build occlusion colliders.
- `segmentOcclusion(...)` through `reasoningSegmentBlocked(...)`.

## Called By / Entry Points

- `buildReasoningGraph(...)`, `summarizeReasoningGraph(...)`, `reasoningSegmentBlocked(...)`, `reasoningNodeKindName(...)`, `reasoningEdgeKindName(...)`.
- Session, guard decision, creative activation, readouts, and tests call this graph surface.
- Grep proof: `rg -n "reasoningSegmentBlocked|buildReasoningGraph|ReasoningGraphConfig" src/runtime/ai tests/unit`.

## Invariants

- `Clear` occlusion allows an edge; `Blocked` and `Unknown` block the edge.
- Successful empty bake means open graph; failed bake returns graph nodes but no fabricated edges.
- Node ids remain stable-sorted and deterministic for identical inputs.
- Graph build is not a per-tick navmesh and must not read `Session`.

## Tests / Proof Commands

- `rg -n "reasoning_graph_tests|stealth_garden_tests" cmake tests/unit`.
- `rg -n "SegmentOcclusionVerdict|segmentOcclusion" src/runtime/ai src/runtime/session tests/unit`.

## Nearby Files Usually Not Touched

- `src/runtime/ai/ReasoningRoute.*` unless route search semantics change.
- `src/runtime/ai/SegmentOcclusion.*` unless occlusion verdict semantics change.
- `src/runtime/session/Session.*` unless graph ownership or live AI wiring changes.

## Update When

- Node/edge vocabulary, graph build inputs, occlusion mapping, or deterministic ordering changes.
- Failed/empty bake behavior changes.

## Do Not Update When

- Route search changes without graph construction changes.
- App presentation reads graph facts without changing runtime graph contracts.
