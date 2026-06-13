# Next Slice Backlog

Purpose: ordered candidate slices with reviewer-needed flags. This prevents broad jumps and makes pauses intentional.

Status labels:
- `ready`: can be planned directly if current dependencies landed.
- `review`: needs Reviewer Dex boundary ruling before build order.
- `defer`: known future work, not next.

## Near-Term

1. Runtime render-frame step
   - status: `review`
   - owner candidate: `runtime`
   - purpose: caller-driven presentation step from session + camera state to `LevelRenderFrame2DResult`.
   - caution: do not make render frame generation automatic inside gameplay ticks.

2. Camera/presentation state packet
   - status: `review`
   - owner candidate: `runtime` or `scene/camera`
   - purpose: decide whether camera state is session-owned or sibling presentation state.
   - caution: avoid mixing camera update with render backend.

3. Save slot retention/overwrite policy
   - status: `review`
   - owner candidate: `runtime`
   - purpose: define optional policy above explicit slot store/list/delete operations, such as overwrite prompts or retention limits.
   - caution: keep platform storage, cloud sync, compression, encryption, and user-facing UI out unless explicitly scoped.

## Ready When Requested

4. Collision cache incremental optimization
   - status: `defer`
   - owner candidate: `scene/level`
   - purpose: replace only changed collision tile objects.
   - caution: only do this if full rebuild becomes a measured problem.

5. Runtime command queue
   - status: `defer`
   - owner candidate: `runtime`
   - purpose: buffer command frames before command ticks.
   - caution: raw input mapping and queue ownership need separate design.

6. Interaction command execution
   - status: `review`
   - owner candidate: `scene/player` for interpretation, runtime for orchestration
   - purpose: execute `Interact` plans.
   - caution: requires target ownership/range semantics first.

7. Raw input binding
   - status: `defer`
   - owner candidate: platform/input layer not defined yet
   - purpose: map keyboard/gamepad to `GameplayCommand2D`.
   - caution: do not put raw devices in runtime session state.

## Checkpoint Before Broad Work

Pause before any slice that:

- removes legacy runtime render-cache fields
- introduces platform save storage, cloud sync, compression, encryption, or save UI policy
- makes runtime define tile mutation semantics
- makes runtime rebuild caches implicitly during ticks
- adds dynamic collision bodies
- introduces an entity registry/ECS
