# Iggy 3D Product Platform Roadmap

Updated: 2026-06-20

This roadmap records the current product direction after the 3D/platform
decision. It supersedes any product framing that treats the project as a 2D
game or as a single-camera tactical-only prototype.

The first rule is simple: Iggy is a native 3D game. Existing 2D/tile systems are
allowed to remain as gameplay, collision, authoring, fixture, and debug data
scaffolding, but they are not the shipping game renderer.

Runtime ownership and the complete native 3D runtime build sequence are tracked
in `engine/research/runtime_ownership_complete_build_roadmap.md`.
The dedicated fresh 3D runtime side repo is `iggy3d`; its no-legacy-build plan
is tracked in `engine/research/iggy3d_side_repo_plan.md`.

## Locked Product Direction

- Runtime: native 3D product path, no Qt shipping shell.
- Renderer: native Vulkan path first, with platform backends abstracted only
  when concrete platform gates require it.
- Visual style: stylized low-poly.
- Camera modes: fixed, isometric, free third-person, tactical orbit, and FPS.
- Platforms: macOS, Linux, Windows, Steam Deck, and Nintendo-family targets.
- Multiplayer: couch co-op, online co-op, local 4-player battle, and online
  4-player battle.
- Tools: internal developer tool first; public editor UX is not a first target.
- Content: authored packages should feed the native runtime and internal tools.

## Product Definition

The long-term product is a 3D game platform capable of running authored
stylized-low-poly scenarios with multiple camera modes, local and online
multiplayer modes, durable saves where applicable, and internal tooling for
content iteration.

The project is not done when a runtime test passes. It is done when a player can
launch a native build, load packaged content, see and control a real 3D
character, interact with a 3D world, play alone or with other players in the
supported modes, and run on the target platform set.

## Non-Goals And Guardrails

- Do not graduate the 2D render command path into the product renderer.
- Do not make Qt a shipping runtime dependency.
- Do not build material, glTF, or network architecture before the native 3D
  playable slice is visually proven.
- Do not let every camera mode create a separate game. Camera rigs must sit over
  one simulation/input/scene contract.
- Do not start with online multiplayer. Local multiplayer and local-host
  command/session architecture come first.
- Do not make Nintendo support depend on assumptions about private SDK details.
  Keep the core portable, then gate Nintendo work behind official SDK/devkit
  access.

## Architecture Direction

### Runtime Contract

The simulation must stay camera-independent, renderer-independent, and
platform-independent. Player commands, entity ids, world state, interaction
targets, collision, and save/load data belong below the renderer and shell.

### 3D Projection Contract

`NativeSceneDrawList` or its successor is the product-facing 3D projection
boundary. It should project gameplay state into semantic 3D draw items:

- terrain;
- walls and doors;
- player actors;
- NPC/enemy actors;
- pickups and props;
- interaction markers;
- camera/debug overlays when enabled.

Runtime 2D render commands may remain as debug or legacy product-panel support,
but they are not the canonical game scene.

### Renderer Contract

The renderer should own GPU resources, frame submission, shader pipelines,
model-slot binding, render diagnostics, and platform backend integration. It
should not own gameplay state, command semantics, save/load truth, or content
package validation.

### Tooling Contract

Internal tools author and validate content packages. They should produce data
that the native runtime can load without editor-only state.

## Phase 0: Direction Sync And Baseline

Goal: make the 3D product direction durable and prevent old 2D assumptions from
driving implementation order.

Deliverables:

- This roadmap linked from the main roadmap.
- Native play identified as the product proof shell.
- 2D render path documented as debug/scaffolding only.
- Dirty working tree and live packet state reconciled before integration claims.
- Baseline proof commands listed for native tests, product runtime tests,
  render/resource tests, `iggy_native_play --help`, and static model load report.

Exit criteria:

- A planner, builder, or reviewer can tell which surfaces are product runtime,
  which are debug scaffolding, and which are deferred.

## Phase 1: Native 3D Playable Slice

Goal: prove a real no-Qt 3D playable slice before broader renderer or
multiplayer work.

Deliverables:

- `iggy_native_play` launches an authored scenario.
- Player, floor, walls, doors, pickups, and at least one NPC render as 3D
  model slots.
- Scripted controls prove movement and collision.
- Slot diagnostics report file mesh versus fallback for every visible slot.
- One native visual proof artifact exists: captured frame, nonblank/pixel
  check, or equivalent deterministic render summary.
- Runtime blocked movement and interaction diagnostics are visible.

Exit criteria:

- The player can be seen moving through a 3D room with collision and visible
  world objects, without Qt.

## Phase 2: Camera Rig Framework

Goal: support every target camera style over the same simulation and scene
contract.

This phase should be implemented in the standalone `iggy3d` repo, not by
extending the existing 2D presentation camera or the old `iggy` runtime into
the product authority.

Camera modes:

- fixed: authored camera volumes/anchors for rooms or encounters;
- isometric: stable tactical overview with constrained rotation/zoom;
- tactical orbit: orbit around selected actor, party, or encounter center;
- free third-person: follow camera with player-relative control;
- FPS: first-person camera with collision, pitch/yaw limits, and interaction
  projection.

Deliverables:

- Backend-free camera rig data model.
- Shared camera target contract: actor, party, point, volume, or authored anchor.
- Per-camera input mapping policy.
- Per-camera pointer/raycast/target projection policy.
- One test room proving all five camera modes can view and control the same
  playable slice.
- Debug overlay for active camera mode, target, yaw/pitch/orbit distance, and
  projected interaction target.

Exit criteria:

- Switching cameras does not fork gameplay. It only changes presentation and
  input projection.

## Phase 3: Platform Layer

Goal: make platform support an explicit architecture lane before platform bugs
become renderer or gameplay bugs.

Targets:

- macOS;
- Linux;
- Windows;
- Steam Deck;
- Nintendo-family hardware, gated by official SDK/devkit access.

Deliverables:

- Platform capability report: renderer backend, controller support, save path,
  filesystem behavior, display modes, and performance class.
- Input abstraction for keyboard/mouse and controllers.
- Save directory policy by platform.
- Build/package scripts per public target as each target becomes active.
- Steam Deck profile: controller-first input, handheld performance budget, and
  readable UI scale.
- Nintendo portability guardrails: no SDK assumptions in open code, no
  platform-specific APIs leaking into gameplay/runtime contracts.

Exit criteria:

- Platform differences are isolated to platform/backend layers and do not
  rewrite simulation, content packages, or authoring data.

## Phase 4: Stylized Low-Poly Asset Pipeline

Goal: replace placeholders with package-driven 3D content while keeping the
renderer simple.

Deliverables:

- `.igmesh` remains the first static mesh proof format.
- Runtime model-slot policy expands beyond `Floor`, `Wall`, `NpcActor`, and
  `Player` to include doors, pickups, props, hazards, and camera markers.
- Asset reports identify missing, invalid, fallback, and loaded assets.
- Package directory load data feeds model-slot selection through a narrow
  renderer-facing boundary.
- Constrained glTF/glb subset is added only after `.igmesh` proves the slot and
  package contracts.
- Material policy starts with vertex color and flat color; texture support comes
  after static mesh visibility, fallback, and package diagnostics are stable.

Exit criteria:

- A content package can replace all primary visible placeholders with
  stylized-low-poly 3D assets without code changes.

## Phase 5: Single-Player Core Loop

Goal: turn the visual slice into a playable game loop before multiplayer.

Deliverables:

- Continuous product frame pump.
- Pause, resume, reset, retry, save, and load.
- Interaction flow: pickup, door, target selection, action execution.
- Basic combat or encounter rules.
- Objective, completion, and failure conditions.
- HUD/debug feedback sufficient to play without reading logs.

Exit criteria:

- A short authored 3D scenario can be started, played, saved, loaded, won, lost,
  and retried in the native runtime.

## Phase 6: Local Multiplayer Foundation

Goal: make multiple local players work before networked modes.

Deliverables:

- Multiple local player entities.
- Controller assignment and join/leave flow.
- Shared-camera and split-camera policies.
- Local co-op spawn, revive, inventory, and interaction rules.
- Local 4-player battle mode with spawn/team/free-for-all rules.
- Camera mode compatibility matrix for co-op and battle.

Exit criteria:

- Four local players can join and play a small co-op or battle scenario on one
  machine.

## Phase 7: Multiplayer-Ready Simulation Boundary

Goal: make the local game behave like a local authoritative session so online is
transport and authority work, not a gameplay rewrite.

Deliverables:

- Serializable player command objects.
- Stable network-facing entity ids.
- Command log and replay hooks.
- Determinism audit: identify deterministic surfaces and accepted nondeterminism.
- Random seed/state capture where randomness is used.
- Host/session state model for local host authority.
- Rejection diagnostics for invalid commands without mutating simulation state.

Exit criteria:

- A local session can be replayed or inspected from command/state data well
  enough to define online replication.

## Phase 8: Online Co-Op

Goal: ship cooperative online play on top of the multiplayer-ready simulation.

Deliverables:

- Session/lobby model.
- Host-authoritative or server-authoritative decision.
- Command replication.
- State snapshot or delta replication as needed.
- Join, disconnect, reconnect, timeout, and host migration policy.
- Latency handling: input delay, prediction, or simple conservative sync based
  on game feel.
- Online save/resume policy for co-op sessions.

Exit criteria:

- Players on separate machines can complete a small co-op scenario.

## Phase 9: Online 4-Player Battle

Goal: turn the battle mode into a networked competitive mode.

Deliverables:

- Match rules: free-for-all or teams, scoring, respawn, round timing.
- Matchmaking or direct invite flow.
- Authority and anti-cheat baseline appropriate to project scale.
- Network replication tuned for combat readability.
- Spectator/debug mode for inspecting match state.
- Result reporting and rematch flow.

Exit criteria:

- Four online players can complete a battle match with stable scoring and
  acceptable latency behavior.

## Phase 10: Internal Authoring Tool

Goal: make content creation repeatable before trying to make tools public.

Deliverables:

- Room/space authoring.
- Spawn points for single-player, co-op, and battle modes.
- Doors, pickups, NPCs, hazards, objectives, and camera volumes.
- Camera-mode preview.
- Asset slot binding and validation.
- Multiplayer metadata: teams, spawn groups, player counts, battle arenas.
- Package check/lint/build commands.
- Native launch from authored package.

Exit criteria:

- A developer can create or edit a playable 3D package, validate it, and launch
  it in native play without hand-editing runtime code.

## Phase 11: Renderer Upgrade

Goal: move from proof renderer to production renderer only after the playable
path is real.

Deliverables:

- Renderer resource lifetime cleanup and RAII.
- Device-local/staged mesh upload path.
- Render graph or explicit pass policy if needed.
- Material v1: flat color/vertex color, then texture subset.
- Lighting strategy: unlit/stylized first, then simple directional or baked
  lighting if needed.
- Debug overlays for draw count, mesh count, material count, missing assets,
  fallback slots, and frame timing.
- Headless or capture-based render validation for CI/platform gates.

Exit criteria:

- Renderer quality and validation support real stylized-low-poly scenes without
  changing gameplay contracts.

## Phase 12: Platform Certification And Packaging

Goal: prepare each target platform as a product target, not a local build
accident.

Deliverables:

- macOS package and smoke tests.
- Linux package and dGPU validation.
- Windows package and input/save/render validation.
- Steam Deck profile and performance/readability validation.
- Nintendo SDK/devkit integration plan once access exists.
- Crash/error reporting.
- Settings: display, input, audio, accessibility basics.
- Release checklist per platform.

Exit criteria:

- The same authored content can be built, launched, played, and validated on the
  active platform set.

## Immediate Packets

1. Sync docs so the 3D product/platform roadmap is the current product target.
2. Add native model-slot diagnostics for loaded file versus fallback.
3. Add focused `NativeSceneDrawList` projection coverage.
4. Prove one native visual frame from a scripted scenario.
5. Add camera rig data types and a fixed/isometric proof over the existing room.
6. Add tactical orbit, third-person, and FPS camera proofs over the same room.
7. Expand 3D draw-list items for doors, pickups, and interaction markers.
8. Add local multi-player entity/controller assignment model.
9. Define serializable command objects before online work.
10. Promote internal authoring package output into native 3D launch input.

## Open Questions

These should not block Phase 1, but they must be answered before hardening later
phases:

- Which camera is the default first impression: isometric, third-person, or
  fixed?
- Is FPS a full gameplay mode, an inspection mode, or both?
- Are co-op and battle character rosters identical?
- Is online authority host-based, dedicated-server based, or both?
- Is Steam Deck treated as a first-class launch target or a required portable
  compatibility gate after Linux?
- What exact Nintendo hardware generation is targeted once official access is
  available?
- What minimum visual quality defines "stylized low-poly enough" for the first
  vertical slice?
