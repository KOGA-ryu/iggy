# File Spec

Files:

- `src/runtime/ai/SegmentOcclusion.hpp`
- `src/runtime/ai/SegmentOcclusion.cpp`

Verified at: `f5f0a691`

## Owns

- Public AI-facing segment occlusion verdict enum: `SegmentOcclusionVerdict`.
- Public function contract and implementation for `segmentOcclusion(...)`.
- Validation and verdict mapping from segment/collider facts to `Clear`, `Blocked`, or `Unknown`.
- AI boundary over physics AABB segment-hit queries.

## Does Not Own

- Raw ray/AABB intersection math.
- Collider storage, baking, or surface extraction.
- Sensor filtering internals inside physics queries.
- NPC behavior policy, perception scoring, route selection, or session tick orchestration.

## Reads

- Caller-supplied `std::span<const PhysicsAabbCollider>`.
- Segment endpoints and occlusion margin.
- Collider validity through `isValidPhysicsAabbCollider(...)`.
- Physics collider and `Vec3` type contracts through `PhysicsAabbCollider.hpp`.

## Writes / Mutates

- No external state.
- Local validation and hit-result variables only.

## Calls Out To / Wires Out To

- `segmentHitsAnyPhysicsAabb(...)` in `src/runtime/physics/PhysicsCollisionQueries.cpp`.
- Runtime math helpers such as `isFinite(...)` and `lengthSquared(...)`.

## Called By / Entry Points

- `reasoningSegmentBlocked(...)` in `src/runtime/ai/ReasoningGraph.cpp`.
- Session LOS and sound occlusion helpers in `src/runtime/session/Session.cpp`.
- Grep proof: `rg -n '\bsegmentOcclusion\b|\bSegmentOcclusionVerdict\b|\breasoningSegmentBlocked\b' src tests`.

## Invariants

- Non-finite endpoints, degenerate segments, invalid margins, or invalid colliders return `Unknown`.
- Empty valid collider spans return `Clear`.
- Physics hit results map only to `Blocked` or `Clear`; invalid input is handled before the physics call.
- Failed or absent collision bakes are mapped by callers, not by this file.
- Verdict values preserve the fail-closed distinction between `Clear`, `Blocked`, and `Unknown`.
- Runtime AI code here must not gain app, window, render, projection, or save dependencies.

## Tests / Proof Commands

- `rg -n 'segment_occlusion_tests|reasoning_graph_tests' cmake tests`.
- `rg -n '\bsegmentHitsAnyPhysicsAabb\b|\bsegmentOcclusion\b' src tests`.

## Nearby Files Usually Not Touched

- `src/runtime/physics/PhysicsCollisionQueries.*` unless the segment query behavior changes.
- `src/runtime/session/Session.cpp` unless caller interpretation of `Unknown` changes.
- `src/runtime/ai/ReasoningGraph.*` unless reasoning route occlusion semantics change.
- `src/runtime/ai/NpcBehaviorSystem.*` unless behavior policy changes.

## Update When

- `SegmentOcclusionVerdict` values or meaning change.
- `segmentOcclusion(...)` parameters, validation rules, or return contract change.
- The physics query function used by occlusion changes.
- AI occlusion stops using physics AABB collider packets.

## Do Not Update When

- Only caller-side behavior policy changes and this wrapper contract stays unchanged.
- Physics ray internals change without changing `segmentHitsAnyPhysicsAabb(...)` behavior.
- Only tests are renamed without behavior or proof-command impact.
