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

`RuntimeNpcActorMovementPlannedFrameStep` is the explicit convenience layer for
callers that want planning and prepared movement application in one call. It
composes `RuntimeNpcActorMovementRequestPlanStep` with
`RuntimeNpcActorMovementFrameStep` only when directly invoked. It does not make
raw or policy gameplay frame steps auto-plan movement.

`RuntimeNpcActorMovementPlannedFrameRunner` is the sequence companion for the
same explicit path. It repeats the planned-frame step over caller-supplied
map/config frames, carrying the returned `RuntimeGameplayState` forward between
frames and aggregating movement/request diagnostics. It also remains outside
raw and policy gameplay runners.

`RuntimeNpcAiMovementPlannedFrameStep` is the explicit one-frame AI+movement
composition helper. It runs caller-selected AI control planning/apply first,
then runs caller-selected movement planning/apply using the post-control state.
It is not invoked by raw or policy gameplay frame steps.

`RuntimeNpcAiProfileControlPlannedFrameStep` is the profile-backed variant for
the control half. It resolves actor `aiProfileId` values into map-play control
subjects, then delegates to `RuntimeNpcAiControlPlannedFrameStep`. This keeps
profile trait lookup explicit and outside raw/policy gameplay frame steps.

`RuntimeNpcAiProfileMovementPlannedFrameStep` is the profile-backed one-frame
AI+movement helper. It runs profile-backed control planning/apply first, then
uses the post-control gameplay state for actor movement planning/application.
It remains caller-selected and does not make raw/policy gameplay frames
auto-orchestrate AI or movement.

`RuntimeGameplayProfileScenarioDefinition` is the profile-driven authoring
packet for scenarios. It resolves actor `aiProfileId` values from the initial
actor registry into ordinary scenario frame subjects, then uses the existing
scenario definition/runner path. It is not a parser, UI, save/load hook, or
live per-frame profile resolver.

`RuntimeGameplayProfileScenarioRunner` is the convenience path over that
authoring packet. It validates the profile scenario, lowers it to a normal
scenario, runs the existing scenario runner, and projects the scenario ledger.
It is not a separate gameplay loop and does not add profile-specific frame
runner orchestration.

`RuntimeNpcAiMovementPlannedFrameRunner` is the explicit sequence companion for
that AI+movement path. It repeats the one-frame helper over caller-supplied
frames for simulations, tests, and tools while keeping raw/policy gameplay
loops free of automatic NPC AI orchestration.

`RuntimeNpcAiMovementRefreshFrameStep` and
`RuntimeNpcAiMovementRefreshFrameRunner` add refresh packet projection to that
explicit path. They compose planning, movement, and
`NpcActorMovementRefreshFrameProjector2D`, but they do not execute occupancy
cache consumers, interaction effects, AI decisions, visibility/FOV, render cache
updates, UI, or gameplay frame loops. The runner derives previous occupancy
from carried actor state at the start of each caller-supplied frame; that
occupancy remains derived local helper data, not runtime truth.

`RuntimeGameplayOrchestratedFrameStep` is a top-level explicit gameplay helper
for tools and tests that want one readable frame call. It runs the existing
player/input/interaction/pickup frame path first with prepared NPC movement
requests disabled, then runs `RuntimeNpcAiMovementRefreshFrameStep` with the
post-player gameplay state. This keeps NPC AI and movement orchestration
caller-selected and out of the raw/policy gameplay frame steps.

`RuntimeGameplayOrchestratedFrameRunner` repeats that explicit orchestrated
frame over caller-supplied frames, carrying `RuntimeGameplayState` forward and
aggregating player inventory events plus NPC control, movement, and refresh
packet facts. It is a separate top-level helper for tools, tests, and
simulations; it does not replace or wrap the old raw/policy gameplay frame
runners.

`RuntimeGameplayOrchestratedFrameReporter` and
`RuntimeGameplayOrchestratedFrameRunnerReporter` project compact report packets
from those explicit frame/runner results. They are engine data for future
debug, IDE, or UI tooling; they do not run gameplay frames, execute caches,
render, or mutate state.

`RuntimeGameplayScenarioRunner` wraps the explicit orchestrated frame runner
and runner reporter for tests, future replay tooling, and IDE/debug inspection.
It consumes an initial gameplay state plus scripted orchestrated frames and
returns the final gameplay state, nested runner result, and compact runner
report. External formats such as ASCII/Edi can emit this scenario packet later;
this boundary does not parse files, drive UI, or replace raw/policy runners.

`RuntimeGameplayScenarioDefinition` and
`RuntimeGameplayScenarioValidator` define the typed authoring packet that
external tools should target before building a `RuntimeGameplayScenario`.
Validation checks initial NPC actor/control joins and per-frame authoring facts
such as movement-map shape and trait subjects. It does not parse ASCII/Edi,
invoke UI/editor systems, or run gameplay.

## Scenario/Profile Boundary Closeout

The current profile-driven scenario path is intentionally frozen as this
engine-only sequence:

```text
RuntimeGameplayProfileScenarioDefinition
  -> RuntimeGameplayProfileScenarioValidator
  -> RuntimeGameplayProfileScenarioDefinitionBuilder
  -> RuntimeGameplayScenarioRunner
  -> RuntimeGameplayScenarioLedgerReporter
```

The convenience `RuntimeGameplayProfileScenarioRunner` performs exactly that
sequence in one explicit call. It is a tooling/test authoring helper over the
normal scenario path, not a new gameplay loop, parser, autosave path, or UI
integration point. Invalid profile scenarios stop at validation and do not run
gameplay.

Raw and policy gameplay frame paths remain prepared-request consumers. They do
not auto-run NPC AI control planning, movement planning, scenario lowering, or
profile trait resolution. External ASCII/Edi/IDE tooling should target typed
scenario definitions or profile scenario definitions first, then let engine
validation/build/runner/ledger packets handle the in-engine transition.

## External Authoring Boundary

The current external-authoring bridge is deliberately parser-free:

```text
external tool-owned data
  -> RuntimeGameplayScenarioAuthoringAdapter
  -> RuntimeGameplayProfileScenarioDefinition
```

`RuntimeGameplayScenarioAuthoringAdapter` accepts an explicit profile scenario
definition packet today. Unsupported sources remain unsupported, missing
profile scenario payloads are reported as adapter diagnostics, and the adapter
does not validate, build, run, or ledger the scenario. Those steps remain owned
by the profile scenario validator/builder/runner path.

`RuntimeGameplayAsciiScenarioPacket` is the typed in-memory ASCII authoring
packet that future ASCII/Edi/IDE tooling can target before conversion. It
preserves source id, grid rows, marker declarations, and optional frame
declarations by value. `RuntimeGameplayAsciiScenarioPacketValidator` validates
only the packet structure: empty row policy, rectangular grid shape, duplicate
markers, actor marker ids, known marker kinds, and unknown grid markers.

There is intentionally no ASCII-to-profile-scenario converter yet. The next
authoring lane should convert a validated ASCII packet into
`RuntimeGameplayProfileScenarioDefinition`, including explicit choices for map
construction, actor/control placement, profile catalog references, movement
maps, and frame scripting. Until that lane exists, ASCII packets are validated
authoring data only, not gameplay state, parser output from files, UI data, or
runtime orchestration input.

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
- `scene/npc` owns movement-derived refresh work projections and pure refresh consumers such as occupancy rebuild selection, AI map query refresh facts, and interaction refresh fact extraction.
- Runtime owns application of prepared movement requests into gameplay state.
- Runtime may expose explicit runtime-adjacent helpers that call scene planners when directly selected by the caller.
- Runtime may also expose explicit runtime-adjacent AI control helpers that call `scene/ai` planners when directly selected by the caller; raw/policy gameplay frames still do not auto-run AI control planning.
- Runtime gameplay frame steps and runners do not generate NPC movement decisions, route targets, paths, occupancy filters, or planner requests in the current boundary.

The optional helper layers are:

1. `scene/npc` planner creates prepared requests.
2. `RuntimeNpcActorMovementFrameStep` applies prepared requests.
3. `RuntimeNpcActorMovementPlannedFrameStep` composes 1 and 2 for one caller-selected frame.
4. `RuntimeNpcActorMovementPlannedFrameRunner` repeats 3 over caller-supplied frames.
5. `RuntimeNpcAiProfileControlPlannedFrameStep` resolves profile traits before caller-selected control planning when directly invoked.
6. `RuntimeNpcAiProfileMovementPlannedFrameStep` resolves profile traits before one caller-selected AI+movement planned frame.
7. `RuntimeNpcAiMovementPlannedFrameStep` composes caller-selected AI control planning with one movement planned frame when directly invoked.
8. `RuntimeNpcAiMovementPlannedFrameRunner` repeats 7 over caller-supplied AI+movement frames when directly invoked.
9. `RuntimeNpcAiMovementRefreshFrameStep` and runner variants add refresh packet projection to 7/8 without executing downstream caches.
10. `RuntimeGameplayOrchestratedFrameStep` composes the existing player frame path with 9 for one explicit caller-selected gameplay frame.
11. `RuntimeGameplayOrchestratedFrameRunner` repeats 10 over caller-supplied frames without replacing raw/policy gameplay runners.
12. Orchestrated frame/runner reporters project compact inspection packets over 10/11 without executing gameplay.
13. `RuntimeGameplayScenarioRunner` is a lightweight scenario/replay harness over 11 and 12, intended for tests and future tooling packets rather than parser/UI integration.
14. `RuntimeGameplayScenarioDefinition` and validator are typed authoring packets for tools to produce before conversion to the scenario runner input.
15. `RuntimeGameplayProfileScenarioDefinition` and validator are profile-driven authoring packets that convert actor profile traits into ordinary scenario subjects from the initial actor registry.
16. `RuntimeGameplayScenarioLedgerReporter` projects scenario execution results into compact ledger rows, final-state counts, and deterministic tooling events without running gameplay or adding UI/parser behavior.

## Intentionally Not Included

This lane deliberately does not include:

- runtime request generation
- AI decision orchestration
- runtime/cache execution of occupancy, interaction, AI map, render, or visibility refresh work
- save/session snapshot ownership for NPC actor/control registries
- command queue integration
- spirit/swarm/faction/profile gameplay semantics beyond configurable occupancy capacity
- rendering, bgfx, or Vulkan integration

The current post-move refresh flags can be projected into explicit scene-level
work packets and pure consumers. `NpcActorOccupancyRefresh2D` can rebuild
derived NPC occupancy by value, `NpcActorAiMapRefresh2D` can report affected
NPCs and optional pure AI map query diagnostics, and
`NpcActorInteractionRefresh2D` can report dirty interaction facts.
`NpcActorVisualRefresh2D` can split movement-derived visual work into separate
render and visibility packets. `NpcActorMovementRefreshFrame2D` composes those
refresh consumers into one inspectable movement refresh frame report. None of
these calls runtime, cache, render, visibility, interaction effect, save, UI, or
AI decision systems.

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
