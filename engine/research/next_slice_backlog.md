# Next Slice Backlog

Purpose: ordered candidate slices with reviewer-needed flags. This prevents broad jumps and makes pauses intentional.

Status labels:
- `ready`: can be planned directly if current dependencies landed.
- `review`: needs Reviewer Dex boundary ruling before build order.
- `defer`: known future work, not next.

## Near-Term

1. `RuntimeLevelMutationStep`
   - status: `review`
   - owner candidate: `runtime`
   - purpose: take `RuntimeSessionState` plus explicit `LevelTileEdit` list, delegate to `LevelMutationCacheUpdateStep`, and return an updated session packet.
   - caution: runtime must not define mutation semantics, infer changed tiles, increment ticks, or rebuild caches directly. On success it should update `session.level`, `session.derivedCaches`, and legacy render-cache mirrors only.

2. Save snapshot boundary
   - status: `review`
   - owner candidate: `runtime`
   - purpose: define plain saveable session snapshot excluding derived caches.
   - caution: no disk IO, no versioned file format yet.

3. Runtime render-frame step
   - status: `review`
   - owner candidate: `runtime`
   - purpose: caller-driven presentation step from session + camera state to `LevelRenderFrame2DResult`.
   - caution: do not make render frame generation automatic inside gameplay ticks.

4. Camera/presentation state packet
   - status: `review`
   - owner candidate: `runtime` or `scene/camera`
   - purpose: decide whether camera state is session-owned or sibling presentation state.
   - caution: avoid mixing camera update with render backend.

## Ready When Requested

5. Collision cache incremental optimization
   - status: `defer`
   - owner candidate: `scene/level`
   - purpose: replace only changed collision tile objects.
   - caution: only do this if full rebuild becomes a measured problem.

6. Runtime command queue
   - status: `defer`
   - owner candidate: `runtime`
   - purpose: buffer command frames before command ticks.
   - caution: raw input mapping and queue ownership need separate design.

7. Interaction command execution
   - status: `review`
   - owner candidate: `scene/player` for interpretation, runtime for orchestration
   - purpose: execute `Interact` plans.
   - caution: requires target ownership/range semantics first.

8. Raw input binding
   - status: `defer`
   - owner candidate: platform/input layer not defined yet
   - purpose: map keyboard/gamepad to `GameplayCommand2D`.
   - caution: do not put raw devices in runtime session state.

## Checkpoint Before Broad Work

Pause before any slice that:

- removes legacy runtime render-cache fields
- introduces save/load
- makes runtime define tile mutation semantics
- makes runtime rebuild caches implicitly during ticks
- adds dynamic collision bodies
- introduces an entity registry/ECS
