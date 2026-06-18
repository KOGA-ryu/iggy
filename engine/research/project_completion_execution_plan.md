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
runtime/product manual frame request wrapper complete; next work is product
shell ownership for invoking frame requests, render projection gaps, automatic
frame pump ownership, any further mouse/world/tile input mapping, and first-play
UX policy gates.

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
  `LevelRenderFrame2DResult` with `Rendered` status.
- Not-loaded product loop state returns `NotLoaded`, echoes/copies the camera,
  and leaves the level frame default.
- The wrapper does not step gameplay, change product-loop state/index/context
  behavior, own camera lifecycle, persist presentation data, render player
  sprites, render modern `RuntimeGameplayState::npcActors`, perform IO, touch
  UI/CLI, or add save/load behavior.

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
  `--play` sessions into the app-shell-owned transient
  `RuntimeGameplayProductInputFrame2D productInputFrame_`.
- It records product input events only, not raw `QKeyEvent` objects or pointers.
- Events are recorded only when product play mode exists, play-mode build status
  is `RuntimeGameplayProductPlayModeBuildStatus::Ready`, and
  `productPlayInputFocusEnabled()` is true.
- Disabled focus, failed load, or not-ready play state records no event; turning
  product input focus off clears the transient input frame.
- The input frame is latest-event bounded: recording clears the frame before
  appending one event.
- Auto-repeat and unsupported keys are ignored.
- Arrow/WASD map to `MoveNorth`, `MoveSouth`, `MoveWest`, and `MoveEast`;
  `E`/Return/Enter map to `Interact`; `I` maps to `Inspect`; Space maps to
  `Wait`; Escape maps to `Cancel`.
- `productInputFrame_.bindingContext` remains default; no current-player tile,
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
  frame state, expand player/modern NPC render projection, or add UX/save/load
  semantics.

Remaining input gate questions:
- Is mouse/world/tile input mapping needed after keyboard mapping, and where
  should any future mapping beyond supported keys remain transient?
- When does the product shell invoke frame requests, drain transient input
  frames, and store previous camera/latest frame results?
- When are player sprite and modern `RuntimeGameplayState::npcActors` render
  projections added?
- Who owns automatic app/tick-loop frame pumping around caller-requested frames?
- Who owns pause/retry/reset?
- What state must save/load for the first playable slice?
- What does completion/failure mean in the first slice?

Output:
- one optional mouse/world/tile input mapping packet, if needed;
- one product shell manual frame request integration packet;
- one automatic frame pump packet, if needed;
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
- do not map mouse position to world/tile coordinates or create `PrimaryPoint`
  or `PrimaryTile` input from Qt input mapping;
- do not expose product input frames through scene/UI models or settings;
- do not persist raw input in runtime session/gameplay/product-loop state,
  snapshots, saves, or UI models;
- do not persist raw input in play-mode state; only durable focus is stored;
- do not persist camera, presentation, or render-frame data as
  gameplay/session/product-loop/play-mode/save truth;
- do not call product loader/file/package/TOML APIs from play mode;
- do not add player sprite or modern NPC actor render projection without a
  separate gate;
- do not add Qt/UI/CLI behavior, product frame stepping, app tick loop, product
  input adapter/binding execution, or raw input changes to presentation camera
  policy;
- do not add Qt/UI/CLI behavior, Qt manual Step button, automatic tick
  loop/frame pump, input-frame draining policy, previous-camera/latest-frame
  storage, context enrichment from game state, post-step camera follow, mouse
  screen-to-world/tile mapping, raw input persistence, save/load, or UX
  semantics to product frame request;
- do not persist derived caches as save truth.

## Phase 5: Presentation / Render Integration

Status: projection-only product presentation wrapper, runtime-only app-neutral
play-surface frame, app-neutral play-mode state, read-only product play UI
projection, Qt `--play` launch/load/build consumer, and Qt product input focus
toggle plus ready/focused keyboard product input mapping integrated; product
presentation camera policy and manual frame request wrapper integrated; player
sprite projection, modern NPC actor projection, product shell invocation of
frame requests, automatic frame pump, further input mapping beyond supported
keyboard controls, and UI execution beyond launch/focus/input capture remain
separate gates.

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
10. Debug overlay projection:
   - trace/final rows, AI map, collision, path, interactions, inventory.
11. UI presentation adapter:
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
5. Product shell frame request integration / manual step surface.
5-B. Automatic frame pump if approved.
6. Player sprite and modern NPC actor render projection.
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
21. Dispatch product shell frame request integration, automatic frame pump,
    render projection gaps, further input mapping, or a focused-input follow-up,
    depending on planner scope.

Do not broaden the next Product Loop packet into pause/retry/reset,
completion/failure, save/load productization, product-loop signature changes,
command/gate execution, presentation state persistence, Qt/UI/CLI behavior,
raw OS event types, raw input persistence, player sprite/modern NPC actor render
projection, or new gameplay semantics unless the user explicitly reprioritizes.
