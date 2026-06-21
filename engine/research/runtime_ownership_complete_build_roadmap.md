# Runtime Ownership And Complete Build Roadmap

Updated: 2026-06-20

This is the runtime source of truth for the native 3D product path. It explains
what runtime owns, what the current runtime can already do, what must change for
3D, and the complete build sequence from current product-loop scaffolding to a
shippable native 3D runtime with durable saves and multiplayer-ready command
contracts.

The 3D transition is now planned as a fresh standalone side repo named
`iggy3d`. Use `engine/research/iggy3d_side_repo_plan.md` for the no-legacy-build
plan, source boundaries, and first implementation paths.

## Runtime Charter

Runtime owns the game truth below shells, tools, and renderer backends.

Runtime owns:

- product session lifecycle;
- package load acceptance and compatibility;
- simulation state;
- player and session commands;
- command admission and rejection diagnostics;
- deterministic stepping policy;
- tactical clock state;
- world/entity state;
- interaction, pickup, combat, objective, and win/loss state;
- durable runtime save/load semantics;
- replay/multiplayer-ready command logs;
- 3D scene projection contracts consumed by renderer-facing layers.

Runtime does not own:

- Qt or SDL raw events;
- UI widgets, HUD drawing, menus, or editor panels;
- Vulkan device, swapchain, shader, pipeline, buffer, or texture lifetime;
- authoring-tool mutation UX;
- asset creation tools;
- renderer-specific mesh upload policy;
- platform-specific save directory selection, except through an abstract policy
  surface.

The boundary rule is: shells collect input and display output, renderer draws
resolved scene facts, tools author packages, but runtime decides what the game
state is and how it changes.

## Current Runtime Capability Map

### Exists And Usable

- Explicit TOML/package load through `RuntimeGameplayProductScenarioLoader`.
- Product-owned loop state through `RuntimeGameplayProductLoopState`.
- One-frame product stepping through caller-provided input.
- Input adapter and binding from transient product controls into player intents.
- Product input accumulator for held controls and one-shot events.
- Target query, target-context enrichment, and targetless interaction fallback.
- Reach-gated interaction execution through existing interaction/effect paths.
- Pickup, inventory mutation, required-item door interaction, and target toggle.
- Runtime gameplay state carrying session, command queue, interaction,
  inventory, NPC actor registry, and NPC control registry.
- NPC actor/control movement surfaces and runtime-adjacent planning/application
  helpers.
- Gameplay snapshot save/load and gameplay save-slot store.
- Native no-Qt play shell through `iggy_native_play`.
- Scripted native controls for deterministic product-loop smoke checks.
- Native scene draw-list extraction for floor, wall, NPC actor, and player.
- Native Vulkan proof renderer with `.igmesh` file loading/fallback slots.

### Partial Or Scaffold

- Product save/load: current native save/load stores only
  `RuntimeGameplayState`, not a full product session.
- Pause/retry/reset: available in native play, but pause is input focus rather
  than simulation time.
- 3D runtime: current 3D path projects 2D/tile gameplay state into model slots;
  there is not yet a runtime-owned 3D world/entity contract.
- Targeting: current target discovery is nearest spatial target, not
  action-aware 3D target acquisition.
- Assets: renderer uses default app-local mesh filenames; authored packages do
  not yet own 3D asset manifests for runtime consumption.
- Multiplayer: command queue and state surfaces are useful, but command records,
  authority, replay, entity ids, and determinism checks are not complete.
- Camera: camera projection exists in app/product presentation pieces, but the
  runtime does not yet own a full camera rig contract for fixed, isometric,
  tactical orbit, third-person, and FPS modes.

### Missing For Complete Runtime

- Product-session state above the current product loop.
- Versioned product save envelope with package/scenario/content compatibility.
- Tactical simulation clock.
- Durable command log with accepted and rejected commands.
- Runtime-owned stable entity ids for 3D actors, props, pickups, doors, hazards,
  objectives, and player slots.
- Runtime-owned 3D transforms and collision/interaction volumes.
- Action-aware target discovery and command admission.
- Combat state and execution.
- Objective/win/loss lifecycle.
- Package-driven asset identity for runtime scene projection.
- Runtime 3D scene graph/projection contract independent of Vulkan.
- Local multiplayer player/session model.
- Online-ready replay, state hash, and command replication boundaries.

## 3D Runtime Direction

Existing 2D/tile systems remain useful as simulation and authoring scaffolding.
They should not become the shipping renderer contract.

The runtime should evolve toward this stack:

```text
Authored package
  -> package/content acceptance
  -> product session state
  -> command admission and tactical clock
  -> authoritative simulation state
  -> runtime 3D entity/world state
  -> 3D scene projection
  -> renderer draw list / model slots
  -> native renderer backend
```

The key technical step is introducing a runtime-owned 3D world/entity layer that
sits between gameplay simulation and renderer projection.

Target shape:

```text
RuntimeEntity3D
  stable entity id
  owner/simulation id
  entity kind
  world transform
  collision proxy
  interaction volume
  model or asset ref
  faction/team/status tags
  camera target data
  persistence flags
```

Rules:

- 3D entity state belongs to runtime, not renderer.
- Renderer receives draw items or resolved model-slot work, not gameplay truth.
- Physics/collision may start from tile proxies, but runtime-facing collision
  APIs should become 3D-volume capable.
- Camera controls and ray/target projection must be presentation/input policy
  over the same simulation contract, not separate gameplay modes.
- Multiplayer uses commands and stable entity ids; it does not replicate raw
  input events or renderer facts.

## Complete Runtime Architecture

### Product Session

`RuntimeProductSessionState` should become the top-level runtime state for play.

It should own:

- package identity;
- scenario identity;
- content version/hash data;
- lifecycle state;
- active player slots;
- current gameplay state;
- initial/retry state;
- product loop cursor;
- tactical clock;
- command log;
- random seed/state when randomness exists;
- objective/completion state;
- save compatibility metadata.

The current `RuntimeGameplayProductLoopState` remains a useful inner state but
is not enough for durable product play.

### Command And Authority

Runtime should treat single-player as a local authoritative session.

Command flow:

```text
shell input
  -> transient input frame
  -> proposed command
  -> command admission
  -> accepted/rejected command record
  -> simulation step
  -> runtime state mutation
  -> presentation projection
```

Command records must eventually support:

- player/client source id;
- actor/entity id;
- stable target id or target location;
- sequence number;
- simulation tick/frame;
- command type;
- payload;
- admission result;
- pre/post state hash placeholders;
- replay diagnostics.

This is the multiplayer seam. Online transport should wait until local
authority, command logs, replay, and deterministic-enough stepping exist.

### Tactical Clock

Runtime must own simulation time separately from input focus and render frames.

Clock modes:

- normal;
- slow;
- paused;
- step requested.

Runtime rules:

- paused time accepts command planning;
- render frames do not advance simulation by themselves;
- simulation ticks drive AI, cooldowns, windups, recovery, status effects, and
  objectives;
- save/load preserves clock state.

### Runtime 3D World

The 3D world layer should start as an adapter over existing 2D/tile data and
then become the long-term product-facing simulation representation.

First conversion:

- player tile/position -> 3D transform;
- NPC actor position -> 3D transform;
- wall/floor tile -> terrain/wall entity or draw item;
- pickup/item drop -> 3D pickup entity;
- interaction target -> 3D interaction volume;
- door target -> 3D door entity;
- objective marker -> 3D objective/debug entity.

Later conversion:

- character capsule/volume collision;
- camera collision;
- 3D raycast target projection;
- 3D line-of-sight and cover;
- non-grid movement if the game direction requires it.

### 3D Scene Projection

`NativeSceneDrawList` or its successor should be the runtime-to-renderer
projection boundary.

It should project semantic items:

- terrain;
- floors/walls/doors;
- player actors;
- NPC/enemy actors;
- pickups/items;
- props/hazards;
- interaction markers;
- attack/ability markers;
- objective markers;
- camera/debug overlays when enabled.

Projection rules:

- draw-list items are transient;
- draw-list items are GPU-free;
- draw-list construction does not parse packages;
- renderer does not inspect gameplay state;
- diagnostics report item counts by kind/model id.

### Save/Load

Runtime save/load should use a product save envelope around existing gameplay
snapshots.

Required durable facts:

- save format id/version;
- package identity and content hash;
- scenario identity and source hash;
- product loop cursor;
- runtime gameplay snapshot;
- runtime 3D entity state when it becomes authoritative;
- tactical clock state;
- command log cursor and records;
- objective/completion state;
- combat state;
- random seed/state when randomness exists;
- compatibility diagnostics.

Save files must store state and content identity, not mesh bytes or renderer
resources.

### Content And Assets

Runtime package acceptance should become asset-aware before renderer expansion.

Ownership chain:

```text
package.toml
  -> scenario data
  -> asset manifest
  -> logical AssetCatalog records
  -> native asset resolver
  -> CPU mesh/asset diagnostics
  -> renderer model slots
```

Runtime owns package compatibility and logical asset references. Renderer owns
GPU upload and drawing. Asset creation/import remains an internal tooling lane.

## Complete Build Roadmap

### Phase 0: Runtime Ownership Baseline

Goal: make runtime ownership and current capability explicit.

Deliverables:

- this document linked from primary roadmap docs;
- current runtime capability map kept current;
- complete build roadmap linked from the 3D platform roadmap;
- builder packets identify runtime-owned versus renderer/tool-owned surfaces.

Exit criteria:

- a builder can tell whether a change belongs in runtime, native shell,
  renderer, authoring, platform, or content.

### Phase 1: Product Session Save Envelope

Goal: make runtime play durable before expanding combat and 3D state.

Deliverables:

- product save envelope;
- package/scenario/content compatibility metadata;
- product loop cursor persistence;
- gameplay snapshot wrapping;
- native save/load integration through runtime semantics;
- mismatch diagnostics.

Exit criteria:

- save, quit/reset, load, and continue restore package identity and loop cursor,
  not only gameplay state.

### Phase 2: Tactical Clock

Goal: separate simulation time from input focus and render frames.

Deliverables:

- runtime clock state;
- normal/slow/paused/step modes;
- paused command planning;
- clock persistence in product saves;
- native scripted clock controls.

Exit criteria:

- a paused session can accept commands, step once, save, load, and resume with
  the same clock state.

### Phase 3: Serializable Command And Admission Layer

Goal: make commands durable and multiplayer-ready.

Deliverables:

- command record v0;
- admission result model;
- accepted/rejected command log;
- source/actor/target/sequence/tick fields;
- replay-oriented diagnostics;
- no raw SDL/Qt input in command records.

Exit criteria:

- runtime can reject invalid commands without mutation and record why.

### Phase 4: Runtime 3D Entity Model

Goal: introduce runtime-owned 3D world facts without rewriting all simulation.

Implementation path: create the new 3D runtime in the standalone `iggy3d` repo
described by `engine/research/iggy3d_side_repo_plan.md`. Current 2D runtime
files are reference material only, not adapter inputs or build dependencies.

Deliverables:

- `RuntimeEntity3D` or equivalent state;
- stable entity ids;
- transform component;
- model/asset ref;
- collision proxy placeholder;
- interaction volume placeholder;
- adapter from current player/NPC/item/door/tile state into 3D entities.

Exit criteria:

- the current product demo can be represented as runtime 3D entities before
  renderer projection.

### Phase 5: 3D Scene Projection V2

Goal: make renderer input semantic and complete enough for a playable 3D slice.

Deliverables:

- draw-list/model ids for doors, pickups, props, hazards, markers, objectives,
  enemies, and selected targets;
- draw-list summary diagnostics;
- stable item ordering;
- tests or deterministic summaries for expected draw item counts.

Exit criteria:

- player, floor, walls, doors, pickup, and NPC/enemy are visible as semantic 3D
  draw items.

### Phase 6: Package-Driven Runtime Assets

Goal: move mesh selection out of app-local defaults and into package data.

Deliverables:

- package asset manifest;
- logical asset catalog records;
- native `.igmesh` resolver diagnostics;
- loaded/fallback/invalid/missing asset report;
- renderer receives resolved model-slot data.

Exit criteria:

- a package can choose demo meshes without changing renderer code.

### Phase 7: 3D Camera And Input Projection

Goal: support camera modes over one simulation contract.

Deliverables:

- backend-free camera rig state;
- fixed/isometric/tactical orbit/third-person/FPS policy models;
- camera target contract;
- 3D target projection/raycast placeholder;
- input mapping by camera mode;
- save/load policy for camera state if it becomes product state.

Exit criteria:

- switching camera modes changes presentation/input projection, not gameplay
  truth.

### Phase 8: Runtime Targeting And Interaction In 3D

Goal: move from nearest 2D target discovery to action-aware 3D target admission.

Deliverables:

- target discovery policy;
- 3D interaction volumes;
- action masks;
- target priorities;
- range/reach diagnostics;
- disabled/out-of-range/missing-item/wrong-faction/no-effect diagnostics;
- line-of-sight placeholder if not yet implemented.

Exit criteria:

- overlapping targets choose deterministically and report why other candidates
  were rejected.

### Phase 9: Combat Runtime V0

Goal: create the first real game loop.

Deliverables:

- combat entity state;
- faction/team;
- health/alive/dead;
- cooldowns/statuses;
- one attack or ability;
- objective/win/loss state;
- save/load coverage.

Exit criteria:

- a native 3D package can be played, paused/slowed, attacked, saved, loaded,
  won/lost, and retried.

### Phase 10: Native Runtime UX Integration

Goal: make the runtime playable through native shell without developer-only
knowledge.

Deliverables:

- continuous runtime pump;
- native controls for movement, target, attack, ability, time, save/load,
  retry/reset;
- status surfaces for command rejection, save mismatch, missing assets, and
  renderer failure;
- debug overlay data from runtime diagnostics.

Exit criteria:

- a person can play the demo in native 3D without reading test output.

### Phase 11: Local Multiplayer Foundation

Goal: prove multiple local players before online.

Deliverables:

- player slot model;
- controller assignment;
- multi-source command streams;
- local co-op and local battle session rules;
- shared/split camera policy;
- local save/resume policy.

Exit criteria:

- multiple local players can issue commands into one authoritative runtime
  session.

### Phase 12: Online-Ready Runtime Boundary

Goal: make online transport possible without gameplay rewrite.

Deliverables:

- command replay;
- state hash/checkpoint hooks;
- deterministic audit;
- package/content handshake;
- host/session authority model;
- disconnect/reconnect state policy;
- accepted nondeterminism list.

Exit criteria:

- a local session can be replayed or inspected from package, initial state, and
  command log well enough to design replication.

### Phase 13: Runtime Hardening And Release Gate

Goal: stabilize the runtime for a shippable demo.

Deliverables:

- runtime acceptance demo;
- product save compatibility tests;
- command replay tests;
- package asset compatibility tests;
- native 3D summary/screenshot proof;
- performance budget reports;
- crash/error diagnostics;
- runtime known-issues list.

Exit criteria:

- a clean checkout can build, run, save, load, complete, and verify the native
  3D demo path.

## Packet Order For Builders

1. Product session save envelope.
2. Tactical clock state.
3. Serializable command/admission record.
4. Runtime 3D entity model.
5. 3D scene projection v2.
6. Package-driven asset manifest/resolver.
7. Camera rig and input projection.
8. 3D target discovery and interaction admission.
9. Combat state v0.
10. Attack/ability execution v0.
11. Native runtime UX integration.
12. Local multiplayer player slots.
13. Command replay and state hash hooks.
14. Runtime hardening/release gate.

## Verification Matrix

Every runtime packet needs focused verification.

Minimum gates:

- `git diff --check`;
- focused runtime CTest target;
- save/load compatibility test if state format changes;
- package compatibility test if package format changes;
- native scripted-control smoke if native shell behavior changes;
- deterministic summary or screenshot proof if 3D projection changes;
- command replay/admission test if command format changes.

Full gate before claiming a runtime milestone:

```sh
cmake -S engine -B engine/build
cmake --build engine/build
ctest --test-dir engine/build --output-on-failure
git diff --check
```

Native proof gate, when local dependencies exist:

```sh
engine/build/iggy_native_play --play engine/content/demos/product_loop_demo
engine/build/iggy_native_play --dump-static-model-load-report
```

## Runtime Review Blocks

Block a runtime packet if:

- renderer owns gameplay state;
- UI/native shell owns gameplay semantics;
- raw input events are persisted;
- save/load mutates active state before compatibility is checked;
- package parsing moves into renderer;
- GPU resources appear in runtime state;
- 3D draw-list items become save truth;
- pause blocks tactical command planning;
- command records depend on SDL, Qt, or platform event types;
- multiplayer work starts before local command/session authority exists;
- randomness appears before seed/state persistence.

## Documentation Rules

Runtime changes must update documentation when they change:

- ownership boundaries;
- product/session lifecycle;
- save/package/command format;
- 3D entity or draw-list contracts;
- verification commands;
- no-go surfaces;
- compute/storage budget assumptions.

The builder handoff for each runtime packet should include:

- purpose;
- current source facts;
- semantic owner;
- data owner;
- write scope;
- explicitly untouched surfaces;
- tests added or changed;
- focused verification command;
- known follow-up packets.
