# Shippable Tactical Demo Execution Spec

Updated: 2026-06-20

This is the builder-facing task plan for the first shippable tactical demo.
It expands `shippable_tactical_demo_roadmap.md` with runtime semantics, data
ownership, compute and storage budgets, concrete packet boundaries, and
verification gates.

The 3D runtime implementation lane is now the standalone `iggy3d` side repo.
Builders should use `engine/research/iggy3d_side_repo_plan.md` for exact first
paths, no-legacy-build boundaries, and the rule that current 2D runtime files
are reference material only, not adapter inputs or authority.

The product target is a native 3D, bespoke, single-player tactical demo. The
tools are internal. The first shipped mode is single-player local, but runtime
state, command, save, and package contracts must preserve a future path to
local and online multiplayer.

## Current Source Facts

- The broader product platform is native 3D; 2D/tile systems may remain as
  simulation, collision, authoring, fixture, and debug scaffolding, but they are
  not the shipping renderer.
- `engine/content/demos/product_loop_demo` is the current product-facing content
  proof. It loads as a package, picks up a key, saves/loads gameplay state, and
  opens a required-item door.
- `RuntimeGameplayProductLoopState` currently owns package/source identity,
  lowered scenario data, initial/current `RuntimeGameplayState`, and
  `nextFrameIndex`.
- Native play currently saves only `product_.play.state.loop.currentState` and
  loads only that state back into the active product loop. Package identity,
  loop cursor, clock state, command history, and compatibility metadata are not
  durable yet.
- Pause currently means input focus is disabled. It is not a simulation time
  mode and it blocks issuing tactical commands while paused.
- Target discovery is nearest enabled spatial target by distance, with
  registry-order tie behavior. There is no action mask, faction rule, line of
  sight rule, combat range rule, or explicit target priority yet.
- The native 3D draw-list path emits floor tiles, walls, present NPC actors, and
  the player. Renderer slots currently cover floor, wall, NPC actor, and player.
- Current native static assets are tiny `.igmesh` fixtures. The renderer loads
  default model filenames from an app-local policy, not from the authored
  scenario package.

## Product Semantics

### Product Session

Add an explicit product session state above the current product loop state.

Semantic shape:

```text
RuntimeProductSessionState
  package identity
  scenario identity
  content compatibility metadata
  lifecycle state
  initial/retry state
  current RuntimeGameplayState
  product loop cursor
  tactical simulation clock
  accepted/rejected command log
  random seed/state when randomness exists
  combat/objective state
```

Rules:

- Product session owns lifecycle: loading, playing, paused, completed, failed,
  retrying, saving, loading, incompatible-save.
- Product loop owns deterministic gameplay stepping.
- Native shell owns OS input, window, CLI options, and user-facing status text.
- Input frames remain transient and are never saved directly.
- Presentation observes session state and draw-list output. It does not mutate
  gameplay truth.

### Tactical Time

Pause must split into two separate concepts:

- `InputFocus`: whether OS/app input should be accepted.
- `SimulationClockState`: whether simulation is normal, slow, paused, or
  consuming a requested step.

Required modes:

- `Normal`: simulation consumes fixed ticks at full rate.
- `Slow`: simulation consumes fixed ticks at a scaled rate.
- `Paused`: simulation consumes no automatic ticks, but accepts player command
  planning.
- `StepRequested`: simulation consumes exactly one fixed tick or one command
  resolution quantum, then returns to paused.

Rules:

- Tactical command issuing must work while simulation is paused.
- Save/load must preserve clock mode, accumulated time if used, and pending
  step request status.
- AI, cooldowns, windups, recovery, status effects, and objectives advance from
  simulation ticks, not from render frames.

### Commands And Multiplayer Path

The single-player demo should behave like a local authoritative session.

Player input flow:

```text
OS/input event
  -> transient product input frame
  -> binding action
  -> serializable player/session command
  -> command admission
  -> simulation application
  -> command log entry
  -> presentation projection
```

Command ownership:

- Product input adapter maps transient controls to binding actions.
- Binding maps actions to proposed commands.
- Command admission owns validation, reach/range checks, faction checks, and
  rejection diagnostics.
- Simulation owns accepted command effects.
- Command log owns accepted and rejected command records for replay, debugging,
  durable saves, and future multiplayer.

Minimum command fields:

```text
command id
source player/client id
actor/entity id
target entity id or target point/tile
command type
simulation tick/frame
sequence number
payload
admission result
pre-state hash placeholder
post-state hash placeholder
```

First command types:

- move;
- interact;
- inspect;
- cancel;
- wait;
- select target;
- attack;
- ability;
- time mode change;
- retry/reset request;
- save/load request as session commands, not gameplay mutations.

### Target Discovery And Action Admission

Keep `InteractionTargetSpatialQuery2D` as a primitive, but do not let nearest
target alone define product behavior.

Add a product-level target discovery policy:

```text
TargetDiscoveryPolicy(action, actor, query, state)
  -> candidates
  -> chosen target
  -> rejection diagnostics
```

Rules:

- Candidate collection can be O(target count) for the first demo.
- Candidate ordering must be deterministic.
- Priority order must be explicit. Recommended first order:
  enemy attack target, active objective, required interaction, pickup,
  inspection-only marker.
- Ties use distance, then stable target id.
- Disabled targets are reported, not silently chosen.
- Out-of-range, missing required item, wrong faction, blocked line, not
  attackable, no effect, and no target are distinct diagnostics.
- Reach/range checks should be shared across pickup, interact, attack, and
  ability execution.

### Combat Slice

The first combat slice should be deliberately small.

Minimum state:

```text
CombatEntityState
  entity id
  faction id
  health
  alive/dead
  selected/current order
  cooldowns
  statuses
```

Minimum effects:

- direct attack;
- damage;
- defeat;
- cooldown;
- win/loss objective mutation.

Rules:

- Combat entity ids must align with runtime actor/entity ids.
- Faction checks happen before damage.
- Defeated entities stop receiving orders and stop being attackable except for
  explicit inspection/debug.
- Randomness should be avoided in the first combat slice. If randomness is
  introduced, save/load and command replay must persist seed/state first.

### Durable Save/Load

The current gameplay snapshot remains useful, but product saves need an outer
product-session envelope.

Required product save fields:

```text
save format id/version
package id or manifest identity
package path/content hash
scenario id
scenario source/content hash
static asset package identity/hash when present
product loop nextFrameIndex
runtime gameplay snapshot
tactical clock state
command log cursor and command records
random seed/state when present
combat/objective state
created/updated metadata
```

Rules:

- Loading must reject or warn on package/content mismatch before mutating the
  active session.
- Native play may choose the slot, path, and UX text, but runtime owns save
  compatibility semantics.
- Save files store content identity and state, not mesh bytes.
- Save summary/listing should not require full gameplay restore once save counts
  grow beyond the demo.

### Content And Asset Package

The authored scenario package should own content identity and references. The
asset catalog should own logical asset records. The native asset resolver should
load CPU mesh assets. The renderer should own GPU resources only.

Ownership chain:

```text
package.toml
  -> scenario.toml
  -> optional assets.toml
  -> AssetCatalog records
  -> native asset resolver
  -> CPU NativeStaticMeshAsset or fallback diagnostic
  -> renderer model-slot upload
  -> NativeSceneDrawList item references a semantic model id
```

Rules:

- Package reader validates manifest fields and referenced files.
- Asset catalog stores ids, kind ids, source paths, and metadata. It does not
  parse `.igmesh` and does not know Vulkan.
- Native resolver parses `.igmesh` through the existing loader and reports
  loaded, missing, invalid, fallback, and unused facts.
- Renderer receives resolved mesh data or slot assignments. It must not parse
  authored packages.
- Draw-list construction remains backend-free and GPU-free.

## Data Ownership Matrix

| Data | Owner | Persistent | Not Owner |
| --- | --- | --- | --- |
| Package manifest identity | runtime package reader | yes | renderer, native shell |
| Scenario source and lowered scenario | runtime loader/product loop | yes by identity/hash | renderer |
| Current gameplay state | runtime product session/loop | yes | native shell |
| Input events | native shell/input accumulator | no | save system |
| Player commands | runtime command boundary | yes in command log | renderer |
| Command admission diagnostics | runtime command boundary | yes for log/debug | renderer |
| Tactical clock | runtime product session | yes | input focus |
| Save slot path/name | native shell/platform policy | yes as metadata | gameplay simulation |
| Save compatibility | runtime save/product session | yes | native shell |
| Asset logical ids | package/AssetCatalog | yes | Vulkan renderer |
| CPU mesh data | native asset resolver/loader | no or cache only | gameplay |
| GPU buffers/pipelines | renderer | no | runtime/gameplay |
| Draw items | projection layer | transient | save system |
| HUD/debug text | native shell/presentation | no | simulation |

## Compute And Storage Budgets

These budgets are first-demo gates, not final engine limits.

### Product Loop

- Product stepping cost target: O(active actors + queued commands + interaction
  targets + local map facts touched by commands).
- Avoid full-map recomputation in every tactical tick unless the map is still
  fixture scale.
- First demo target: one player unit, one or two enemy units, one objective,
  one pickup/door/hazard class.

### Target Discovery

- Current O(target count) scan is acceptable for the first demo.
- Demo budget: under 128 targetable entities.
- Stress budget: add one test near 512 targets before adding broad props or
  overlapping combat markers.
- Candidate sorting must be stable and deterministic.

### Save/Load

- Current snapshot save/load cost is O(save bytes) and duplicates payload bytes
  through snapshot chunks, archive, envelope, CRC32, and file IO.
- Demo save target: under 256 KiB.
- Demo save/load target: under one frame on desktop for normal slots.
- Slot summaries may load full saves for the demo, but release-style autosaves
  need metadata summaries before large slot counts.

### Native Draw List And Renderer

- Current product-loop demo native draw list is about 47 draw items:
  28 floor tiles, 18 wall tiles, and 1 player.
- First tactical demo target: under 250 draw items.
- Stress target: one deterministic scene near 500 draw items to expose the
  cost of one indexed draw per item.
- Current renderer path binds and draws per item. It is fine for the first
  slice; batching/instancing should wait until render summaries prove the need.
- Current mesh assets are tiny: floor 4 vertices/6 indices, wall 8/36,
  player 6/24, NPC 7/30. Keep first new assets in this scale class.

### Package And Asset Load

- Package load is O(manifest bytes + scenario bytes + asset manifest bytes).
- Static mesh load is O(total mesh bytes + vertices + indices).
- Saves store content identity, not mesh bytes.
- Renderer upload should happen on load/asset change, not every frame.

## Builder Packets

Each packet should be independently reviewable. Avoid mixing docs, runtime
state, renderer, and package grammar changes unless the packet explicitly owns
that boundary.

### Packet 1: Product Save Envelope

Owner: runtime.

Write scope:

- `engine/src/runtime/RuntimeGameplayProduct*Save*` new files or equivalent;
- `RuntimeGameplayProductLoop` snapshot helpers;
- focused save/load tests;
- native session call-site only if required to prove integration.

Semantics:

- Capture package identity, scenario identity, `nextFrameIndex`,
  `RuntimeGameplaySnapshot`, and placeholder clock/command-log metadata.
- Restore full product loop state, not only `currentState`.
- Detect save-versus-loaded-package mismatch.

Data ownership:

- Runtime owns compatibility and restore semantics.
- Native shell owns save directory and slot selection only.

Verification:

- Save after pickup, reset, load, then continue with the same `nextFrameIndex`.
- Mismatched package load reports an explicit compatibility status.
- Existing gameplay save slot tests still pass.

Hard stops:

- Do not change `.igmesh` loader or renderer.
- Do not add combat in this packet.

### Packet 2: Tactical Clock State

Owner: runtime product session.

Write scope:

- new clock type and tests;
- product session/play-mode state if needed;
- native command mapping only for pause/slow/step smoke.

Semantics:

- Add `Normal`, `Slow`, `Paused`, and `StepRequested`.
- Keep input focus separate from simulation pause.
- While paused, allow command planning/input admission.
- Step consumes one deterministic simulation quantum.

Data ownership:

- Runtime owns clock state and tick policy.
- Native shell owns key bindings and display text.

Verification:

- Paused session accepts a command without advancing simulation until step or
  resume.
- Step advances exactly once.
- Save/load preserves clock mode.

Hard stops:

- Do not build enemy combat yet.
- Do not use render-frame timing as simulation truth.

### Packet 3: Serializable Command Boundary

Owner: runtime command layer.

Write scope:

- command structs/codecs/log tests;
- input binding mapper extensions;
- no renderer files.

Semantics:

- Extend from move/interact/wait into inspect, cancel, select target, attack,
  ability, and time/session command records.
- Add source id, actor id, sequence, simulation tick/frame, target payload,
  admission result, and hash placeholders.
- Rejected commands log without mutating simulation.

Data ownership:

- Input adapter owns transient input-to-action mapping.
- Runtime command boundary owns durable command records.
- Simulation owns accepted effects.

Verification:

- Command records round-trip.
- Missing actor/source/target where required is rejected.
- Rejected command leaves gameplay state unchanged.

Hard stops:

- Do not implement network transport.
- Do not make commands depend on SDL or Qt types.

### Packet 4: Target Discovery Policy

Owner: runtime/scene interaction.

Write scope:

- target discovery policy files;
- tests around overlapping targets and diagnostics;
- native product path call-site when ready.

Semantics:

- Build deterministic candidates from spatial query results plus action rules.
- Add target priority, action masks, disabled/out-of-range/not-attackable/missing-item
  diagnostics, and stable tie-breaks.
- Produce diagnostics for no target, disabled target, out of range, missing
  item, wrong faction, not attackable, blocked line, and no effect.

Data ownership:

- Scene spatial query remains primitive.
- Product discovery policy owns action-specific choice.
- Action admission owns final execution permission.

Verification:

- Overlapping pickup/door/enemy chooses by priority, then distance, then id.
- Disabled and out-of-range candidates are reported distinctly.
- Existing targetless interact behavior remains covered.

Hard stops:

- Do not hide diagnostics behind nearest-only behavior.
- Do not mutate target state during discovery.

### Packet 5: Combat State V0

Owner: runtime combat/gameplay.

Write scope:

- combat state/model files;
- snapshot extensions;
- tests for health, faction, alive/dead, cooldowns, and status basics.

Semantics:

- Add combat entity state keyed by stable entity id.
- Add faction, health, alive/dead, cooldowns, and current order/status fields.
- First demo avoids randomness.

Data ownership:

- Combat state belongs in runtime gameplay/product state.
- Actor/NPC position remains in existing actor/session state until a later
  entity unification packet.

Verification:

- State initializes from authored scenario fixture.
- Snapshot save/load preserves combat state.
- Defeated entity cannot receive attack/move orders unless explicitly allowed.

Hard stops:

- Do not add broad RPG stats.
- Do not start multiplayer or animation work here.

### Packet 6: Attack/Ability Execution V0

Owner: runtime action execution.

Write scope:

- command admission;
- attack/ability effect application;
- combat outcome tests.

Semantics:

- One attack command checks source, target, faction, alive/dead, range, and
  cooldown.
- Accepted attack applies damage, cooldown, and defeat.
- Objective state can observe defeat and set win/loss.

Data ownership:

- Admission owns rejection reasons.
- Simulation owns damage and objective mutation.
- Presentation owns feedback only.

Verification:

- Valid attack damages enemy.
- Out-of-range attack is rejected with no mutation.
- Wrong-faction or defeated target is rejected.
- Win condition triggers after enemy defeat.

Hard stops:

- Do not add animation timing until clock and command semantics are stable.

### Packet 7: Product Package Asset Manifest

Owner: runtime package/content plus native asset resolver.

Write scope:

- package manifest grammar extension;
- `AssetCatalog` or package-local asset manifest reader;
- native asset resolver/report tests.

Semantics:

- Add optional asset manifest reference such as `asset_manifest = "assets.toml"`.
- Validate ids, relative paths, missing files, duplicate ids, unsupported kinds,
  and invalid `.igmesh` files.
- Produce package asset report before renderer upload.

Data ownership:

- Package owns asset references.
- AssetCatalog owns logical ids and source paths.
- Native resolver owns CPU `.igmesh` load result.
- Renderer owns GPU upload.

Verification:

- Valid package asset manifest loads.
- Missing/invalid asset reports exact diagnostics.
- Renderer can still use fallbacks when resolver fails.

Hard stops:

- Do not add glTF/glb yet.
- Do not make package reader scan directories.

### Packet 8: Native Draw List Expansion

Owner: native scene projection.

Write scope:

- `NativeSceneDrawList`;
- model ids/slots for pickup, door, attack marker, objective marker;
- draw-list summary tests or report.

Semantics:

- Project doors, pickups/items, enemy actors, selected target, attack marker,
  and objective marker as semantic draw items.
- Keep draw items GPU-free.
- Include counts by model id for diagnostics.

Data ownership:

- Draw-list owns transient projection from runtime state.
- Renderer owns upload and draw submission.

Verification:

- Product demo reports expected draw item counts.
- Combat fixture reports player/enemy/objective markers.
- No package parsing occurs in draw-list code.

Hard stops:

- Do not add material graph or texture policy.

### Packet 9: Native Shell Tactical Controls

Owner: `apps/native_play`.

Write scope:

- native key/CLI mapping;
- `NativeProductSession`;
- scripted-control parser/tests.

Semantics:

- Add attack, ability, select/cycle target, slow, pause, step, retry, reset,
  save, and load controls.
- Replace thrown product save/load errors in normal play paths with status
  diagnostics that can feed HUD/debug overlay.
- Keep scripted controls on the same path as live controls.

Data ownership:

- Native shell owns input and display.
- Runtime owns command semantics and save compatibility.

Verification:

- Scripted controls can slow/pause/step/attack/save/load.
- Paused command planning works.
- Save/load errors produce explicit status.

Hard stops:

- Do not define gameplay truth in native shell.

### Packet 10: First Tactical Demo Fixture

Owner: content/runtime tests.

Write scope:

- new demo package under `engine/content/demos`;
- focused fixtures/tests;
- demo README command list.

Semantics:

- One player unit.
- One or two enemies.
- One movement decision.
- One attack or ability.
- Time slow/pause/step command issuing.
- Durable mid-fight save/load.
- Win/loss state.

Data ownership:

- Demo content owns authored facts.
- Runtime owns validation and execution.
- Native shell owns launch path.

Verification:

- One acceptance test proves load, target, attack, pause/step, save, load,
  resume, win/loss.
- One native scripted command proves the same loop through `iggy_native_play`.

Hard stops:

- Do not expand to squad tactics until this single-unit slice is green.

### Packet 11: Render And Compute Summary Gate

Owner: native app/render diagnostics.

Write scope:

- render/draw-list summary helper;
- CLI/debug output;
- tests that do not require live GPU where possible.

Semantics:

- Report package path, map size, draw items by model id, loaded/fallback asset
  counts, vertex/index counts, and frame status.
- Add one stress fixture near 500 draw items.

Data ownership:

- Diagnostics observe renderer/draw-list state; they do not mutate gameplay.

Verification:

- Summary output is deterministic.
- Stress fixture stays under the chosen first-demo budget or fails clearly.

Hard stops:

- Do not optimize batching/instancing until this summary proves the need.

### Packet 12: Shippable Demo Hardening

Owner: platform/native app/integration.

Write scope:

- package/build scripts;
- README/release checklist;
- save directory policy;
- acceptance command aggregation.

Semantics:

- Build can be handed to someone who is not reading the codebase.
- Saves land in a platform-appropriate durable directory.
- Failure states are visible: package failure, save mismatch, missing assets,
  renderer initialization failure.

Verification:

- Clean checkout build command.
- Demo acceptance command.
- Native launch command.
- Save/load across process restart.
- Known issues recorded.

Hard stops:

- Do not add public editor UX.
- Do not start online multiplayer.

## Ordered Builder Task List

1. Product save envelope.
2. Tactical clock state.
3. Serializable command boundary.
4. Target discovery policy.
5. Combat state v0.
6. Attack/ability execution v0.
7. Product package asset manifest.
8. Native draw-list expansion.
9. Native shell tactical controls.
10. First tactical demo fixture.
11. Render and compute summary gate.
12. Shippable demo hardening.

## Review Gates

Every packet must state:

- semantic owner;
- data owner;
- files touched;
- new tests;
- no-go surfaces explicitly left untouched;
- focused verification command;
- whether it changes save format, package format, command format, or renderer
  API.

Reviewer blocks:

- UI/native shell owns gameplay truth.
- Renderer parses package data.
- Save/load mutates active state before compatibility is checked.
- Paused tactical mode prevents command planning.
- Command records contain SDL/Qt/raw OS input data.
- Assets are saved inside user save files.
- Nearest-target behavior silently masks wrong target priority.
- Combat randomness appears before seed/state persistence.

## Open Decisions With Defaults

These are not blockers for packets 1 through 4.

- First controllable unit: default to one hero/unit.
- Pause command issuing: default to commands allowed while fully paused.
- Combat randomness: default to none until command replay and RNG persistence
  exist.
- First multiplayer shape: default to local authoritative host/session command
  log, no transport yet.
- First camera: default to tactical/isometric over the same simulation contract.
- First platform: default to macOS native proof, with Linux validation where
  renderer/CI claims matter.
