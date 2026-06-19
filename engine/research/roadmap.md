# Iggy Roadmap

This is the internal project roadmap checkpoint. It is broader than the
authoring batch bucket and should be updated after project-shaping batches, not
after every small slice.

For the detailed completion backlog, use
`engine/research/project_completion_todo.md`.
For phase order and dispatch gates, use
`engine/research/project_completion_execution_plan.md`.

## Current Checkpoint

- The TOML authored-scenario lane has moved from a CLI harness into reusable
  runtime facades and projections.
- The package boundary is implemented as a thin wrapper over the single-file
  TOML scenario facade.
- The read-only authoring preview model exists for explicit TOML files and
  explicit package paths.
- The first read-only UI preview consumer is integrated in the existing Qt
  shell via explicit `--preview PATH` and `--preview-mode run|trace|check|lint`
  options.
- Authoring V1 is release-candidate defined for engine/test/dev-tool use, with
  fixture-scale budgets and cleanup/prune gates recorded.
- Full verification has stayed green through the recent stretches; the latest
  UI preview consumer integration reported 343/343 tests.
- Short-term implementation packets live in
  `engine/research/authoring_batches/`.

Recently completed optimized stretches:
- Facade/diagnostics/output contracts: `16`, `31`, `46`, `59`, `28`, `47`,
  `48`.
- Fixture/expectation hardening: `32`, `49`, `33`, `56`, `57`, `58`, `34`.
- Source-plan contract hardening and cleanup: `21`, `22`, `39`, `52`, `38`,
  `13`, `24`.
- Package boundary: `14`, `17`, `18`, `44`, `50`, `51`, `24`.
- Editor read-only preview: `15`, `19`, `26`, `23`.
- Authoring V1 release-candidate closure: `29`, `27`, `25`, `60`, `24`.

## Have

### Engine Foundation

- CMake/test foundation with focused subsystem test targets and full CTest
  verification.
- Core math/resource types and backend-neutral server primitives for render,
  navigation, and physics queries.
- Level/map/cache lanes for tile maps, runtime state, render cache, collision
  cache, derived caches, and level mutation/cache updates.
- Newer scene AI/NPC/player lanes for profile traits, AI maps, actor/control
  state, movement planning/application, player input intents, and command
  planning.
- Interaction and inventory foundations: targets, effects, reach/query/plan/apply
  flow, drops, pickup policy, inventory transfer, and required-item interaction
  gating.
- Runtime gameplay/session/scenario infrastructure: command queues, player input,
  interaction, pickup, NPC movement, orchestrated frames, profile scenarios, and
  scenario runners.
- Save/load foundations: session/gameplay snapshots, chunk archive/envelope,
  file IO, save slots, and autosave rotation.

### Authoring And Scenario Execution

- TOML source-plan authoring lane with grid, legend, cells, regions, controls,
  profiles, interaction targets/effects, item drops, player commands, safety
  flags, promotion policy, expectations, and trace expectations.
- Source-plan format/version policy pinned to
  `format_id = "iggy:ascii-source-plan"` and `version = 1`.
- Test-owned TOML schema snapshot for supported tables/keys.
- TOML subset stress coverage for supported authoring patterns and unsupported
  shapes.
- Source-plan validator and TOML diagnostics with table/key/line/index metadata.
- Runtime authoring adapter and source-plan-to-profile-scenario converter.
- Region-to-AI-map promotion, profile scenario validation, profile scenario run,
  and final debug row projection.
- Runtime `RuntimeGameplayTomlScenarioFacade` for run/lint/check/trace over
  explicit one-file TOML scenarios.
- Runtime `RuntimeGameplayTomlScenarioPackageReader` for shared explicit
  package directory/manifest path reading without package discovery or scanning.
- Runtime `RuntimeGameplayTomlScenarioPackageFacade` for explicit package
  directory or manifest paths, delegating to the TOML scenario facade.
- Runtime `RuntimeGameplayAuthoringPreviewModel` for read-only preview over TOML
  files and packages.
- Runtime `RuntimeGameplayProductScenarioLoader` for load-only product scenario
  setup over explicit TOML files, package directories, and `package.toml` paths;
  it reads, adapts, and validates without running frames.
- Runtime `RuntimeGameplayProductLoop` for product-owned one-frame stepping over
  successful loader output; it consumes caller-provided player input intents and
  optional per-step input context overrides, and carries current gameplay state
  without owning raw input, presentation, or persistence.
- Scene/player `PlayerInputBinding2D` for device-agnostic normalized action to
  `PlayerInputIntent2D` binding; it reports binding issues and preserves action
  order without applying gate rules or command mapping.
- Runtime `RuntimeGameplayProductInputAdapter` for product/app transient input
  events to normalized `PlayerInputBindingAction2D` actions plus carried binding
  context; it does not call the product loop, step gameplay, or own Qt/device,
  camera, render, save/load, gate, or command behavior.
- Runtime `RuntimeGameplayProductInputAccumulator` for shell-neutral transient
  product input state, storing held movement controls and pending one-shot
  product events by value, emitting held movement each requested frame and
  draining one-shots, and preserving explicit `PrimaryPoint`/`PrimaryTile`
  pressed events as payload-carrying one-shots without Qt/raw event types,
  cadence policy, persistence, or pointer synthesis.
- Runtime `RuntimeGameplayProductPointerProjection` for Qt-free viewport-local
  point projection through existing `CameraView` normalization into world point
  plus `tileForPoint(...)` tile, returning status/flags/world/tile for future
  policy without owning Qt mouse input, viewport/canvas coordinates, target
  lookup, interaction execution, or persistence.
- Qt shell `productViewport_` owner for product play sessions:
  `QFrame#productViewport` is created only when product play mode exists, including
  failed/not-ready play sessions, with expanding layout inside `QFrame#mainSlot`.
- Qt shell thin primary-tile mouse consumer on `productViewport_`: focused,
  ready left-button press records exactly one transient `PrimaryTile` pressed
  event in the product input accumulator after normalizing Qt pixel-local
  coordinates into configured product camera-view span; no `PrimaryPoint`, world
  payload, right/middle/move/wheel/double-click/drag/hover behavior, frame
  execution, persistence, or runtime/product API change.
- Qt shell thin product viewport render command drawer: `QFrame#productViewport`
  now paints existing latest product play frame render commands from app-shell
  transient `latestProductPlayModeFrame_` data as untextured debug/material
  rectangles. It maps latest-frame camera-view bounds to widget pixels, keeps
  command vector order, draws only quad commands, ignores textures, uses Qt-local
  material debug colors, and does not execute frames, change runtime/scene APIs,
  own long-term renderer semantics, or persist presentation data.
- Qt shell thin product target highlight overlay: `ProductViewportWidget` accepts
  a const pointer to the latest transient target-context diagnostics and paints
  a Qt-local outline/tint marker after command quads when a copied diagnostics
  target exists. It uses copied target position/radius only, performs no
  paint-time target query, treats reach as visual annotation, and does not add
  labels, selection, interaction execution, persistence, or runtime API changes.
- Scene `InteractionTargetSpatialQuery2D` for pure spatial lookup over
  `InteractionTarget2DRegistry`: enabled targets only, Euclidean distance,
  clamped target/extra radius, nearest eligible target with registry-order ties,
  `find(...)` and `findTileCenter(...)` status/results, and no Qt/runtime/product
  wiring, reach/LOS/occupancy/pathfinding/kind-priority policy, click-to-interact
  execution, persistence, or gameplay semantics.
- Runtime `RuntimeGameplayProductInteractionTargetQuery` for read-only
  product-state target query reports: point or tile-center queries delegate to
  `InteractionTargetSpatialQuery2D`, found targets are copied into the result,
  reach is annotated only when the current product state has a player, and the
  surface does not mutate selected/hovered context, convert `PrimaryTile` to
  `Interact`, execute commands/effects, persist query state, or add Qt/UI
  behavior.
- Runtime `RuntimeGameplayProductInputTargetContext` for transient binding
  context enrichment from an already-computed target query report: a valid found
  target projects only hovered target id into a copied
  `PlayerInputBindingContext2D`, unchanged paths preserve the base context
  exactly, and the helper does not own hover lifecycle, selected target state,
  reach gating, input conversion, execution, persistence, or Qt/UI behavior.
- Runtime `RuntimeGameplayProductInputFrameTargetContext` for opt-in pre-frame
  enrichment: copies a product input frame, finds the latest eligible
  `PrimaryTile` pressed event with tile payload, queries target/reach as a
  tile-center report, applies target-context enrichment to the copied frame
  binding context only, preserves events unchanged/in order, and does not
  auto-run from frame request/play surface or convert `PrimaryTile` to
  `Interact`.
- Runtime `RuntimeGameplayProductInputContext` for app-neutral product binding
  context projection, returning default gates plus current player tile only when
  product play state is loaded and has a player, without target discovery,
  mouse mapping, primary input synthesis, persistence, or pump/cadence policy.
- Runtime `RuntimeGameplayProductPresentationFrame` for projection-only product
  presentation over loaded product loop state plus caller-owned `CameraState`
  and `LevelRenderFrame2DConfig`; it returns a `LevelRenderFrame2DResult`
  without stepping gameplay or owning camera lifecycle, UI, render backend, or
  persistence.
- Runtime `RuntimeGameplayProductActorRenderCommands` for projection-only
  debug/material quad commands over current gameplay actors: player first when
  present, then present modern NPC actors in registry order, using centered 1x1
  default `material:player` / `material:npc_actor` quads on layer 20 without
  textured sprite/animation sampling, assets, state mutation, or persistence.
- Runtime `RuntimeGameplayProductPresentationCamera` for app-neutral
  presentation camera policy that chooses transient caller-owned
  `CameraState` plus `LevelRenderFrame2DConfig` from product play state and
  caller config, using existing `CameraRig` follow/clamp behavior without frame
  execution, Qt/UI, input mapping, or persistence.
- Runtime `RuntimeGameplayProductFrameRequest` for app-neutral manual product
  frame requests, composing presentation camera policy first and then one
  `RuntimeGameplayProductPlayMode::frame(...)` call with the selected transient
  camera/render config while leaving input draining, previous-camera storage,
  latest-frame storage, and presentation ownership to the caller.
- Runtime `RuntimeGameplayProductPlaySurfaceFrame` for one caller-requested,
  app-neutral product play frame, composing product input adaptation, player
  input binding, one product-loop step with per-step context override, and
  presentation projection without adding Qt/UI, raw OS event, automatic tick
  loop, or launch-mode ownership.
- Runtime `RuntimeGameplayProductPlayMode` for app-neutral durable play-mode
  state that stores only product loop state plus an input-focus bit and
  delegates one requested frame to the play-surface frame without owning Qt/UI,
  raw input, camera/presentation persistence, or app tick-loop behavior.
- Scene/UI `UiProductPlayModePanelModel` and product play feature context seam
  for read-only projection of product play build/state/latest-frame pointers
  into `feature:product_play` / `panel:product_play` rows without UI execution,
  stepping, loading, raw input, camera defaults, or state mutation.
- Qt shell `iggy_qt_shell --play PATH` launch/load/build consumer for explicit
  product scenario paths, mutually exclusive with `--preview`, which loads,
  builds loop/play-mode state, wires stable product play context pointers, and
  reveals the existing read-only product play panel without stepping frames or
  mapping raw input.
- Qt shell `Product Input Focus` View-menu toggle for ready `--play` sessions,
  updating only durable current `RuntimeGameplayProductPlayMode` focus state and
  refreshing the read-only product play panel without routing Qt input events or
  stepping frames.
- Qt shell ready/focused `--play` keyboard mapping from supported key
  press/release events into app-shell-owned transient
  `RuntimeGameplayProductInputAccumulatorState`, storing product controls/events
  only without raw Qt event persistence, adapter or binding calls, frame
  stepping, cadence policy, or camera/render ownership.
- Qt shell `Product Step` View-menu action for ready `--play` sessions,
  invoking `RuntimeGameplayProductFrameRequest` exactly once, updating only
  replaceable app-shell transient play state/latest frame/presentation camera,
  enriching the copied input frame target context through the shared helper,
  clearing transient product input after executed requests, and refreshing the
  existing read-only product play panel without settings persistence, shortcuts,
  or runtime semantic changes.
- Qt shell `Product Frame Pump` View-menu toggle for ready `--play` sessions,
  using window-owned `QTimer` timing at 250 ms / 4 Hz through the same one-frame
  helper as manual Step, including the same target-context enrichment call. It is
  app-shell-owned replaceable timing only, updates transient shell play
  state/latest frame/presentation camera/accumulator state, and does not persist
  pump settings or add runtime/product semantics.
- Runtime authoring diagnostic projection with stable printable error-code
  strings.
- Runtime summary projection for CLI/facade summary and final rows.

### CLI, Fixtures, And Packages

- `iggy_scenario_toml_runner` supports explicit TOML files and explicit package
  directories/manifests.
- CLI modes: run, `--trace`, `--check`, and `--lint`.
- CLI output contracts are locked with path-normalized snapshots.
- Canonical fixture manifest and manifest-driven sweep tests.
- Canonical fixtures cover movement, multi-frame movement, player/NPC movement,
  interaction, pickup, mixed progression, locked door/key, collision/blocked
  movement, and package equivalents.
- Negative fixture catalog covers parser/type/source/conversion/profile-style
  failures through checked-in regression assets.
- Expectations cover final rows, summary counts, trace frames, inventory stacks,
  interaction target state, actor final tile, and player final tile.
- Package boundary is implemented with `package.toml`, one main scenario TOML,
  display-only metadata, fixture pack, and one-file/package parity tests.

### Planning And Workflow

- Authoring batch bucket with numbered packets and hard stops.
- Finisher worktree/bucket exists for docs/coordination/cleanup work.
- Local Mac department orchestration is documented in
  `engine/research/local_mac_department_orchestration.md`: hub-led planning,
  local worktrees, builder/finisher/reviewer/researcher departments, apprentice
  scouts, merge gates, verification lanes, and context ticks.
- Roadmap, API index, compatibility debt notes, smell dashboard, and merge
  protocol documents exist.

## Sorta Have

- Runtime orchestration is broad and effective, but has many wrappers, counters,
  reports, and runners that can grow ceremony if not actively pruned.
- Legacy `modules/npc_ai` still coexists with newer `scene/ai` and `scene/npc`.
  It is compatibility-bearing and not safely deletable yet.
- Derived cache migration still has compatibility mirrors.
- Save/load works for runtime state, but authored package save/load roundtrip is
  not proven.
- UI exists as shell/models plus a first read-only authoring preview panel in
  the Qt shell, but not as an authoring editor or playable product shell.
- Read-only preview model is consumed by the existing Qt shell for explicit
  TOML/package paths; source-linked diagnostics, visual trace playback, play
  shell integration, and editor mutation are still gated.
- Render is command/resource/cache level, not product-grade presentation.
- Audio is effectively unbuilt.
- Physics is enough for current AABB movement/query constraints, not a broad
  simulation engine.
- Package mode is intentionally narrow: one manifest, one main scenario, no
  discovery/dependencies/assets.
- TOML remains a supported subset, not a full TOML implementation.

## Need

### Immediate Authoring Stabilization

- Keep API index current when facade, package, preview, check/lint/trace, or
  package CLI behavior changes.
- Pull the next authoring packet from `engine/research/authoring_batches/`
  unless a planner-scoped stretch overrides raw queue order.
- Use the local department workflow for substantial work: scope with
  reviewer/researcher, assign builder/finisher to isolated worktrees, and merge
  through the integration hub after focused plus integration verification.

### Remaining Authoring Work

- AI-map region fixture examples.
- Multi-actor/profile fixture coverage.
- Existing interaction effect fixture pack.
- ResourceId namespace policy.
- Frame ordering policy.
- Authoring warning-channel decision.
- Source-plan compatibility/migration gate.
- Authoring diff report.
- Product/source-plan size-limit policy gate if fixture budgets prove
  insufficient.

### Product And Runtime Work

- Runtime cleanup after authoring contracts stabilize.
- Legacy NPC migration plan with explicit deletion prerequisites.
- Product scenario loading and product-owned one-frame stepping exist for
  explicit TOML/package paths, and the scene/player normalized input binding
  layer plus runtime/product transient input adapter and per-step product loop
  input context override plus projection-only product presentation frame exist;
  the app-neutral runtime play-surface frame now composes those public surfaces
  for one caller-requested focused play frame; app-neutral play-mode state now
  stores only durable loop state and input focus; the Qt shell now maps
  supported keys to transient product input events for ready, focused `--play`
  sessions; the runtime/product camera policy now selects transient
  caller-owned camera/config for presentation; the runtime/product frame request
  wrapper now composes camera selection plus exactly one play-mode frame call for
  caller-requested manual frames; the Qt shell now exposes a ready-state manual
  `Product Step` action that executes one frame request; the shell now stores
  transient held/one-shot product input in an accumulator and enriches only
  accumulator frame output with projected current-player-tile binding context;
  the Qt shell now has replaceable app-owned `Product Frame Pump` timing for
  ready `--play` sessions; runtime/product presentation now appends
  debug/material player and modern NPC actor quads after level rendering;
  runtime/product pointer projection plus explicit primary point/tile
  accumulator event preservation exists; Qt shell now owns a
  product-play viewport frame as an event/render target boundary and has a
  focused ready-play left-click `PrimaryTile` consumer for that viewport; the
  runtime/product interaction target query now reports point/tile-center target
  lookup plus reach annotation read-only, and
  `RuntimeGameplayProductInputTargetContext` can enrich a copied transient
  binding context with a hovered target id from that report. The opt-in
  `RuntimeGameplayProductInputFrameTargetContext` helper can now consume a frame's
  latest eligible `PrimaryTile` pressed event, run that query/enrichment, and
  return a copied frame with only binding context replaced. Qt manual Step and
  frame pump now call that helper through their shared one-frame path, the Qt
  product viewport can draw the latest frame's existing quad render commands as
  a temporary debug-material presentation consumer, and the viewport can overlay
  a thin Qt-only target highlight from latest target-context diagnostics. Next
  product runtime work is deciding whether frame request/play surface should own
  enrichment, whether richer overlays/labels or diagnostics should be surfaced,
  or whether explicit interaction intent should be synthesized, then interaction
  execution if approved, real renderer ownership, textured sprite/animation/
  material/asset policy, pause/retry/reset policy, completion/failure
  evaluation, and save/load UX.
- Rendering backend/presentation layer.
- Audio server boundary.
- Save/load productization and authored package roundtrip.
- Source-linked diagnostics and richer visual trace playback for the read-only
  preview panel.
- Product/play shell integration after the product-loop gate.
- Editor mutation only after explicit authoring roundtrip/source mutation gates.
- Gameplay semantics beyond the current set: inspect/talk/event gates, region
  triggers, equipment, combat, progression, win/fail conditions.
- Performance and scale policy for authored content and runtime hot paths.

## Avoid / Defer

- Do not turn the read-only preview consumer into UI/Edi mutation without an
  explicit source roundtrip/mutation gate.
- Do not add a generic scripting/event language to TOML.
- Do not add JSON or a TOML dependency unless a packet explicitly opens that
  policy.
- Do not delete `modules/npc_ai` until compatibility users and tests have a
  migration path.
- Do not expand runtime wrapper/report families without a cleanup gate.
- Do not persist derived caches as save truth.
- Do not add broad ECS/entity registry work yet.
- Do not build full physics simulation before gameplay needs exceed current
  movement/query constraints.
- Do not add combat/dialogue/quest systems before product-loop and authoring
  boundaries stabilize.
- Do not make package mode a content management system: no recursive discovery,
  dependencies, asset registry, or multi-scenario package semantics yet.

## Roadmap Phases

### 1. Authoring V1 Closure

Status: complete for engine/test/dev-tool use.

Objective: make TOML single-file and package authoring stable,
self-documenting, and self-checking.

Done:
- Facade-backed run/lint/check/trace.
- Stable CLI output contracts.
- Diagnostic and summary projections.
- Canonical fixture manifest and manifest sweep.
- Final-row, trace, inventory, interaction, actor, and player expectations.
- Negative fixture catalog.
- Source-plan version/schema/subset hardening.
- Package runner/metadata/parity.
- Read-only preview model.
- Release-candidate acceptance set.
- Deterministic fixture-scale budget checks.
- Runtime authoring cleanup gate.
- Batch queue prune gate.

Remaining adjacent work:
- AI-map region fixture examples.
- Multi-actor/profile fixture coverage.
- Existing interaction effect fixture pack.
- ResourceId namespace and frame-ordering policy.

### 2. Runtime/API Cleanup

Status: cleanup gate complete; implementation is not started.

Objective: reduce bloat after authoring contracts stabilized.

Exit criteria:
- Duplicated wrapper/report/helper patterns are consolidated or marked
  intentional.
- Count/report projection growth is contained.
- Legacy debt has explicit removal conditions.
- No gameplay behavior changes are introduced as cleanup.

Candidate packets:
- `59_authoring_run_summary_projection` is complete and should be treated as the
  first successful cleanup pattern.
- A focused authoring test/support deduplication packet from Batch 25 evidence.
- A focused counter/report projection cleanup packet from smell-audit evidence.
- A focused CMake/test registration hygiene packet if useful.

Hard stops:
- Do not delete compatibility-bearing `modules/npc_ai`.
- Do not alter save/load chunks.
- Do not change CLI output contracts as cleanup.

### 3. Editor Readiness

Status: first read-only Qt shell preview consumer integrated; later UI/editor
work remains gated.

Objective: let tools preview authored TOML/package scenarios without mutation.

Done:
- `15_editor_handoff_gate`
- `19_editor_preview_model_gate`
- `26_minimal_editor_model`
- `23_fixtures_from_cli_examples`
- UI workspace profile notes are recorded in
  `engine/research/ui_integration_profiles.md`.
- Existing Qt shell accepts explicit `--preview PATH` plus
  `--preview-mode run|trace|check|lint` and renders a read-only
  `panel:authoring_preview`.
- `UiAuthoringPreviewPanelModel` projects
  `RuntimeGameplayAuthoringPreviewModel` into UI rows/sections without UI-owned
  parsing or mutation.
- `UiProductPlayModePanelModel` projects provided product play build/state/latest
  frame pointers into read-only rows for `feature:product_play` /
  `panel:product_play`; the panel is hidden by default and exists only when
  product play context is supplied.
- Existing Qt shell accepts explicit `--play PATH`, runs product load/loop
  build/play-mode build only, stores stable product play context pointers, and
  reveals the existing read-only `panel:product_play`.
- `--play` is mutually exclusive with `--preview`; missing `--play` path and
  combined play/preview usage exit with code 2.
- In `--play` sessions, the View menu exposes a checkable `Product Input Focus`
  action when product play is ready. Toggling it updates the durable current
  play-mode focus bit and refreshes `panel:product_play`; failed-load play
  sessions keep the action disabled/non-applicable.
- Ready, focused `--play` sessions map supported Qt key press/release events to
  transient product accumulator state in `IggyQtShellWindow`: Arrow/WASD held
  cardinal movement, `E`/Return/Enter interact, `I` inspect, Space wait, and
  Escape cancel. Focus disable clears the accumulator; auto-repeat and
  unsupported keys are ignored; raw `QKeyEvent` objects or pointers are not
  stored.
- `RuntimeGameplayProductPresentationCamera` chooses caller-owned transient
  camera and render config for product presentation. It handles not-loaded
  previous/fallback camera selection, loaded player initialization,
  previous-camera player follow through `CameraRig`, follow-disabled
  previous/fallback behavior, clamp result flags, and render config forwarding
  for view/camera config, NPC command rendering, tile chunk cache, and cache
  pointer.
- `RuntimeGameplayProductFrameRequest` builds presentation camera policy first,
  calls `RuntimeGameplayProductPlayMode::frame(...)` exactly once with the
  selected transient camera/render config, maps play-mode frame status to
  request status, carries the nested next play-mode state, projects supplied
  input event count plus nested ignored input count, and preserves nested camera
  and play-mode frame results without inventing flattened fields.
- `RuntimeGameplayProductInputContext` projects transient
  `PlayerInputBindingContext2D` for product play input. Statuses are
  `NotLoaded`, `LoadedWithoutPlayer`, and `Projected`; projected contexts keep
  default gates and set only `hasCurrentPlayerTile/currentPlayerTile` from the
  loaded state's existing `playerTile(...)` when a current player exists.
- `RuntimeGameplayProductInputAccumulator` stores product-level transient input
  state only: held `MoveNorth`/`MoveSouth`/`MoveWest`/`MoveEast` controls plus
  pending one-shot `Interact`/`Inspect`/`Wait`/`Cancel` events. Held movement
  press adds a control, duplicate held press is suppressed, release removes held
  movement, and release of non-held movement is a no-op. One-shot press queues
  one event and one-shot release is a no-op. Frame output emits ordinary
  `Pressed` events for held movement each requested frame followed by pending
  one-shots in press order, carries supplied binding context, preserves held
  controls, drains one-shots in returned state, and `clear(...)` returns empty
  transient state.
- Qt shell `Product Step` is enabled only for ready `--play` sessions and is
  independent of `Product Input Focus`: when focus is false, existing
  play-surface behavior ignores input but still consumes one available frame
  with empty intents/context. Executed steps build accumulator frame output with
  projected binding context, store the returned accumulator state before the
  frame request, build `RuntimeGameplayProductFrameRequestInput` from current
  app-shell play state, accumulator output, and shell-owned presentation camera
  config, call
  `RuntimeGameplayProductFrameRequest {}.run(input)` once, replace only
  transient app-shell `productPlayState_`, latest frame/context pointer, and
  presentation camera, preserve held movement across steps, drain one-shots
  after executed steps, and refresh the product play panel.
- Qt shell `Product Frame Pump` is a checkable View-menu action for ready
  `--play` sessions. It is Qt/app-shell-owned replaceable timing only, driven by
  a window-owned `QTimer` at 250 ms / 4 Hz. Manual Step and timer ticks share the
  same one-frame helper: accumulator output plus projected input context, stored
  returned accumulator state, one `RuntimeGameplayProductFrameRequest` call,
  transient play-state/latest-frame/previous-camera/context-pointer update, and
  product play panel refresh. Pump availability is ready product play only,
  independent of `Product Input Focus`; the timer stops if product play becomes
  unavailable or not ready, and it does not auto-stop on `NoFrameAvailable`.

Remaining exit work:
- Add source-linked diagnostics and richer trace/expectation inspection.
- Decide when the product shell should move beyond the temporary Qt-owned frame
  pump into an owned app loop, and when it supplies any input mapping beyond the
  supported keyboard controls.
- Gate any build canvas, structured authoring controls, or source/TOML
  roundtrip separately.
- Keep editing/mutation APIs out until a separate gate approves them.

Likely next packets:
- A source-linked diagnostics projection/display packet.
- A visual trace playback packet.
- A product/play shell boundary packet.
- A future editor mutation/roundtrip gate only after read-only UI remains stable.

### 4. Package And Content Pipeline

Status: narrow package boundary complete.

Objective: support explicit authored scenario packages without becoming a package
manager.

Done:
- Layout proposal.
- Package runner gate.
- Package runner implementation.
- Metadata manifest.
- Package fixture pack.
- File/package parity.

Remaining exit work:
- Package save/load roundtrip decision.
- Package examples/docs polish.
- Package compatibility/version policy only if package content evolves.

Likely next packets:
- `12_authored_scenario_save_load_roundtrip` only after a narrow gate.
- `45_authoring_compatibility_migration_gate`.
- Package-to-product-shell loader gate later.

### 5. Gameplay Semantics V1

Status: one narrow semantic landed; remaining semantics need gates.

Objective: add small, fixture-backed gameplay semantics without creating a
scripting system.

Done:
- Locked door/key through required-item interaction gating.

Candidate gates:
- `41_emit_event_semantics_gate`
- `42_talk_interaction_semantics_gate`
- `43_region_trigger_semantics_gate`
- `53_terrain_authoring_policy_gate`

Hard stops:
- No generic condition language.
- No event bus framework unless explicitly approved.
- No dialogue trees, quests, or combat until product-loop boundaries exist.

### 6. Product Loop

Status: Packets 1, 2, 3A, 3B-A, 4-A, 4-B, play-surface frame, and play-mode
state complete; product runtime has a load boundary, caller-driven one-frame
step, optional per-step input context override, scene/player normalized input
binding, a runtime/product input adapter for transient product input events, a
projection-only presentation wrapper over loaded loop state plus caller-owned
camera/config, an app-neutral one-frame play-surface composition facade, and
durable app-neutral play-mode state storing only loop state plus focus. Product
play UI projection is read-only and context-provided, and Qt `--play PATH`
launch/load/build context wiring, a ready-state focus toggle, and ready/focused
keyboard-to-product-input-event mapping exist. The runtime/product presentation
camera policy chooses caller-owned transient camera/config for presentation, and
`RuntimeGameplayProductFrameRequest` composes that policy with exactly one
play-mode frame call for caller-requested manual frames. Qt `Product Step`
invokes one frame request for ready `--play` sessions using accumulator frame
output enriched with projected current-player-tile binding context before
updating replaceable app-shell transient state. Qt `Product Frame Pump` adds
app-shell-owned 250 ms / 4 Hz timing around that same one-frame helper for ready
`--play` sessions. Product Pointer Tile Input Mapping Option A is complete as
runtime/product projection and accumulator event preservation, and the Qt shell
now records focused ready left-clicks on `productViewport_` as transient
`PrimaryTile` pressed events. Runtime/product target lookup now exists as a
read-only report surface over point or tile-center queries with optional reach
annotation, and Qt manual Step/frame pump now apply opt-in pre-frame target
context enrichment before frame requests. There is still no durable hover or
selection state, `PrimaryTile` to `Interact` conversion, interaction execution,
right/middle/move/wheel/
double-click/drag behavior, textured sprite/animation/material/asset policy, UX
policy, or save/load productization yet.
Qt Product Viewport Owner is complete as a temporary app-shell viewport/canvas
boundary: product play sessions get `QFrame#productViewport`; the current input
policy is left-click `PrimaryTile` only. Thin Qt Product Viewport Render Command
Drawer is complete as a temporary app-shell presentation consumer: the viewport
draws existing latest-frame quad render commands with Qt-local debug colors,
without becoming renderer ownership, canvas polish, texture sampling, or gameplay
truth.
Thin Qt Product Target Highlight Overlay is complete as a Qt-only visual
consumer: the viewport draws an outline/tint marker after render-command quads
when latest transient target-context diagnostics contain a copied target. It
uses copied target position/radius, marks reachable/unreachable/no-reach with
local styles, and does not run target queries, synthesize input, select targets,
execute interactions, persist highlight state, or change runtime APIs.
InteractionTargetSpatialQuery2D is complete as a scene-only lookup primitive:
it scans enabled interaction targets in registry order, compares Euclidean
distance to clamped target radius plus clamped extra radius, returns the nearest
eligible target with registry-order tie behavior, supports tile-center queries,
and keeps Qt/product wiring, selected/hovered target context, reach policy, and
interaction execution separate.
RuntimeGameplayProductInteractionTargetQuery is complete as an app-neutral
runtime/product read-only report surface: it maps point or tile-center requests
against current product play state targets through the spatial query primitive,
copies found target payloads into the report, annotates reach only when a player
exists, and keeps selected/hovered state, input conversion, command/effect
execution, Qt behavior, and persistence separate.
RuntimeGameplayProductInputTargetContext is complete as a runtime/product
projection helper: it starts from a copied base binding context and, only for a
valid `TargetFound` query with a non-empty target id, replaces hovered target
fields and reports `TargetProjected`. Non-found/invalid paths return
`Unchanged` while preserving base selected target, hover, current player tile,
and input gates; reach remains report annotation and no lifecycle, execution,
or persistence policy is added.
RuntimeGameplayProductInputFrameTargetContext is complete as an opt-in
runtime/product pre-frame helper: it copies the input frame, finds the latest
eligible `PrimaryTile` pressed event with a tile payload, runs the product target
query as a tile-center lookup, applies target-context enrichment to the copied
frame binding context, preserves all events unchanged, and returns diagnostics.
It does not auto-wire frame request/play surface, synthesize `Interact`/`Inspect`
targets, inject target ids into events, persist hover, or change adapter/command
semantics.
Qt Product Input Frame Target Context Consumer is complete as a thin caller and
diagnostics projector in `IggyQtShellWindow::runProductFrameRequestOnce()`: after
accumulator `buildFrame(...)` and storing the returned accumulator state, but
before `RuntimeGameplayProductFrameRequest`, Qt calls
`RuntimeGameplayProductInputFrameTargetContext {}.enrich(...)` with
`productPlayState_`, the built frame, and default spatial/reach configs. It
stores the latest transient helper result in replaceable app-shell state, wires
the stable diagnostics pointer into `UiFeatureContext`, passes the stored copied/
enriched frame into the request, clears diagnostics when Product Input Focus is
disabled, and leaves focus gating, mouse/key mapping, pump timing, manual Step
availability, accumulator drain, and runtime helper semantics unchanged.

Objective: move from engine harness to playable/editor-backed game loop.

Done:
- `RuntimeGameplayProductScenarioLoader` loads explicit TOML files, package
  directories, and `package.toml` paths into validated profile scenario
  definitions plus initial state.
- Loader is load-only: no frame execution, check/trace/final rows, raw input,
  presentation/camera, save/load productization, or UI behavior.
- Package manifest/path parsing is shared through
  `RuntimeGameplayTomlScenarioPackageReader`.
- `RuntimeGameplayProductLoop` builds product loop state from a successful load,
  starts at `nextFrameIndex = 0`, steps exactly one lowered scenario frame per
  call, replaces authored/scripted player frame intents with caller-provided
  `PlayerInputIntent2D`, runs `RuntimeGameplayOrchestratedFrameStep`, carries
  current state forward, and reports failed-load/not-loaded/exhausted-frame
  guard statuses.
- `RuntimeGameplayProductLoopStepInput` can carry an optional per-step
  `PlayerInputContext2D` override; unset steps preserve the authored/lowered
  frame context, and set overrides flow into the existing lower-level
  frame-step/gate path without persisting context as gameplay or save truth.
- `PlayerInputBinding2D` maps device-agnostic normalized actions into
  `PlayerInputIntent2D`, carrying `PlayerInputContext2D` as data, preserving
  action order, reporting stable counts/issues, and excluding no-op/issues from
  emitted intents.
- `RuntimeGameplayProductInputAdapter` maps transient product input events into
  `PlayerInputBindingAction2D` actions plus carried `PlayerInputBindingContext2D`,
  preserving order and input immutability while reporting stable counts/issues
  for release/no-op policy, movement controls, target fallback handoff, payload
  validation, and unsupported controls.
- `RuntimeGameplayProductPresentationFrame` projects a loaded product loop
  state's `currentState.session.level` through existing
  `LevelRenderFrame2D::build(...)` using caller-owned `CameraState` and
  `LevelRenderFrame2DConfig`, then appends product actor debug/material quads
  after level render commands using `RenderCommandList2DComposer::append(...)`;
  unloaded state returns `NotLoaded`, echoes the camera, leaves the level frame
  default, and emits no actor commands.
- `RuntimeGameplayProductActorRenderCommands` projects current gameplay actors
  into untextured debug/material quad render commands without mutating player or
  NPC state. It emits the player first when `state.session.hasPlayer` is true,
  then present `state.npcActors.actors` entries in registry order, using
  configurable include flags, material IDs, size, anchor, and layer defaults
  (`material:player`, `material:npc_actor`, centered 1x1 quads, layer 20).
  Bounds follow the legacy NPC convention: negative sizes normalize, positions
  subtract size times anchor, and zero-size degenerate bounds are allowed.
- `RuntimeGameplayProductPlaySurfaceFrame` composes
  `RuntimeGameplayProductInputAdapter`, `PlayerInputBinding2D`,
  `RuntimeGameplayProductLoop`, and `RuntimeGameplayProductPresentationFrame`
  for one caller-requested play frame: product input events become binding
  actions, then player intents/context, then exactly one product-loop step, then
  presentation projection from the returned step state. Not-loaded and exhausted
  states skip input adaptation/binding/step and count transient input events as
  ignored; unfocused frames ignore transient events before adapter semantics,
  step once with empty intents/context override, and present the post-step state.
- `RuntimeGameplayProductPlayMode` owns durable state only as
  `RuntimeGameplayProductLoopState loop` plus `hasInputFocus`; builds mirror
  loop build status and copy ready loop state, focus defaults true, focus
  toggles preserve loop state, and `frame(...)` delegates exactly once to
  `RuntimeGameplayProductPlaySurfaceFrame` with transient input/camera/config,
  carrying stepped loop state forward only on `Stepped`.
- `UiProductPlayModePanelModel` is a read-only scene/UI projection over provided
  product play build/state/latest-frame pointers, showing build/loop status,
  identity paths, loaded/focus/frame facts, latest frame status, ignored input,
  adapter/binding/step/presentation counts, render counts, and compact
  target-context diagnostics rows when a latest diagnostics pointer is provided,
  without calling loaders, frame stepping, product run APIs, or mutating
  product/runtime state.
- `iggy_qt_shell --play PATH` is an explicit-path Qt launch consumer that runs
  `RuntimeGameplayProductScenarioLoader::load(path)`,
  `RuntimeGameplayProductLoop::build(load)`, and
  `RuntimeGameplayProductPlayMode::build(loopBuild)`, stores the load/loop/play
  build results plus play-mode state in `IggyQtShellWindow`, wires product play
  context pointers, reveals `panel:product_play`, and leaves latest frame null.
  Bad paths still open the shell and show failed load/build state; `--play` and
  `--preview` are mutually exclusive with exit code 2.
- Qt shell builds `QFrame#productViewport` in the main slot whenever product
  play mode exists, including failed/not-ready play sessions. It uses
  zero-margin/zero-spacing layout, expanding size policy, and local viewport
  stylesheet only; `productViewport_` is reset to null on rebuilds where no
  viewport is created. The viewport is a stable event/render target boundary,
  not input readiness or runtime truth.
- Qt shell `QFrame#productViewport` now uses a Qt-local paint path to draw
  existing latest product play frame render commands from
  `latestProductPlayModeFrame_.surface.presentation.levelFrame.commands.commands`
  using `latestProductPlayModeFrame_.surface.presentation.levelFrame.cameraView.bounds`
  as visible world bounds. It maps command world bounds to widget pixel
  rectangles, normalizes world bounds and mapped `QRectF` defensively, keeps
  x/y orientation direct with no flip, skips commands when no latest frame,
  non-positive widget size, or zero camera-view axes are present, iterates the
  command vector without Qt-side sorting, draws only `RenderCommand2DType::Quad`
  as untextured flat rectangles, ignores texture payloads, and uses Qt-local
  hardcoded debug colors for floor, wall, player, NPC actor, legacy NPC, and
  fallback materials.
- Qt shell installs a viewport-only event filter for product play sessions.
  Focused, ready left mouse press on `productViewport_` records one transient
  `RuntimeGameplayProductInputEvent2D` with `control = PrimaryTile`, `kind =
  Pressed`, `hasTile = true`, and `tile = projection.tile` in the product input
  accumulator. The handler accepts/returns true only when that product event is
  recorded; otherwise it falls through.
- Qt shell `Product Input Focus` toggles are enabled/applicable only for ready
  product play state. The action updates only current `productPlayState_` via
  `RuntimeGameplayProductPlayMode {}.withInputFocus(...)`, keeps product play
  context pointers stable, clears latest frame to null, and refreshes the
  read-only product play panel so its `hasInputFocus` row changes.
- Qt shell keyboard mapping records product input controls into an
  app-shell-owned transient `RuntimeGameplayProductInputAccumulatorState` when
  product play exists, play-mode build is ready, and product input focus is
  enabled. Disabling focus clears the accumulator. Auto-repeat and unsupported
  keys are ignored. Arrow/WASD map to held cardinal movement controls;
  `E`/Return/Enter, `I`, Space, and Escape queue one-shot interact, inspect,
  wait, and cancel events. No raw `QKeyEvent` persistence, adapter call, binding
  call, frame step, camera/render config, UI model exposure, or settings
  exposure is added.
- `RuntimeGameplayProductInputAccumulator` is a shell-neutral return-by-value
  transient input helper. Held movement controls are exactly
  `MoveNorth`/`MoveSouth`/`MoveWest`/`MoveEast`; one-shot controls are exactly
  `Interact`/`Inspect`/`Wait`/`Cancel`. Held movement emits ordinary `Pressed`
  events each frame in held press order before pending one-shot press order,
  preserves held controls, drains one-shots in returned state, carries supplied
  binding context, and preserves explicit pressed `PrimaryPoint` and
  `PrimaryTile` events with their payloads as pending one-shots. Primary
  releases are no-ops; the accumulator still does not synthesize point/tile
  events.
- `RuntimeGameplayProductPointerProjection` maps viewport-local points through
  existing `CameraView` normalization to a world point plus `tileForPoint(...)`
  tile using only `CameraState`, `CameraView`, `CameraViewConfig`, and
  `TileCoord`. It preserves `CameraView` behavior for negative viewport
  dimensions, non-positive zoom, and zero-axis degenerate viewports, and returns
  projection status/flags/world point/tile for a later consumer to choose
  `PrimaryPoint`, `PrimaryTile`, or both under a separate policy gate.
- `RuntimeGameplayProductPresentationCamera` is an app-neutral runtime/product
  camera policy. It chooses transient caller-owned `CameraState` plus
  `LevelRenderFrame2DConfig` from product play state and caller-owned config,
  supports not-loaded previous/fallback selection, loaded-player
  initialization, previous-camera player follow through existing `CameraRig`,
  follow-disabled previous/fallback behavior, clamp result flags, and render
  config forwarding. It does not execute frames, call play-surface build, call
  product input adapter/binding, persist presentation state, or add Qt/UI/CLI
  behavior.
- `RuntimeGameplayProductFrameRequest` is an app-neutral runtime/product manual
  frame request wrapper. It computes `RuntimeGameplayProductPresentationCamera`
  first, then calls `RuntimeGameplayProductPlayMode::frame(...)` exactly once
  with the selected transient camera/render config. It maps nested frame status
  to request status, returns the carried next play-mode state, exposes supplied
  input event count and nested ignored input count, and preserves nested camera
  and play-mode frame results. The caller still owns transient input-frame
  clearing/draining, previous-camera storage, latest-frame storage, and
  presentation state ownership.
- `RuntimeGameplayProductInputContext` is an app-neutral runtime/product
  projection for transient request binding context. It reports `NotLoaded`,
  `LoadedWithoutPlayer`, or `Projected`, returns default
  `PlayerInputBindingContext2D` gates, and sets only current player tile from
  loaded product play state using existing `playerTile(...)` when a player
  exists. It does not set selected or hovered targets, inspect interaction
  state, search targets, map mouse input, synthesize `PrimaryPoint` or
  `PrimaryTile`, add cadence/pump behavior, or persist binding context.
- Qt shell `Product Step` is a ready-state View-menu action for `--play`
  sessions. It builds accumulator frame output with projected binding context,
  stores the returned accumulator state, enriches a copied frame through
  `RuntimeGameplayProductInputFrameTargetContext`, stores the latest transient
  target-context diagnostics for read-only projection, builds request input from
  current `productPlayState_`, the enriched frame, and shell-owned camera config,
  runs `RuntimeGameplayProductFrameRequest` once, replaces only
  `productPlayState_`, `latestProductPlayModeFrame_`, the stable product play
  context pointer, and `productPresentationCamera_`, preserves held movement
  across steps, drains one-shots after executed steps, and refreshes the
  read-only product play panel. If the action is unavailable, accumulator state
  is not silently cleared. Shell camera defaults are presentation-only
  fallback/view defaults with NPC commands enabled and tile chunk cache
  disabled; they are not settings/save truth. The projected binding context is
  request-input only and is not persisted in the accumulator state.
- Qt shell `Product Frame Pump` is a ready-state View-menu toggle for `--play`
  sessions. It owns only replaceable Qt timing/action checked state via a
  window-owned `QTimer` at 250 ms / 4 Hz, shares the same one-frame helper as
  manual Step including input-frame target-context enrichment, and updates only
  transient app-shell product play state, input accumulator state, latest frame,
  previous presentation camera, product play state context pointer, and panel
  projection. It stops if product play becomes unavailable/not ready, does not
  stop on `NoFrameAvailable`, and persists no pump enabled state, interval,
  keybinding, input, camera, latest-frame, or render-frame data.
- Product actor render projection is runtime-only and projection-only:
  `RuntimeGameplayProductActorRenderCommands` emits debug/material quads for
  the current player and present modern NPC actors, and
  `RuntimeGameplayProductPresentationFrame` appends those commands after level
  render commands. Qt product play/manual Step/pump receive the commands only
  through the existing frame request/latest-frame path and do not own render
  semantics.
- Product pointer projection is runtime/product and Qt-free:
  `RuntimeGameplayProductPointerProjection` provides the camera/view math needed
  for Qt to emit transient `PrimaryTile` events into the accumulator. The
  corrected Qt handler normalizes `QMouseEvent::position()` by
  `productViewport_` pixel width/height into the configured camera-view span
  from `productPresentationCameraConfig().cameraView.viewportSize` before
  calling the projection helper, and builds presentation camera policy
  read-only from `productPlayState_`. Point-vs-tile / `PrimaryPoint` behavior,
  target context wiring/lifecycle/reach beyond the enrichment helper, and
  interaction execution remain separate gates.
- Interaction target spatial query is scene-only and pure:
  `InteractionTargetSpatialQuery2D` finds the nearest enabled target in range of
  a point or tile center, including zero-radius same-position hits and exact
  boundary matches, while preserving first registry entry on exact distance
  ties. It does not project hovered/selected context, check reach, execute
  interactions, or wire Qt/product input.
- Product interaction target query is runtime/product and read-only:
  `RuntimeGameplayProductInteractionTargetQuery` accepts product play state plus
  a point or tile-center query, preserves nested spatial result details, copies
  the found target, and annotates reach through `InteractionReach2D` only when
  the state has a player. It does not use `InteractionPlan2D`, mutate target
  context, synthesize input, or execute interactions.
- Product input target context projection is runtime/product and enrichment-only:
  `RuntimeGameplayProductInputTargetContext` copies a base binding context and
  projects a hovered target id only from a valid found query result. It preserves
  selected target fields and existing hover on unchanged paths, does not require
  reachability, and does not change `RuntimeGameplayProductInputContext`,
  adapters, frame requests, play-surface behavior, or command/effect execution.
- Product input frame target context is runtime/product and opt-in:
  `RuntimeGameplayProductInputFrameTargetContext` scans a copied frame for the
  latest eligible primary-tile press, forwards spatial/reach configs to the
  query helper, applies target-context enrichment to the frame binding context,
  and keeps event order/payloads intact. Missing-tile and release primary events
  stay ineligible for this helper and preserved for existing adapter behavior.
- Qt product input frame target context consumer is app-shell thin:
  manual Step and frame pump both call the opt-in helper through
  `runProductFrameRequestOnce()`, pass the enriched frame to the frame request,
  and store only the latest transient helper result for read-only compact panel
  projection. Qt does not expose full copied frames, all events, full target
  payloads, or nested structs, and diagnostics alone do not create product play
  context.
- Qt product viewport render command drawer is app-shell thin:
  `QFrame#productViewport` paints only existing latest-frame render commands as
  temporary untextured debug/material rectangles. Paint events do not execute
  frames, sort commands, load textures, create render commands, change input
  behavior, add target highlighting from the drawer itself, or persist camera/
  presentation/viewport state as runtime or save truth.
- Qt product target highlight overlay is app-shell thin:
  `ProductViewportWidget` receives the latest target-context diagnostics pointer,
  paints command quads first, then paints a target marker only when latest frame,
  diagnostics target, non-degenerate camera view, and positive widget size exist.
  Paint uses copied target payload position/radius and reach annotation only; it
  runs no target query and adds no label, target id text, selected marker, trail,
  click animation, command execution, or richer overlay.

Exit criteria:
- Load a package or explicit scenario.
- Bind device input to player intents.
- Run gameplay frames.
- Produce render frames.
- Present frames in an app shell.
- Save/load user-facing state.

First gates:
- Richer overlays/labels or diagnostics beyond the compact panel rows and thin
  target highlight marker.
- Frame request/play-surface ownership decision for target-context enrichment.
- Hover lifecycle and selected-target workflows beyond the helper's one-shot
  enrichment.
- Explicit interact target synthesis and reach-gated interaction execution.
- Point-vs-tile / `PrimaryPoint` behavior beyond the current `PrimaryTile`
  policy.
- Real renderer ownership, textured sprite/animation/material/asset policy over
  the debug/material actor quads, and any canvas polish.
- Pause/retry/reset policy.
- Completion/failure evaluator.
- Save-slot UX ownership.

### 7. Systems Expansion

Status: deferred.

Objective: build missing game systems only after the product loop is real.

Candidate gates:
- Audio server boundary.
- Equipment/inventory expansion.
- Combat/damage.
- Progression/win/fail conditions.
- Encounter AI budget.
- UI menus.

## Near-Term Recommended Order

1. Return to the bucket raw next packet, `11_ai_map_region_fixtures`, unless a
   planner-scoped stretch overrides raw order.
2. Keep no-new-semantics fixture/policy packets ahead of new gameplay semantics
   unless the planner explicitly opens a semantics gate.
3. Use Batch 25 evidence to plan any behavior-preserving authoring test/support
   cleanup before broad runtime wrapper work.
4. Gate save/load roundtrip only if a later productization stretch explicitly
   requires persistence proof.

## Open Decisions

- Which remaining fixture/policy packet should follow Authoring V1 closure:
  AI-map regions, multi-actor/profile coverage, existing interaction-effect
  fixtures, ResourceId policy, or frame-ordering policy?
- Is the first UI milestone a read-only Qt shell panel, a separate editor
  prototype, or no UI until product loop?
- Do save/load roundtrip tests belong in Authoring v1 or later productization?
- When does legacy `modules/npc_ai` become active migration work instead of
  compatibility debt?
- How much runtime wrapper cleanup should happen before new gameplay semantics?
- Which semantic gate is highest priority: emit event, talk, region trigger, or
  terrain policy?
- Is TOML the long-term hand-authored format or the v1/source-fixture format?
- What authored content size should v1 support?
- What does "near finished" mean for this project: engine, tooling, playable
  slice, or editor-backed game?

## Update Policy

- Update this file after project-shaping batches, not after every small slice.
- Keep short-term implementation details in `engine/research/authoring_batches/`.
- Keep API facts in `engine/research/api_index.md`.
- Keep compatibility-removal prerequisites in
  `engine/research/compatibility_debt.md`.
