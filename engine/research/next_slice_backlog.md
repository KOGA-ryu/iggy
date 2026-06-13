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

5. Runtime input command observability cleanup
   - status: `review`
   - owner candidate: `runtime`
   - purpose: refine reports or docs around input-intent gating, mapping, queueing, command running, and per-frame diagnostics if real callers need it.
   - caution: do not add raw device input mapping, move gate policy out of scene/player, or hide underlying runner diagnostics.

6. Interaction state lifetime policy
   - status: `review`
   - owner candidate: caller-owned sibling state, or runtime if session continuity is explicitly needed
   - purpose: decide whether `RuntimeInteractionState` remains an explicit caller-owned packet or becomes part of a broader runtime session/presentation state.
   - caution: target toggles can now update registries and player-input apply steps return updated interaction state, but `RuntimeSessionState` still does not own interaction targets/effects.

7. Broader interaction effect application
   - status: `review`
   - owner candidate: depends on effect domain
   - purpose: decide how recorded interaction events, inventory/combat/quest effects, or level/player mutations become authoritative outcomes.
   - caution: keep this out of runtime until the target subsystem and save semantics are explicit.

8. Runtime gameplay state and save lifetime policy
   - status: `review`
   - owner candidate: runtime if `RuntimeGameplayState` becomes the durable top-level packet; save lane only after explicit snapshot rules
   - purpose: decide whether `RuntimeGameplayState` is the long-term runtime packet for session + command queue + interaction + inventory, and which inventory/drop fields become saveable.
   - caution: gameplay frame steps now carry explicit interaction and inventory state, but `RuntimeSessionState` and existing save snapshots still do not own those fields.

9. Policy pickup consolidation
   - status: `review`
   - owner candidate: `scene/inventory` for policy semantics, `runtime` for orchestration
   - purpose: decide whether catalog-aware policy pickup should replace the simple pickup path in gameplay-frame orchestration, or whether both lanes intentionally coexist.
   - caution: do not hide item-definition lookup, stack-cap diagnostics, or inventory event ordering inside runtime.

10. UI action integration policy
   - status: `review`
   - owner candidate: UI for action planning, runtime/caller for executing actions
   - purpose: decide how `UiActionPlan` should be invoked by a host loop without making UI own runtime mutation.
   - caution: UI models/tool intents exist, but backend window/input bindings and runtime execution remain out of scope.

11. Raw input binding
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
