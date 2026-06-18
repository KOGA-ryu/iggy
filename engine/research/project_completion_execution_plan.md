# Iggy Project Completion Execution Plan

This plan converts `project_completion_todo.md` into an execution system. The
first finish line is a playable scenario: authored content loads through the
engine tooling, runs in a product loop, renders through a UI shell, and supports
the minimal save/load state needed for that slice. Editor-backed authoring comes
after that.

## Finish Line

Primary target: playable scenario first, editor-backed game second.

Playable scenario means:
- an explicit authored package is the content unit;
- package read/lint/check errors are visible before play;
- runtime can load the package into a product loop;
- player input can drive existing player intents;
- gameplay frames advance under caller control;
- render/presentation state can show the world, player, NPCs, items,
  interactions, and debug overlays when enabled;
- the slice can save/load the state needed to resume;
- tests prove the path without relying on manual UI use.

Editor-backed game means:
- the UI consumes the read-only preview model first;
- authoring mutation is gated later;
- far-left profiles can expose Play, Build, Script, Check, Actors, Items,
  Interactions, Package, Debug, and Docs/Examples without making UI own runtime
  truth.

## Execution Model

Use the old thread/packet workflow.

Roles:
- Head planner: owns this plan, roadmaps, phase order, worker dispatch, and
  integration decisions.
- Researcher: read-only scans, ownership maps, compute/cost notes, prior art,
  and risk surfaces.
- Reviewer: gates semantics, ownership, file boundaries, verification, and
  merge fit.
- Builder: implements bounded packets after planner/reviewer scope.
- Finisher: behavior-preserving cleanup, docs, test-support extraction, and
  bloat reduction.

Default chain:
```text
head planner
  -> researcher/reviewer scout if risk is unclear
  -> bounded packet
  -> builder or finisher
  -> focused verification
  -> reviewer/integration gate if needed
  -> merge/full verification
  -> roadmap sync
```

Do not restart native Terminal/tmux department automation as part of this plan.

## Phase 0: Baseline And Scout Prep

Status: ready.

Goal: make the next implementation packet evidence-backed.

Packets:
1. Runtime cleanup scout.
2. Legacy NPC compatibility map.
3. UI preview consumer gate. Complete for Milestone 1; later UI milestones
   remain gated.
4. Authoring maintenance status sweep.

Verification:
- Docs/scouts: `git diff --check`.
- If no code changes, no CMake/CTest required.

Exit criteria:
- One finisher-ready runtime cleanup packet exists.
- One legacy NPC migration map exists.
- UI preview Milestone 1 is integrated, and later UI milestones remain gated.
- Authoring bucket status does not conflict with roadmap.

## Phase 1: Runtime Cleanup And Debt Containment

Status: starts after Phase 0 scout.

Goal: reduce bloat without behavior changes before product-loop work builds on
runtime surfaces.

Likely packets:
1. Report/count duplication cleanup:
   - extract or consolidate one literal copy/projection path;
   - preserve public fields;
   - add parity tests.
2. Authoring test-support trim:
   - remove redundant lower-level ceremony only where manifest/CLI coverage
     owns equivalent behavior;
   - keep diagnostics/override tests.
3. CMake/test registration hygiene:
   - group or clarify tests only after active test additions slow down.

Candidate files to scout:
- `engine/src/runtime/RuntimeGameplayScenarioRunner.*`
- `engine/src/runtime/RuntimeGameplayScenarioLedger.*`
- `engine/src/runtime/RuntimeGameplayFrameReport.*`
- `engine/src/runtime/RuntimeGameplayFrameRunner.*`
- `engine/src/runtime/RuntimeGameplayOrchestratedFrame*`
- `engine/src/runtime/RuntimeNpcOrchestrationAggregates.*`
- `engine/tests/support/*`
- `engine/cmake/iggy_runtime_tests.cmake`

Hard stops:
- no save/load format changes;
- no CLI output changes unless output-contract tests are updated intentionally;
- no runtime behavior changes;
- no deletion of `modules/npc_ai`.

Focused verification:
- affected runtime scenario/frame/orchestrated tests;
- summary/facade/CLI output-contract tests if projection is touched;
- `git diff --check`.

Batch-end verification:
```sh
cmake -S engine -B engine/build
cmake --build engine/build
ctest --test-dir engine/build --output-on-failure
git diff --check
```

## Phase 2: Legacy NPC Compatibility Plan

Status: scout first, implementation later.

Goal: identify exactly what blocks old NPC module removal and remove only
accidental dependencies.

Packets:
1. Compatibility map:
   - group all `npc_ai::NpcAgentEntry`, `NpcAgentTickConfig`, and
     `modules/npc_ai` uses by ownership.
2. Wrong-direction dependency cleanup:
   - choose one non-save/session dependency that can move to scene/runtime
     ownership.
3. Deletion prerequisite list:
   - define what must migrate before each old module file can disappear.

Likely blockers:
- `LevelRuntimeState::npcAgents`;
- runtime tick/session flow;
- session snapshot chunk codec;
- save/load tests;
- render projection compatibility;
- old module tests.

Hard stops:
- no deletion of `modules/npc_ai`;
- no snapshot/save chunk changes;
- no actor-state migration without a separate gate.

Focused verification:
- old NPC module tests;
- runtime session/save tests if touched;
- render-frame tests if projection is touched.

## Phase 3: UI Preview Consumer

Status: Milestone 1 implemented and integrated in the existing Qt shell;
remaining UI/editor milestones stay gated.

Goal: consume existing read-only preview model in a UI surface without creating
editor mutation.

Milestone 1 completed:
- first consumer is the existing Qt shell;
- `iggy_qt_shell --preview PATH` accepts one explicit TOML file or package path;
- `--preview-mode run|trace|check|lint` selects the read-only preview mode;
- `UiAuthoringPreviewPanelModel` projects
  `RuntimeGameplayAuthoringPreviewModel` into UI sections/rows;
- `panel:authoring_preview` renders the read-only preview panel.

Milestone 1 display surface:
- selected path;
- package metadata when present;
- read/adapt/convert/run/check status;
- diagnostics with source location;
- summary counts;
- final rows;
- trace frames;
- expectation result;
- actor/player final state where available;
- interaction target and inventory expectation state where available.

Remaining later UI milestones:
- source-linked diagnostics;
- richer visual trace playback and expectation inspection;
- product/play shell boundary;
- build canvas and structured authoring controls only after separate gates;
- source/TOML roundtrip only after explicit mutation/roundtrip approval.

Hard stops:
- no editing;
- no TOML/source mutation;
- no file watcher;
- no package scanning;
- no UI-owned parsing;
- no new gameplay semantics.

Focused verification:
- preview model tests;
- UI model/projection tests;
- shell smoke for explicit preview options;
- screenshot/browser/UI checks only when an interactive visual QA packet opens
  that scope.

Exit criteria:
- a user can open one explicit TOML/package path and inspect status/diagnostics
  and final/trace state from the UI.

## Phase 4: Product Loop Gate

Status: Packets 1, 2, 3A, 3B-A, 4-A, 4-B, the app-neutral play-surface frame,
play-mode state, read-only product play UI projection, Qt `--play` launch
consumer, Qt product input focus toggle, and ready/focused Qt keyboard product
input mapping complete; runtime/product presentation camera policy complete;
runtime/product manual frame request wrapper complete; product input binding
context projection complete; product input accumulator complete; Qt product
manual Step consumer now uses accumulator output plus projected context; Qt
product frame pump toggle complete; product gameplay actor debug/material quad
projection complete; Product Pointer Tile Input Mapping Option A complete as
runtime/product pointer projection plus accumulator event preservation; Qt
product viewport owner complete as an app-shell viewport boundary; thin Qt
product mouse `PrimaryTile` consumer complete for focused ready left-clicks on
that viewport; scene-only `InteractionTargetSpatialQuery2D` complete as the pure
spatial lookup primitive; runtime/product `RuntimeGameplayProductInteractionTargetQuery`
complete as a read-only point/tile-center target query plus reach report; next
work is target context wiring, explicit interact target synthesis,
reach-gated interaction execution, textured sprite/animation/material/asset
policy, and first-play UX policy gates.

Goal: define and build the first playable loop boundary without making authoring
or session state own product concerns.

Packet 1 complete:
- `RuntimeGameplayProductScenarioLoader` supports explicit TOML files, package
  directories, and `package.toml` paths.
- The loader reads TOML, adapts through
  `RuntimeGameplayScenarioAuthoringAdapter`, validates with
  `RuntimeGameplayProfileScenarioValidator`, and exposes
  `RuntimeGameplayProfileScenarioDefinition` plus initial state.
- `RuntimeGameplayTomlScenarioPackageReader` owns shared package
  manifest/path reading for the package facade and product loader.
- The loader is load-only: no frame execution, final rows, trace, check,
  expectation comparison, CLI output, UI, raw input mapping, render/camera,
  save/load, package discovery/scanning/watching, source mutation, or new
  gameplay semantics.

Packet 2 complete:
- `RuntimeGameplayProductLoop` builds product loop state from a successful
  `RuntimeGameplayProductScenarioLoadResult`.
- Product loop state copies identity/package metadata, definition, lowered
  scenario from `load.validation.build.scenario`, initial/current state, and
  starts at `nextFrameIndex = 0`.
- Each step consumes exactly one lowered frame, replaces authored/scripted
  player frame intents with caller-provided `PlayerInputIntent2D`, runs one
  `RuntimeGameplayOrchestratedFrameStep`, carries current state forward, and
  increments `nextFrameIndex`.
- Guard statuses cover failed load, not-loaded state, and exhausted frames.
- The explicit collision-world overload delegates to existing frame-step
  behavior without building or caching worlds.

Packet 3A complete:
- `PlayerInputBinding2D` maps device-agnostic normalized actions to
  `PlayerInputIntent2D`.
- `PlayerInputContext2D` is carried through as data.
- Supported action shapes include move point/tile/delta, interact/inspect target
  fallback, wait, cancel, no-op, and issue reporting.
- Target fallback order is explicit action target, then selected target, then
  hovered target; missing target emits `MissingTarget` and no intent.
- Tile delta requires current player tile; missing current tile emits
  `MissingCurrentPlayerTile` and no intent.
- Emitted intents preserve action order and exclude no-op/issues.
- Binding does not apply gate rules or command mapping; `Inspect` and `Cancel`
  remain intents only.

Packet 3B-A complete:
- `RuntimeGameplayProductInputAdapter` maps transient product input events to
  `PlayerInputBindingAction2D` actions plus carried
  `PlayerInputBindingContext2D`.
- The adapter is device-agnostic runtime/product surface, not Qt shell wiring or
  product shell launch/play mode.
- Release/no-op policy, cardinal movement controls to tile deltas,
  interact/inspect target copy or fallback handoff, wait/cancel, primary
  tile/point payload validation, unsupported-control issues, stable counts,
  order preservation, and input immutability are covered.
- Downstream binding to player intents remains owned by `PlayerInputBinding2D`.
- The adapter does not call the product loop, step gameplay, execute gates or
  commands, map Qt/OS events, own camera/render data, or add save/persistence
  semantics.

Packet 4-A complete:
- `RuntimeGameplayProductLoopStepInput` supports an optional per-step
  `PlayerInputContext2D` override.
- Default behavior is unchanged: when no override is set, product loop keeps the
  authored/lowered frame context while replacing current state and caller
  intents.
- When the override is set, normal and explicit collision-world step paths pass
  it into the existing lower-level frame-step/gate path.
- The override is per-step policy only; it is not persisted in product loop
  state, runtime session/gameplay state, save snapshots, or UI models.
- Product-loop load behavior, frame indexing, collision-world behavior, and
  caller-intent replacement semantics are unchanged beyond the optional context
  override.

Packet 4-B complete:
- `RuntimeGameplayProductPresentationFrame` is a projection-only
  runtime/product presentation wrapper.
- Input is product loop state plus caller-owned `CameraState` plus
  `LevelRenderFrame2DConfig`.
- Loaded product loop state projects `state.currentState.session.level` through
  existing `LevelRenderFrame2D::build(...)` and returns a
  `LevelRenderFrame2DResult` with `Rendered` status, then appends product actor
  debug/material quad commands after level render commands.
- Not-loaded product loop state returns `NotLoaded`, echoes/copies the camera,
  leaves the level frame default, and emits no actor commands.
- The wrapper does not step gameplay, change product-loop state/index/context
  behavior, own camera lifecycle, persist presentation data, render textured
  sprites or animation, perform IO, touch UI/CLI, or add save/load behavior.

Product gameplay actor render projection complete:
- Added runtime-only `RuntimeGameplayProductActorRenderCommands`.
- It projects current gameplay actors into untextured debug/material quad render
  commands without mutating or owning player/NPC state.
- It emits the player when `state.session.hasPlayer` is true, then present
  modern NPC actors from `state.npcActors.actors` in registry order.
- Defaults are `material:player`, `material:npc_actor`, centered 1x1 quads, and
  layer 20, with configurable include flags, material IDs, size, anchor, and
  layer.
- Bounds follow the legacy NPC render convention: negative size is normalized,
  position subtracts size times anchor, and zero-size degenerate bounds are
  allowed.
- Result exposes the command list plus emitted player/NPC/command counts.
- `RuntimeGameplayProductPresentationFrame` carries actor render config/result
  and appends actor commands after `LevelRenderFrame2D` output through
  `RenderCommandList2DComposer::append(...)`.
- Qt product play/manual Step/pump can receive latest-frame presentation
  commands that include product actor quads through the existing frame request
  path, but Qt does not own render semantics.
- This is not textured sprite/animation sampling, asset-driven rendering, a
  material registry/package discovery feature, or a render/backend ownership
  change.

Play-surface frame complete:
- `RuntimeGameplayProductPlaySurfaceFrame` is a runtime-only app-neutral facade
  for one caller-requested play frame.
- It composes existing public surfaces: `RuntimeGameplayProductInputAdapter`,
  `PlayerInputBinding2D`, `RuntimeGameplayProductLoop` one-frame step with
  per-step context override, and `RuntimeGameplayProductPresentationFrame`.
- Focused flow is product input events to binding actions, then player
  intents/context, then exactly one product-loop step, then presentation
  projection using the returned step state.
- Not-loaded flow skips adapter/binding/step, delegates to the presentation
  wrapper, and counts input events as ignored.
- No-frame/exhausted flow skips adapter/binding/step, synthesizes a no-frame
  step guard result, renders current loaded state through the presentation
  wrapper, and counts input events as ignored.
- Unfocused flow ignores transient events before adapter semantics, runs
  adapter/binding on empty events/actions with carried context, steps once with
  empty intents/context override, and presents post-step state.
- The result exposes nested adapter, binding, step, and presentation results
  plus ignored input count.
- The facade is not a Qt shell, product launch mode, automatic tick loop, UI
  adapter, raw OS event mapper, camera lifecycle owner, or save/load surface.

Play-mode state complete:
- `RuntimeGameplayProductPlayMode` is a runtime-only app-neutral state wrapper.
- Durable play-mode state owns only `RuntimeGameplayProductLoopState loop` and a
  durable `hasInputFocus` bit.
- Build accepts an existing `RuntimeGameplayProductLoopBuildResult`, mirrors the
  loop build status as `Ready` or `LoadFailed`, stores the nested loop build
  result, copies loop state on ready, defaults focus to true, and performs no
  loader, file, package, or TOML IO.
- `withInputFocus(...)` is a pure state helper that toggles only focus and
  preserves loop state.
- `frame(...)` delegates exactly once to
  `RuntimeGameplayProductPlaySurfaceFrame` using durable loop state/focus plus
  transient product input frame and caller-owned camera/render config.
- On `Stepped`, returned play-mode state carries `surface.step.state` and focus
  unchanged.
- On `NoFrameAvailable` or `NotLoaded`, returned play-mode state preserves the
  input loop state and focus unchanged; no completion/failure/loading UX is
  introduced.
- Camera/config/input frame remain transient caller inputs and are not stored in
  play-mode state.

Product play UI projection complete:
- `UiProductPlayModePanelModel` is a read-only scene/UI projection for product
  play build/state/latest-frame pointers.
- `UiFeatureContext` carries direct product play pointers and a presence helper.
- Runtime workspace registration includes `feature:product_play` and
  `panel:product_play`.
- The workspace populates the product play panel only when product play context
  exists and emits no missing-context diagnostic.
- Product play panel visibility remains controlled by existing assignments and
  settings and is hidden by default.
- Projection rows/counts are backed by existing runtime result fields: build and
  loop status, identity paths, loaded/focus/frame index/scenario frame count,
  latest frame/surface status, ignored input count, adapter counts, binding
  counts, step status/frame/counts, and presentation/render counts.
- The UI model does not execute frames, call loaders, call
  `RuntimeGameplayProductPlayMode::frame`,
  `RuntimeGameplayProductPlaySurfaceFrame::build`, product step/run functions,
  or product loader/file/package/TOML APIs, mutate product play/runtime state,
  create default camera/render config, map raw input, or add Qt shell/product
  launch behavior.

Qt product play launch consumer complete:
- `iggy_qt_shell --play PATH` is an explicit-path launch/load/build consumer.
- Launch runs only `RuntimeGameplayProductScenarioLoader::load(path)`,
  `RuntimeGameplayProductLoop::build(load)`, and
  `RuntimeGameplayProductPlayMode::build(loopBuild)`.
- `IggyQtShellWindow` stores load, loop, play-mode build results and play-mode
  state so product play panel context pointers remain stable.
- The shell sets product play context pointers, reveals existing read-only
  `panel:product_play`, and leaves the latest product play frame pointer null.
- `--play` and `--preview` are mutually exclusive and hard-error with exit 2.
- Missing `--play` path exits 2.
- Present but bad filesystem paths still open the Qt shell and show failed
  load/build state through the product play panel.

Qt product viewport owner complete:
- Added Qt-shell-owned `QFrame *productViewport_` in `IggyQtShellWindow`.
- `buildMainSlot()` creates `QFrame#productViewport` only when
  `hasProductPlayMode_` is true.
- The viewport exists for product play sessions, including failed/not-ready play
  sessions; it does not imply input readiness.
- `productViewport_` is reset to `nullptr` during main-slot rebuild when no
  viewport is created.
- The viewport uses zero-margin/zero-spacing layout and expanding size policy
  inside `QFrame#mainSlot`, plus a local stylesheet for
  `QFrame#productViewport` only.
- It creates a stable event/render target boundary, but does not define render
  semantics, target discovery, or runtime truth.
- No render command drawing, canvas polish, runtime/product/scene/UI API change,
  or persistence change is included.

Thin Qt product mouse primary-tile consumer complete:
- `productViewport_` installs a viewport-only event filter in product play
  sessions.
- Focused, ready left mouse press on `productViewport_` records exactly one
  transient `RuntimeGameplayProductInputEvent2D` into
  `RuntimeGameplayProductInputAccumulator`.
- Event fields are `control = RuntimeGameplayProductInputControl2D::PrimaryTile`,
  `kind = RuntimeGameplayProductInputEventKind::Pressed`, `hasTile = true`, and
  `tile = projection.tile`.
- The handler accepts/returns true only when a product event is recorded;
  otherwise it falls through.
- Gating is `watched == productViewport_`, `QEvent::MouseButtonPress`, left
  button only, ready product play via existing focus availability,
  `Product Input Focus` enabled, non-null viewport, viewport width/height
  greater than zero, and configured product camera-view axes nonzero.
- The corrected implementation normalizes Qt pixel-local mouse coordinates into
  the configured product camera-view span before calling
  `RuntimeGameplayProductPointerProjection`.
- It uses `productPresentationCameraConfig().cameraView.viewportSize` as the
  projection viewport size, not raw widget pixel size.
- It normalizes `QMouseEvent::position()` by `productViewport_` pixel
  width/height into `fabs(viewSize.x/y)`, so widget pixels do not become an
  oversized world span.
- It builds `RuntimeGameplayProductPresentationCamera` read-only from
  `productPlayState_` and `productPresentationCameraConfig()` and stores no
  camera or pointer state from the mouse handler.
- Qt types are converted at the Qt boundary only; no Qt types enter
  runtime/product APIs.
- No `PrimaryPoint`, world-point payload, right/middle/move/wheel/double-click/
  drag/hover behavior, frame execution, target lookup, interaction execution, or
  persistence is included. Product Step and Product Frame Pump consume the
  accumulator later.
- Existing `--preview` behavior remains unchanged.
- Launch does not step frames, call `RuntimeGameplayProductPlayMode::frame(...)`
  or `RuntimeGameplayProductPlaySurfaceFrame::build(...)`, map raw Qt/device
  input, create product input events, create camera/render config, create
  presentation frames, pump app ticks, save/load, scan/watch/discover packages,
  or add gameplay semantics.

Qt product play focus toggle complete:
- The Qt shell View menu includes a checkable `Product Input Focus` action for
  `--play` sessions.
- The action is applicable/enabled only when product play exists and
  `RuntimeGameplayProductPlayModeBuildStatus::Ready`.
- Toggling updates only durable current `productPlayState_` through
  `RuntimeGameplayProductPlayMode {}.withInputFocus(productPlayState_, enabled)`.
- The toggle keeps `context_.productPlayModeState = &productPlayState_` and
  `context_.latestProductPlayModeFrame = nullptr`.
- The toggle refreshes existing read-only `panel:product_play`, so the existing
  `hasInputFocus` row changes between yes/no.
- Launch with `--play` still starts focused by default from runtime play-mode
  build defaults.
- Bad-path or failed-load `--play` sessions continue to show failed load/build
  state; the focus action is disabled/non-applicable and does not invent ready
  state.
- `productPlayBuild_.state` remains the build result; durable current state is
  `productPlayState_`.
- Existing `--preview` behavior remains unchanged.
- The toggle does not call `RuntimeGameplayProductPlayMode::frame(...)` or
  `RuntimeGameplayProductPlaySurfaceFrame::build(...)`, route Qt key/mouse/focus
  events into `RuntimeGameplayProductInputEvent2D`, create product input events,
  camera/render config, presentation frames, latest-frame results, frame steps,
  app tick loops, settings persistence, or keyboard shortcuts.

Qt keyboard product input mapping complete:
- The Qt shell maps supported keyboard press/release events for ready, focused
  `--play` sessions into app-shell-owned transient product accumulator state.
- It records product controls/events only, not raw `QKeyEvent` objects or
  pointers.
- Events are recorded only when product play mode exists, play-mode build status
  is `RuntimeGameplayProductPlayModeBuildStatus::Ready`, and
  `productPlayInputFocusEnabled()` is true.
- Disabled focus, failed load, or not-ready play state records no event; turning
  product input focus off clears accumulator state.
- Auto-repeat and unsupported keys are ignored.
- Arrow/WASD map to `MoveNorth`, `MoveSouth`, `MoveWest`, and `MoveEast`;
  `E`/Return/Enter map to `Interact`; `I` maps to `Inspect`; Space maps to
  `Wait`; Escape maps to `Cancel`.
- Binding context is supplied later when building request frame output; no
  selected target, hovered target, scene/UI model exposure, or settings exposure
  was added.
- The mapping does not call `RuntimeGameplayProductPlayMode::frame(...)`,
  `RuntimeGameplayProductPlaySurfaceFrame::build(...)`,
  `RuntimeGameplayProductInputAdapter::map(...)`, or
  `PlayerInputBinding2D::bind(...)`; it does not create camera/render config,
  presentation frames, latest-frame results, product frame steps, manual step
  actions, app tick loops, or frame pumps.

Product presentation camera policy complete:
- `RuntimeGameplayProductPresentationCamera` is an app-neutral runtime/product
  policy for choosing transient caller-owned `CameraState` plus
  `LevelRenderFrame2DConfig` from product play state and caller-owned config.
- Not-loaded state supports previous-camera and fallback-camera selection.
- Loaded state supports player initialization and previous-camera player follow
  through existing `CameraRig`.
- Follow-disabled paths preserve previous/fallback behavior.
- Clamp behavior is reported through result flags from `CameraRig`.
- Render config forwarding preserves view/camera view config, NPC command flag,
  tile chunk cache flag, and cache pointer.
- The policy does not execute frames, call
  `RuntimeGameplayProductPlaySurfaceFrame::build(...)`, call
  `RuntimeGameplayProductPlayMode::frame(...)`, call product input
  adapter/binding, persist camera/presentation/render-frame data, or add
  Qt/UI/CLI behavior.

Product manual frame request wrapper complete:
- `RuntimeGameplayProductFrameRequest` is an app-neutral runtime/product wrapper
  for one caller-requested manual product frame.
- It composes `RuntimeGameplayProductPresentationCamera {}.build(...)` first.
- It then calls `RuntimeGameplayProductPlayMode {}.frame(...)` exactly once with
  the selected transient camera/render config.
- It maps play-mode frame status directly to frame-request status.
- It returns the carried next `RuntimeGameplayProductPlayModeState` from the
  nested play-mode frame result.
- It projects `inputEventCount` from the supplied transient input frame and
  `ignoredInputEventCount` from the nested play-surface result.
- It preserves nested camera and play-mode frame results without flattening
  invented fields.
- Caller ownership remains explicit for input-frame clearing/draining,
  previous-camera storage, latest-frame storage, and presentation state
  ownership.
- The wrapper does not change `RuntimeGameplayProductPlayMode::frame(...)`,
  add Qt/UI/CLI behavior, add a Qt manual Step button, add an automatic tick
  loop/frame pump, map mouse screen-to-world/tile coordinates, enrich context
  from game state, run post-step camera follow, persist raw input/camera/latest
  frame state, add textured sprite/animation/material policy, or add UX/save/
  load semantics.

Product input binding context projection complete:
- `RuntimeGameplayProductInputContext` is an app-neutral runtime/product
  projection for transient `PlayerInputBindingContext2D`.
- Statuses are `NotLoaded`, `LoadedWithoutPlayer`, and `Projected`.
- Projection returns default `PlayerInputBindingContext2D` gates.
- It sets only `hasCurrentPlayerTile/currentPlayerTile` when product play state
  is loaded and `currentState.session.hasPlayer` is true, using existing
  `playerTile(...)`.
- It does not set selected or hovered targets, inspect interaction state, search
  targets, map mouse input, synthesize `PrimaryPoint` or `PrimaryTile`, or add
  held-key cadence, repeat, input accumulator, or pump behavior.
- The projected context is transient request input only and is not stored in
  runtime/session/gameplay/product-loop/play-mode state, snapshots, saves,
  settings, or scene/UI models.

Product input accumulator complete:
- `RuntimeGameplayProductInputAccumulator` is a shell-neutral return-by-value
  transient input helper.
- Accumulator state stores product-level data only: held
  `RuntimeGameplayProductInputControl2D` values and pending
  `RuntimeGameplayProductInputEvent2D` one-shot events.
- Held movement controls are exactly `MoveNorth`, `MoveSouth`, `MoveWest`, and
  `MoveEast`.
- One-shot controls are exactly `Interact`, `Inspect`, `Wait`, and `Cancel`.
- Held movement press adds a control, duplicate held press is suppressed,
  release removes held movement, and release of non-held movement is a no-op.
- One-shot press queues an event and one-shot release is a no-op.
- Frame output emits ordinary `Pressed` events for held movement controls every
  requested frame, followed by pending one-shot events once, preserving held
  press order before one-shot press order.
- `buildFrame(...)` carries supplied `PlayerInputBindingContext2D`, preserves
  held controls, and drains one-shots in the returned state.
- `clear(...)` returns empty transient state.
- Explicit pressed `PrimaryPoint` and `PrimaryTile` events are payload-preserving
  pending one-shots drained on the next `buildFrame(...)`.
- Primary releases are no-ops, and the accumulator still does not synthesize
  point/tile events.
- Accumulator state is not raw Qt input and is not persisted in
  runtime/session/gameplay/product-loop/play-mode state, snapshots, saves,
  settings, or scene/UI models.

Product pointer projection Option A complete:
- Added Qt-free `RuntimeGameplayProductPointerProjection`.
- It maps viewport-local points through existing `CameraView` normalization to
  world point plus `tileForPoint(...)` tile.
- It uses only `CameraState`, `CameraView`, `CameraViewConfig`, and `TileCoord`;
  no Qt types or scene/UI model ownership.
- It preserves `CameraView` behavior for negative viewport dimensions,
  non-positive zoom, and zero-axis degenerate viewports.
- It returns projection status/flags, world point, and tile so a future consumer
  can choose `PrimaryTile`, `PrimaryPoint`, or both under a separate policy
  gate.
- The runtime helper does not own Qt routing, target lookup, hover/selection
  workflow, interaction execution, persistence, or product loop/frame request/
  play mode semantic changes.

Qt product manual Step consumer complete:
- The Qt shell View menu includes a `Product Step` action for ready `--play`
  sessions.
- The action is enabled only when product play mode exists and build status is
  ready.
- It is independent of `Product Input Focus`; when focus is false, existing
  play-surface behavior ignores input but still consumes one available frame
  with empty intents/context.
- Qt stores accumulator state instead of a latest-edge `productInputFrame_`.
- Key recording maps keys to product controls and records product events into
  accumulator state without clearing per key event.
- Focus-disabled state clears accumulator state.
- Executing the action builds accumulator frame output with projected binding
  context, stores the returned accumulator state before the frame request, and
  builds `RuntimeGameplayProductFrameRequestInput` from current
  `productPlayState_`, accumulator output, and shell-owned camera config.
- It calls `RuntimeGameplayProductFrameRequest {}.run(input)` exactly once.
- It updates only replaceable app-shell transient state: `productPlayState_`,
  `latestProductPlayModeFrame_` plus the stable product play panel context
  pointer, and `productPresentationCamera_` for the next request.
- It preserves held movement across steps and drains one-shots after each
  executed Step.
- It does not persist derived binding context back into accumulator state.
- If no request executes because Step is unavailable, input is not silently
  cleared.
- It refreshes the existing product play panel after the request.
- Shell camera/config defaults are presentation-only: explicit fallback/view
  defaults, NPC commands enabled, and tile chunk cache disabled. They are not
  persisted in settings or saves.
- The action does not add automatic app/tick loops, frame pumps, mouse
  screen-to-world/tile mapping, textured sprite/animation/material policy,
  pause/retry/reset/completion/failure/save-load semantics, package
  scanning/watching/discovery, source mutation, held-key cadence behavior,
  settings persistence, or keyboard shortcuts.

Qt product frame pump toggle complete:
- The Qt shell View menu includes a checkable `Product Frame Pump` action for
  ready `--play` sessions.
- The pump is Qt/app-shell-owned replaceable timing only, using a window-owned
  `QTimer` at 250 ms / 4 Hz.
- Manual `Product Step` and timer ticks share the same one-frame helper/path.
- The shared path builds accumulator output plus projected input context, stores
  returned accumulator state so held movement survives and one-shots drain,
  calls `RuntimeGameplayProductFrameRequest {}.run(input)` exactly once per
  request/tick, updates current `productPlayState_`, latest frame
  member/context pointer, previous presentation camera, product play state
  context pointer, and refreshes the product play panel.
- Pump availability is ready product play only: product play exists and
  play-mode build is ready.
- The pump is independent of `Product Input Focus`; focus remains only the input
  gate, and existing play-surface behavior ignores input while unfocused but can
  still consume available frames.
- The timer stops if product play becomes unavailable or not ready.
- The timer does not auto-stop on `NoFrameAvailable`; no completion/end-state UX
  semantics were added.
- Pump enabled state, interval, and keybindings are not persisted in settings or
  saves.
- The pump does not change runtime/product APIs, scene/UI models, product loop,
  frame request, play mode, input accumulator, or gameplay semantics.

Remaining input gate questions:
- How should transient hovered/selected target context consume the read-only
  `RuntimeGameplayProductInteractionTargetQuery` report?
- Should explicit interact target synthesis consume a `TargetFound` report, and
  should it require `reachable=true`?
- Should a later point-vs-tile policy add `PrimaryPoint` or world-point payloads
  beyond the current `PrimaryTile`-only click behavior?
- Should selected/hovered target context be projected later, and what explicit
  target ownership/search gate would be required?
- What textured sprite/animation/material/asset policy should replace or extend
  the current debug/material actor quads?
- Who owns pause/retry/reset?
- What state must save/load for the first playable slice?
- What does completion/failure mean in the first slice?

Output:
- one optional target context / explicit interact-target packet over the product
  query report, if approved;
- one textured sprite / animation / material policy packet, if approved;
- one updated implementation order packet;
- one verification plan.

Hard stops:
- do not make authoring facade own product runtime;
- do not call TOML/package facades or file IO from the product loop;
- do not use full scenario/profile runners from the product loop;
- do not hide raw input inside runtime session state;
- do not add Qt/OS event types to scene/player or runtime product input
  surfaces;
- do not add gate execution or command mapping to `PlayerInputBinding2D`;
- do not change `RuntimeGameplayProductLoop` signatures or stepping as part of
  later input work;
- do not persist raw input or context overrides in runtime/session/product-loop
  state or snapshots;
- do not put presentation/camera state into gameplay truth;
- do not store presentation camera policy output as gameplay/session/product-loop
  or play-mode truth;
- do not productize save/load in the next input or presentation packet;
- do not add pause/retry/reset or completion/failure/win/lose semantics in the
  input or presentation packet;
- do not treat `RuntimeGameplayProductPresentationFrame` as Qt shell/play mode;
- do not treat `RuntimeGameplayProductPlaySurfaceFrame` as Qt shell, product
  launch mode, automatic tick loop, or UI adapter;
- do not treat `RuntimeGameplayProductPlayMode` as Qt shell behavior, product
  launch mode, UI adapter, or automatic app/tick loop;
- do not treat `UiProductPlayModePanelModel` as Qt shell behavior, product
  launch mode, UI execution, or automatic app/tick loop;
- do not call `RuntimeGameplayProductPlayMode::frame`,
  `RuntimeGameplayProductPlaySurfaceFrame::build`, loader APIs, or product
  step/run functions from scene/UI projections;
- do not mutate product play mode state or runtime/gameplay state from scene/UI;
- do not call `RuntimeGameplayProductPlayMode::frame(...)` or
  `RuntimeGameplayProductPlaySurfaceFrame::build(...)` from Qt launch;
- do not add product input events, default camera/render config, latest frame
  synthesis, product frame stepping, or app tick-loop/frame pump to Qt launch;
- do not combine `--play` and `--preview`;
- do not route Qt key/mouse/focus events into
  `RuntimeGameplayProductInputEvent2D` from the focus toggle;
- do not add product input events, default camera/render config, latest frame
  synthesis, product frame stepping/manual stepping, or app tick-loop/frame pump
  to the focus toggle;
- do not persist focus toggle state into settings or add a keyboard shortcut;
- do not store raw `QKeyEvent` objects or pointers outside Qt shell event
  handling;
- do not call `RuntimeGameplayProductInputAdapter::map(...)`,
  `PlayerInputBinding2D::bind(...)`,
  `RuntimeGameplayProductPlayMode::frame(...)`, or
  `RuntimeGameplayProductPlaySurfaceFrame::build(...)` from Qt input mapping;
- do not add Qt mouse behavior beyond viewport-only focused ready left-click
  `PrimaryTile`; do not create `PrimaryPoint`, world-point payloads, right/
  middle/move/wheel/double-click/drag/hover behavior, or Qt types in
  runtime/scene/product APIs;
- do not expose product input frames through scene/UI models or settings;
- do not persist raw input in runtime session/gameplay/product-loop state,
  snapshots, saves, or UI models;
- do not persist raw input in play-mode state; only durable focus is stored;
- do not persist camera, presentation, or render-frame data as
  gameplay/session/product-loop/play-mode/save truth;
- do not call product loader/file/package/TOML APIs from play mode;
- do not add textured sprite/animation sampling, new art/assets, material
  registry, or package discovery without a separate gate;
- do not add Qt/UI/CLI behavior, product frame stepping, app tick loop, product
  input adapter/binding execution, or raw input changes to presentation camera
  policy;
- do not add Qt/UI/CLI behavior, Qt manual Step button, automatic tick
  loop/frame pump, input-frame draining policy, previous-camera/latest-frame
  storage, context enrichment from game state, post-step camera follow, mouse
  screen-to-world/tile mapping, raw input persistence, save/load, or UX
  semantics to product frame request;
- do not add automatic app/tick loops, frame pumps, mouse screen-to-world/tile
  mapping, textured sprite/animation/material policy, pause/retry/reset/completion/
  failure/save-load semantics, package scanning/watching/discovery, source
  mutation, held-key cadence behavior, settings persistence, or keyboard
  shortcuts to Qt manual Step;
- do not silently clear transient input when Qt manual Step is unavailable;
- do not persist projected binding context in runtime/session/gameplay/
  product-loop/play-mode state, snapshots, saves, settings, scene/UI models, or
  the stored Qt product input frame;
- do not add selected/hovered target discovery, interaction target search,
  reach lookup, mouse screen-to-world/tile mapping, `PrimaryPoint`,
  `PrimaryTile`, held-key cadence, repeat behavior, input accumulator, or frame
  pump behavior to product input context projection;
- do not persist accumulator state in runtime/session/gameplay/product-loop/
  play-mode state, snapshots, saves, settings, or scene/UI models;
- do not add raw Qt key/event storage, cadence/rate policy, automatic frame
  pump behavior, pointer synthesis, selected/hovered target discovery,
  interaction reach lookup, target search, textured sprite/animation/material
  policy, pause/retry/reset/completion/failure/save-load productization,
  package scanning/watching/discovery, or source mutation to product input
  accumulator or its Qt integration;
- do not treat `RuntimeGameplayProductPointerProjection` as product
  viewport/canvas ownership, Qt mouse routing, target discovery/search/reach,
  hover/selection workflow, interaction execution, drag tooling, or persistence;
- do not persist raw input, accumulator state, mouse state, camera,
  presentation, render frames, Qt state, actor render config, or projected
  pointer data in runtime/session/gameplay/product-loop/play-mode saves,
  snapshots, settings, or scene/UI model truth;
- do not treat Qt `productViewport_` as render semantics, target discovery,
  runtime truth, or readiness; do not add event handling beyond the approved
  viewport-only focused ready left-click `PrimaryTile` path, synthesize
  `PrimaryPoint`, attach world-point payloads, draw render commands, add canvas
  polish, or persist viewport geometry/state in runtime/session/gameplay/
  product-loop/play-mode saves, settings, snapshots, or scene/UI model truth;
- do not treat Qt product frame pump as runtime/product semantic ownership;
- do not persist pump enabled state, interval, keybindings, input accumulator,
  pump state, camera, latest frame, or render-frame data in runtime/session/
  gameplay/product-loop/play-mode snapshots, saves, settings, or scene/UI model
  truth;
- do not add textured sprite/animation/material policy, additional Qt mouse
  behavior, `PrimaryPoint` synthesis, selected/hovered target discovery,
  interaction target search/reach lookup, pause/retry/reset/completion/failure/
  save-load productization, package scanning/watching/discovery, source
  mutation, raw Qt event persistence, or product loop/frame request/play mode/
  input accumulator semantic changes to Qt product frame pump;
- do not persist render commands, actor render config, camera/presentation
  state, pump state, raw input, accumulator state, latest frame, or Qt state in
  runtime/session/gameplay/product-loop/play-mode snapshots, saves, settings, or
  scene/UI model truth;
- do not make product actor render projection own player/NPC state, mutate
  gameplay state, sample textured sprites/animation, add art/assets/material
  registry/package discovery, add Qt types, or change product loop/frame
  request/play mode stepping semantics;
- do not persist derived caches as save truth.

## Phase 5: Presentation / Render Integration

Status: projection-only product presentation wrapper, runtime-only app-neutral
play-surface frame, app-neutral play-mode state, read-only product play UI
projection, Qt `--play` launch/load/build consumer, and Qt product input focus
toggle plus ready/focused keyboard product input mapping integrated; product
presentation camera policy and manual frame request wrapper integrated; Qt
manual Step consumer integrated; product input binding context projection and
product input accumulator integrated; Qt product frame pump toggle integrated;
product gameplay actor debug/material quad projection integrated; textured
sprites/animation/material/asset policy, target context projection/search/reach,
interaction execution, product UX/save semantics, and UI execution beyond launch/
focus/input capture/manual step/pump/viewport ownership/primary-tile mouse input
remain separate gates.

Goal: turn runtime state into visible play state.

Packets:
1. Caller-driven product presentation frame. Complete:
   - loaded product loop state + caller-owned camera/config -> level render
     frame result.
2. Runtime product play-surface frame. Complete:
   - transient product input events + focus/context/camera/config -> nested
     adapter, binding, one-step, and presentation results.
3. Runtime product play-mode state. Complete:
   - durable loop state + focus bit -> caller-requested play-surface frames.
4. Read-only product play UI projection. Complete:
   - supplied build/state/latest-frame pointers -> product play panel rows.
5. Qt product play launch consumer. Complete:
   - explicit path -> load/build/play-mode context pointers -> read-only panel.
6. Qt product input focus toggle. Complete:
   - ready play mode -> focus bit toggle -> read-only panel refresh.
7. Qt keyboard product input mapping. Complete:
   - ready/focused key press/release -> transient latest product input event.
8. Runtime product presentation camera policy. Complete:
   - product play state + caller config -> transient camera/render config using
     existing follow/rig/clamp behavior outside gameplay truth.
9. Runtime product manual frame request wrapper. Complete:
   - camera policy + exactly one play-mode frame -> nested request result.
10. Product input binding context projection. Complete:
   - loaded product play state with player -> current-player-tile binding
     context only.
11. Product input accumulator. Complete:
   - held movement + one-shot product events -> transient request frame output.
12. Qt product manual Step consumer. Complete:
   - ready `--play` View action -> accumulator output + one frame request ->
     transient shell state update and panel refresh.
13. Qt product frame pump toggle. Complete:
   - ready `--play` View toggle -> 250 ms Qt timer using the same one-frame
     helper as manual Step.
14. Product gameplay actor render projection. Complete:
   - current player + present modern NPC actors -> untextured debug/material
     quad commands appended after level render output.
15. Product pointer projection Option A. Complete:
   - viewport-local point + camera/view config -> world point/tile projection,
     plus explicit primary point/tile accumulator event preservation.
16. Qt product viewport owner. Complete:
   - `QFrame#productViewport` for product play sessions as app-shell
     event/render target boundary; no render drawing.
17. Thin Qt product mouse primary-tile consumer. Complete:
   - focused ready left-click on `productViewport_` -> normalized camera-view
     projection -> one transient `PrimaryTile` pressed event.
18. Interaction target spatial query. Complete:
   - scene-only `InteractionTargetSpatialQuery2D` -> nearest enabled target in
     range for a point or tile center, with clamped radius policy and
     registry-order tie behavior; no Qt/runtime/product wiring or execution.
19. Product interaction target query. Complete:
   - runtime/product read-only report over point or tile-center target lookup
     plus optional player reach annotation; no selected/hovered state mutation,
     input conversion, command/effect planning, Qt behavior, or execution.
20. Debug overlay projection:
   - trace/final rows, AI map, collision, path, interactions, inventory.
21. UI presentation adapter:
   - convert render frame data into the chosen shell/app surface.

Hard stops:
- no automatic render generation inside gameplay ticks unless approved;
- no backend-specific assumptions in runtime;
- no camera state hidden in save/session until gated.

Focused verification:
- render-frame tests;
- camera/presentation state tests;
- UI presentation tests if target exists.

## Phase 6: Save/Load Productization

Status: gate before code.

Goal: save and resume the first playable slice.

Packets:
1. Authored package save/load roundtrip gate.
2. Runtime state persistence decision:
   - player;
   - NPCs;
   - interaction targets;
   - inventory stacks/drops;
   - command queue if needed;
   - package identity/metadata if needed.
3. Save-slot UX policy:
   - explicit slots vs autosave;
   - overwrite behavior;
   - failure diagnostics.

Hard stops:
- no migration of existing snapshot chunks as cleanup;
- no cloud sync, compression, encryption, or platform storage expansion;
- no derived cache persistence.

Focused verification:
- existing save/load/snapshot tests;
- new roundtrip tests for only approved fields.

## Phase 7: First Playable Slice

Status: after product loop, presentation, and minimal persistence gates.

Goal: produce the first real playable vertical slice.

Default content:
- one package fixture promoted to product demo content;
- player movement;
- one NPC;
- one item pickup;
- one keyed interaction;
- one checkable expectation path;
- visible success/failure/reset behavior.

Implementation packets:
1. Product scenario loader. Complete.
2. Product-owned loop state/step. Complete.
3A. Scene/player normalized input binding. Complete.
3B-A. Runtime/product transient input event adapter. Complete.
3B-B. Optional further raw-device adapter into transient product input events.
4-A. Product loop per-step input context override. Complete.
4-B. Projection-only product presentation frame. Complete.
4-C. Runtime product play-surface frame. Complete.
4-D. Runtime product play-mode state. Complete.
4-E. Read-only product play UI projection/context. Complete.
4-F. Qt product play launch/load/build consumer. Complete.
4-G-A. Qt product play focus toggle. Complete.
4-G-B. Qt keyboard product input mapping. Complete.
4-H. Product presentation camera policy. Complete.
4-I. Product manual frame request wrapper. Complete.
4-J. Product input binding context projection. Complete.
4-K. Product input accumulator. Complete.
5. Qt product manual Step consumer with accumulator/context enrichment. Complete.
5-B. Qt product frame pump toggle. Complete.
6. Product gameplay actor debug/material quad render projection. Complete.
6-B. Product pointer projection and explicit primary input accumulator events. Complete.
6-C. Qt product viewport owner. Complete.
6-D. Thin Qt product mouse `PrimaryTile` consumer. Complete.
6-E. Target discovery/search/reach and interaction execution if approved.
6-F. Textured sprite/animation/material/asset policy if approved.
7. Pause/retry/reset.
8. Minimal save/load if approved.
9. Acceptance fixture/demo.

Verification:
- focused runtime/product tests;
- UI smoke if app surface exists;
- full CTest;
- manual run command documented.

Exit criteria:
- user can run the slice locally and play through it.

## Phase 8: Editor-Backed Game

Status: after playable slice.

Goal: move from read-only preview to controlled authoring surfaces.

Milestones:
1. Source-linked diagnostics.
2. Visual trace playback.
3. Build canvas for placement.
4. Structured controls for existing facts.
5. Source/TOML roundtrip gate.
6. Editable authoring with tests.

Hard stops:
- no arbitrary source mutation before roundtrip gate;
- no UI-owned parser;
- no scripting language.

## Phase 9: Gameplay Semantics Expansion

Status: only after product loop is real unless user explicitly prioritizes.

Candidate gates:
- emit event;
- talk/inspect;
- region triggers;
- terrain metadata;
- equipment;
- combat;
- progression/win/fail conditions.

Rules:
- one semantic gate at a time;
- one tiny fixture-backed implementation at a time;
- no generic condition/effect language without explicit approval.

## Worker Packet Templates

### Scout Packet

```text
Objective:
Files/surfaces to inspect:
Questions to answer:
Hard stops:
Expected output:
Verification:
```

### Builder Packet

```text
Objective:
Files likely touched:
Behavior to add/change:
Behavior to preserve:
Hard stops:
Focused tests:
Batch-end verification:
Required brief:
```

### Reviewer Gate

```text
Scope:
Ownership ruling:
Risks:
Missing tests:
Merge conflicts:
PASS/BLOCK:
```

## Verification Matrix

| Lane | Focused verification | Full verification trigger |
| --- | --- | --- |
| Runtime cleanup | touched runtime tests, summary/facade/CLI tests if projection changes | every cleanup batch |
| Legacy NPC | old module, runtime session, save/load, render tests as touched | any production compatibility change |
| Authoring | source-plan, TOML reader, converter, facade, CLI, manifest sweep | every authoring behavior batch |
| Package | package facade, CLI package tests, file/package parity | every package behavior batch |
| Preview/UI | preview model, UI model/projection, UI smoke when target exists | any UI target or model change |
| Product loop | runtime product-loop tests, input/render/save focused tests | every product-loop batch |
| Save/load | snapshot/chunk/slot/store tests plus new roundtrip tests | every persistence batch |
| Docs-only | `git diff --check` | full build optional unless roadmap-shaping |

Default full verification:
```sh
cmake -S engine -B engine/build
cmake --build engine/build
ctest --test-dir engine/build --output-on-failure
git diff --check
git ls-files '*Devilution*' '*devilution*' '*DevilutionX*' '*master.zip' '*godot*'
git ls-files --others --exclude-standard '*Devilution*' '*devilution*' '*DevilutionX*' '*master.zip' '*godot*'
```

## First Dispatch Sequence

1. Send researcher/reviewer a runtime cleanup scout packet.
2. Convert scout output into one finisher packet.
3. Let finisher build the behavior-preserving cleanup.
4. Merge and run integration verification.
5. Send researcher/reviewer a legacy NPC compatibility map packet.
6. Use the integrated read-only UI preview consumer as product-loop input.
7. Use the integrated load-only product scenario loader as Product Loop Packet 1.
8. Product Loop Packet 2 product-owned loop state/step is integrated.
9. Product Loop Packet 3A scene/player normalized input binding is integrated.
10. Product Loop Packet 3B-A runtime/product input adapter is integrated.
11. Product Loop Packet 4-A per-step input context override is integrated.
12. Product Loop Packet 4-B projection-only product presentation frame is
    integrated.
13. Runtime product play-surface frame is integrated.
14. Runtime product play-mode state is integrated.
15. Read-only product play UI projection/context is integrated.
16. Qt product play launch/load/build consumer is integrated.
17. Qt product play focus toggle is integrated.
18. Qt keyboard product input mapping is integrated.
19. Product presentation camera policy is integrated.
20. Product manual frame request wrapper is integrated.
21. Product input binding context projection is integrated.
22. Product input accumulator is integrated.
23. Qt product manual Step consumer uses accumulator output plus transient
    projected binding context.
24. Qt product frame pump toggle is integrated.
25. Product gameplay actor debug/material quad render projection is integrated.
26. Product pointer projection and explicit primary input accumulator events are
    integrated.
27. Qt product viewport owner is integrated.
28. Thin Qt product mouse `PrimaryTile` consumer is integrated.
29. Interaction target spatial query is integrated.
30. Runtime/product interaction target query report is integrated.
31. Dispatch target context wiring, explicit interact target synthesis,
    reach-gated interaction execution, point-vs-tile policy, textured sprite /
    animation / material policy, render projection gaps, further input mapping,
    or a focused-input follow-up, depending on planner scope.

Do not broaden the next Product Loop packet into pause/retry/reset,
completion/failure, save/load productization, product-loop signature changes,
command/gate execution, presentation state persistence, Qt/UI/CLI behavior,
raw OS event types, raw input persistence, textured sprite/animation/material
policy, target context wiring/search/reach beyond the approved read-only product
query report, additional Qt mouse behavior, render command drawing, canvas
polish, or new gameplay semantics unless the user explicitly reprioritizes.
