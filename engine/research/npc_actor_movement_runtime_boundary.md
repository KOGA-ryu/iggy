# NPC Actor Movement Runtime Boundary

Purpose: close out the current NPC actor movement planner/runtime lane and document the boundary before any request-generation or cache-consumer follow-up work.

## Completed Chain

Current NPC actor movement is split into scene preparation and runtime application:

```text
NpcActorState2DRegistry
  + NpcActorControlState2DRegistry
  + LevelTileMap
  -> NpcActorMovementFramePlan2D
  -> NpcActorMovementFrameApply2DRequest
  -> RuntimeGameplayFrameStep / RuntimePolicyGameplayFrameStep
  -> RuntimeGameplayState.npcActors
```

`NpcActorMovementFramePlan2D` composes the scene-owned movement pipeline:

```text
actor/control registries
  -> NpcActorFrameState2D
  -> NpcActorMovementFrameIntent2D
  -> route target or escape route target
  -> NpcActorNavigationRequest2D
  -> NpcActorPathReport2D
  -> NpcActorPathStep2D
  -> NpcActorPathStepOccupancyFilter2D
  -> prepared frame apply requests
```

The raw and policy runtime frame/runner paths consume those prepared requests unchanged. Runtime applies the prepared requests into the carried gameplay state through `RuntimeNpcActorMovementFrameStep`, which delegates registry mutation to `NpcActorMovementFrameApply2D`.

`RuntimeNpcActorMovementRequestPlanStep` is the runtime-adjacent bridge for
callers that want prepared requests from current `RuntimeGameplayState` plus a
`LevelTileMap`. It passes the scene planner config through directly, including
opt-in occupancy policy and reservation settings, and mirrors the resulting
request/reservation counts for runtime-facing diagnostics.

The optional scene-only reservation lane can sit between prepared requests and
frame apply:

```text
prepared frame apply requests
  + NpcActorOccupancy2D
  + NpcActorOccupancyPolicy2D
  -> NpcActorMovementReservation2D
  -> NpcActorMovementReservedFrameApply2D
  -> NpcActorMovementFrameApply2D
```

This reservation layer is derived decision data. It is not actor truth, runtime
state, save state, or occupancy ownership.

`NpcActorMovementFramePlan2D` can opt into this scene-only policy path through
planner config. By default, it still uses the original hard-block occupancy
filter and emits the same prepared request shape. When enabled, the planner can
use `NpcActorPathStepOccupancyPolicyFilter2D` for policy-aware filtering and can
run `NpcActorMovementReservation2D` before publishing final `plan.requests`.
Planner outputs remain prepared apply requests; the planner still does not
apply movement or mutate actor registries.

## Ownership

- `RuntimeGameplayState` owns and carries `npcActors` and `npcControls` by value.
- `RuntimeSessionState` does not own NPC actor or control registries.
- `scene/npc` owns movement planning, route/path/step/filter semantics, single-actor execution, frame apply, and frame report semantics.
- `scene/npc` owns occupancy policy and per-frame movement reservation semantics as explicit wrappers around prepared movement requests.
- Runtime owns application of prepared movement requests into gameplay state.
- Runtime does not generate NPC movement decisions, route targets, paths, occupancy filters, or planner requests in the current boundary.

## Intentionally Not Included

This lane deliberately does not include:

- runtime request generation
- AI decision orchestration
- occupancy rebuild or cache refresh consumers
- save/session snapshot ownership for NPC actor/control registries
- command queue integration
- spirit/swarm/faction/profile gameplay semantics beyond configurable occupancy capacity
- rendering, bgfx, or Vulkan integration

The current post-move refresh flags remain report facts only. They do not call cache, render, visibility, interaction, save, or runtime consumers.

## Test Proof Points

The boundary is covered by these test families:

- scene-level movement pipeline acceptance through intent, route/escape route, navigation, path, step, occupancy filter, executor, frame apply, and frame report
- runtime prepared NPC movement acceptance for raw and policy frame/runner paths
- planner-to-runtime acceptance proving `NpcActorMovementFramePlan2D` requests feed raw/policy runtime prepared movement paths unchanged
- raw/policy frame and runner parity for prepared movement request application and aggregation
- ownership invariants showing `RuntimeSessionState` still has no `npcActors` or `npcControls`

## Next Possible Lanes

1. Add a thin runtime-adjacent helper that calls the scene planner outside gameplay frame steps, if the caller boundary needs a convenience wrapper.
2. Add cache/dirty consumers for post-move refresh flags.
3. Review save/load ownership for NPC actor and control registries.
4. Let Edi/ASCII dungeon replay feed map plus actor/control registries into this planner boundary.
5. Build a render packet lane before any backend-specific rendering work.

## Rendering Timing

Rendering remains later than movement ownership:

```text
ASCII / Edi replay
  -> render packet lane
  -> bgfx backend
  -> Vulkan much later
```

ASCII and Edi remain import/projection or replay inputs, not authoritative runtime movement state. A render packet lane should be established before bgfx. Vulkan should remain much later than the current scene/runtime movement boundary.
