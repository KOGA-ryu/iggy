# Next Slice Backlog

Purpose: ordered candidate slices with reviewer-needed flags. This prevents broad jumps and makes pauses intentional.

Status labels:
- `ready`: can be planned directly if current dependencies landed.
- `review`: needs Reviewer Dex boundary ruling before build order.
- `defer`: known future work, not next.

## Near-Term

Current active lane: NPC AI mechanics. Older runtime/UI/presentation work remains
valid, but it is not the active road while the project is focused on AI behavior.

1. NPC Play-to-control proposal
   - status: `ready`
   - owner candidate: `scene/ai` for interpretation, `scene/npc` for control-state vocabulary
   - purpose: convert a kept `NpcPlay` / `NpcFold` result into a proposed `NpcObjective`, `NpcBehaviorState`, and `NpcMoveMode` without mutating actor state.
   - caution: do not execute movement, enqueue commands, or update `NpcActorControlState2DRegistry` in this slice.

2. NPC trait-card pipeline acceptance
   - status: `ready`
   - owner candidate: `scene/ai`
   - purpose: acceptance coverage for trait pools -> draws -> hand -> read -> play -> tell -> fold using all six traits.
   - caution: keep this as proof of semantics and ordering, not runtime integration.

3. NPC profile/pool borrowing
   - status: `review`
   - owner candidate: `scene/ai`
   - purpose: define how an NPC profile selects or inherits from behavior pools and trait pools without C++ inheritance or runtime ownership.
   - caution: profile names, rank-like names, `Xed` vocabulary, or special ops vocabulary require explicit user approval before code.

4. NPC control-state apply step
   - status: `review`
   - owner candidate: `scene/npc`
   - purpose: consume an accepted play-to-control proposal and return an updated `NpcActorControlState2DRegistry`.
   - caution: this is control-state mutation only; do not move actors or touch runtime/session/save state.

5. NPC actor executor
   - status: `review`
   - owner candidate: `scene/npc` or `runtime` depending on collision/path ownership
   - purpose: execute actor control state into actor movement/position updates through existing navigation/physics primitives.
   - caution: current command execution is player-centric; avoid pretending NPC movement is solved by reusing player command execution blindly.

6. NPC AI phase runner
   - status: `review`
   - owner candidate: `runtime` for sequencing, `scene/ai` and `scene/npc` for semantics
   - purpose: one explicit phase pipeline: actor frame state -> map query -> trait draws -> hand/read/play/tell/fold -> proposal -> control apply -> actor executor.
   - caution: keep phase order explicit and preserve report/trace options; do not hide state ownership in `RuntimeSessionState`.

7. NPC tactical memory and threat profile
   - status: `review`
   - owner candidate: `scene/ai`
   - purpose: add memory/threat layers separate from static profile and live actor state, matching the repeated pattern in the AI reference tally.
   - caution: do not merge memory into trait pools or map substrate until the ownership boundary is clear.

8. Map reaction/action graph
   - status: `review`
   - owner candidate: `scene/ai` plus map/level owner once explicit
   - purpose: let map-owned intelligence provide affordances/reactions/actions that profiles consult.
   - caution: this is the strategic "map holds the greater AI" layer; keep it separate from actor personality and runtime command execution.

9. NPC compute benchmark harness
   - status: `ready`
   - owner candidate: `engine/tests` or a separate benchmark target
   - purpose: measure trait-card chain cost for tiny/large pools and one/many NPCs before optimizing copies.
   - caution: do not optimize away full trace reporting until measurements prove the cost matters.

10. Runtime render-frame step
   - status: `review`
   - owner candidate: `runtime`
   - purpose: caller-driven presentation step from session + camera state to `LevelRenderFrame2DResult`.
   - caution: do not make render frame generation automatic inside gameplay ticks.

11. Camera/presentation state packet
   - status: `review`
   - owner candidate: `runtime` or `scene/camera`
   - purpose: decide whether camera state is session-owned or sibling presentation state.
   - caution: avoid mixing camera update with render backend.

12. Save slot retention/overwrite policy
   - status: `review`
   - owner candidate: `runtime`
   - purpose: define optional policy above explicit slot store/list/delete operations, such as overwrite prompts or retention limits.
   - caution: keep platform storage, cloud sync, compression, encryption, and user-facing UI out unless explicitly scoped.

## Ready When Requested

1. Collision cache incremental optimization
   - status: `defer`
   - owner candidate: `scene/level`
   - purpose: replace only changed collision tile objects.
   - caution: only do this if full rebuild becomes a measured problem.

2. Runtime input command observability cleanup
   - status: `review`
   - owner candidate: `runtime`
   - purpose: refine reports or docs around input-intent gating, mapping, queueing, command running, and per-frame diagnostics if real callers need it.
   - caution: do not add raw device input mapping, move gate policy out of scene/player, or hide underlying runner diagnostics.

3. Interaction state lifetime policy
   - status: `review`
   - owner candidate: caller-owned sibling state, or runtime if session continuity is explicitly needed
   - purpose: decide whether `RuntimeInteractionState` remains an explicit caller-owned packet or becomes part of a broader runtime session/presentation state.
   - caution: target toggles can now update registries and player-input apply steps return updated interaction state, but `RuntimeSessionState` still does not own interaction targets/effects.

4. Broader interaction effect application
   - status: `review`
   - owner candidate: depends on effect domain
   - purpose: decide how recorded interaction events, inventory/combat/quest effects, or level/player mutations become authoritative outcomes.
   - caution: keep this out of runtime until the target subsystem and save semantics are explicit.

5. Runtime gameplay state and save lifetime policy
   - status: `review`
   - owner candidate: runtime if `RuntimeGameplayState` becomes the durable top-level packet; save lane only after explicit snapshot rules
   - purpose: decide whether `RuntimeGameplayState` is the long-term runtime packet for session + command queue + interaction + inventory, and which inventory/drop fields become saveable.
   - caution: gameplay frame steps now carry explicit interaction and inventory state, but `RuntimeSessionState` and existing save snapshots still do not own those fields.

6. Policy gameplay lane consolidation
   - status: `review`
   - owner candidate: `scene/inventory` for policy semantics, `runtime` for orchestration
   - purpose: decide whether `RuntimePolicyGameplayFrameStep` / Runner should replace the simple gameplay frame lane, or whether both lanes intentionally coexist.
   - caution: do not hide item-definition lookup, stack-cap diagnostics, or inventory event ordering inside runtime, and do not remove source compatibility without a migration slice.

7. UI action integration policy
   - status: `review`
   - owner candidate: UI for action planning, runtime/caller for executing actions
   - purpose: decide how `UiActionPlan` should be invoked by a host loop without making UI own runtime mutation.
   - caution: UI models/tool intents exist, but backend window/input bindings and runtime execution remain out of scope.

8. NPC actor/session ownership policy
   - status: `review`
   - owner candidate: `scene/npc` for actor state, `runtime` only after session/save boundaries are explicit
   - purpose: decide how `NpcActorState2DRegistry`, `NpcActorControlState2DRegistry`, and `NpcActorFrameState2D` relate to existing `modules/npc_ai::NpcAgentState`, runtime session state, and save snapshots.
   - caution: do not let NPC AI queue steps become the owner of NPC actor lifetime or persisted AI decision reports.

9. NPC AI runtime integration policy
   - status: `review`
   - owner candidate: `runtime` for queue order, `scene/ai` for decision semantics
   - purpose: decide whether NPC AI queue/composition should feed the existing gameplay frame lanes, or remain a separate command-frame intake path.
   - caution: preserve `scene/ai` as the source of scoring, routing, path, movement proposal, and command mapping rules.

10. Raw input binding
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
- removes the simple gameplay frame lane in favor of policy gameplay frames without a migration plan
- merges NPC actor/control/frame registries into runtime/session/save state without an ownership ruling
- introduces an entity registry/ECS
