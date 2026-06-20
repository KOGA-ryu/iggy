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
complete as a read-only point/tile-center target query plus reach report;
runtime/product `RuntimeGameplayProductInputTargetContext` complete as transient
hovered-target binding context enrichment from that report; runtime/product
`RuntimeGameplayProductInputFrameTargetContext` complete as opt-in pre-frame
enrichment over the latest eligible `PrimaryTile` pressed event; Qt manual Step
and frame pump now consume that helper through the shared one-frame path; compact
read-only target-context diagnostics projection is complete in the product panel;
thin Qt product viewport render command drawing is complete as a temporary
app-shell consumer of existing latest-frame quad commands; thin Qt target
highlight overlay is complete as a visual annotation over latest target-context
diagnostics; native no-Qt scripted controls/debugger is complete for
deterministic product-path stepping and expectation checks; native no-Qt
  scene draw-list extraction is complete as backend-neutral app-local renderer
  prep; native no-Qt product session extraction is complete as app-local product
  state/request/script orchestration; native no-Qt Vulkan renderer skeleton
  extraction is complete as app-local renderer lifetime/draw submission
  separation; native no-Qt GPU mesh resource wrapping is complete as
  renderer-private cube mesh buffer ownership cleanup; native no-Qt pipeline/
  shader resource wrapping is complete as renderer-private render pass, pipeline
  layout, graphics pipeline, and shader module ownership cleanup; native no-Qt
  model-slot/cube fallback registry is complete as renderer-private model-slot
  groundwork over the existing cube mesh; native no-Qt static mesh asset data
  model is complete as backend-free CPU mesh data for the existing cube fallback;
  native no-Qt procedural bean mesh slot binding is complete as the first
  non-cube in-memory player mesh proof; native no-Qt procedural NPC mesh slot
  binding is complete as the second non-cube in-memory slot proof; loaded
  player/NPC/floor/wall `.igmesh` bindings are complete; and native static model
  slot policy is complete as app-local value-only `.igmesh` filename policy.
  Next work is optional richer overlays/labels or diagnostics, frame request/
  play-surface ownership decisions, explicit interact target synthesis,
  reach-gated interaction execution, app shell/CLI extraction, other model-slot
  file binding, glTF/glb parsing under the constrained subset, asset registry/
  catalog, materials/textures/descriptors/samplers, package/authoring asset
  policy, backend validation/abstraction, and first-play UX policy gates.

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
- It creates a stable event/render target boundary, but does not define target
  discovery, runtime truth, or long-term renderer ownership.
- No target highlighting from viewport ownership itself, canvas polish,
  runtime/product/scene/UI API change, or persistence change is included.

Thin Qt product viewport render command drawer complete:
- `QFrame#productViewport` is a Qt-local paint-capable widget that draws
  existing latest product play frame render commands.
- It consumes only stable, transient app-shell latest-frame data from
  `latestProductPlayModeFrame_`:
  `surface.presentation.levelFrame.commands.commands` and
  `surface.presentation.levelFrame.cameraView.bounds`.
- It uses latest-frame `levelFrame.cameraView.bounds` as visible world bounds
  plus current widget size to map command world bounds to pixel rectangles.
- It normalizes command world bounds defensively and normalizes mapped `QRectF`
  before painting.
- It maps x directly and y directly with no flip, preserving tile/world
  orientation.
- It skips command painting when there is no latest frame, widget size is
  non-positive, or camera view width/height is zero; the background still paints
  normally.
- It iterates the command list in existing vector order with no Qt-side layer or
  order sorting.
- It draws only `RenderCommand2DType::Quad` commands as untextured flat
  rectangles.
- It ignores texture payloads and does not load or sample textures.
- It uses Qt-local hardcoded material-id debug colors for floor, wall, player,
  NPC actor, legacy NPC, and fallback materials.
- It is a temporary app-shell/debug-material renderer over existing latest-frame
  data, not the long-term renderer.
- Paint events do not execute frames, change pump timing, change input behavior,
  create render commands, add target highlighting from the drawer itself, persist
  viewport/camera/presentation data, or change runtime/product/scene/render
  command APIs.

Thin Qt product target highlight overlay complete:
- `ProductViewportWidget` accepts a const pointer to the latest transient
  `RuntimeGameplayProductInputFrameTargetContextResult` diagnostics.
- `buildMainSlot()` passes
  `hasLatestProductInputFrameTargetContext_ ? &latestProductInputFrameTargetContext_ : nullptr`
  into the viewport.
- The paint path draws existing render command quads first, then draws the
  target overlay afterward.
- The overlay draws only when latest frame exists, diagnostics pointer exists,
  diagnostics target has `hasTarget = true`, camera view bounds are
  non-degenerate, and widget size is positive.
- It uses copied diagnostics target payload only: target position and
  non-negative radius. It does not run target queries from paint.
- It builds a world-space marker from position plus/minus radius, maps it
  through the existing viewport world-to-pixel helper, and uses a Qt-local
  minimum marker for tiny or zero-radius targets.
- Reach is visual annotation only: reachable, unreachable, or no-reach choose
  different local styles; unreachable targets are still shown.
- The overlay is outline/tint only: no label, target id text, selected marker,
  trail, click animation, command execution, or richer overlay.
- It changes no runtime/product/scene/UI/render-command APIs and persists no
  target diagnostics, highlight state, viewport geometry, camera/presentation
  state, render-frame data, raw input, accumulator state, or Qt state.

Native scripted controls/debugger complete:
- `iggy_native_play --play PATH --scripted-controls LIST` runs comma-separated
  controls such as `east,east,south` or `right*3,wait`.
- `--scripted-control-interval-ms` controls delay between scripted controls.
- `--debug-scripted-controls` prints per-step product frame diagnostics.
- `--dump-final-state` prints final scripted state after the scripted sequence
  completes.
- `--expect-player-tiles 'x,y;x,y'` validates the player tile after each expanded
  scripted control.
- `--quit-after-script` exits after the scripted sequence completes.
- Scripted controls inject the same product input path as keyboard controls.
- Debug output includes before/after player tile, frame request/play mode/
  surface/loop statuses, input and ignored event counts, accepted/blocked/
  rejected counts, `npcMoved`, and render command count.
- Final-state dump includes player tile, next frame index, render command count,
  active input count, and held input count.
- Expectation mismatch exits nonzero and reports the actual player tile.
- This is native no-Qt app-shell tooling only: no gameplay semantics change,
  runtime/product API change, Qt path, persistence, or render asset/material/glTF
  policy.

Native scene draw-list extraction complete:
- `NativeSceneDrawList.hpp` defines `NativeSceneModelId`,
  `NativeSceneDrawItem`, `NativeSceneDrawListInput`, pure transform helpers, and
  `BuildNativeSceneDrawItems(...)` under `iggy::native_play`.
- Input is nullable `const runtime::RuntimeGameplayState *state` plus `seconds`.
- `IggyNativePlay.cpp` adapts `product_->play.state.loop.currentState` through
  `nativeSceneDrawState()` and calls
  `BuildNativeSceneDrawItems({ nativeSceneDrawState(), seconds })`; Vulkan
  command recording consumes the returned draw items as before.
- Null state or product state with no player keeps the fallback rotating player
  cube.
- Floor cubes are emitted for all map tiles in y/x order.
- Wall cubes are emitted only for non-walkable tiles through existing
  `tileAt(...)` behavior.
- Present modern NPC actors are emitted in registry order.
- Player is appended last.
- Transforms, tints, inclusion policy, and vector order are intended unchanged.
- This is a header-only extraction, so no CMake change is included.
- No Vulkan mesh-buffer ownership extraction, renderer class/swapchain/render
  pass/pipeline/shader module/command pool/command buffer/descriptor/buffer
  upload or destruction extraction, runtime/product/scene/server/render-command
  API change, CLI/debugger output change, SDL/input/scripted-control/free-play/
  gameplay stepping change, glTF/assets/textures/materials/animation/shader
  change, or intended visual behavior change is included.
- Vulkan mesh-buffer ownership remains a separate later packet; renderer/
  swapchain/pipeline/command-buffer extraction remains separate from mesh-buffer
  ownership and debugger CLI/output/docs changes.

Native product session extraction complete:
- `NativeProductSession.hpp/.cpp` defines app-local
  `iggy::native_play::NativeProductSession`.
- The session owns/delegates product load/play state, product input accumulator,
  active movement controls, latest product frame plus has flag, presentation
  camera plus has flag, scripted-control cadence/state, final dump state,
  movement guard, camera request config, and one-frame product request/tick flow.
- `IggyNativePlay.cpp` keeps app-shell responsibilities for
  `LaunchOptions`/CLI parsing/help, SDL key mapping/event loop/window lifecycle,
  and draw-list/camera orchestration; later native renderer skeleton extraction
  moves Vulkan lifetime/draw submission behind `NativeVulkanRenderer`.
- CMake registers `apps/native_play/NativeProductSession.cpp` for
  `iggy_native_play`.
- `NativeProductSession.*` is app-local and non-SDL/non-Vulkan; it does not use
  renderer ownership terms.
- `NativeProductSessionConfig` carries raw scripted-control specs.
- The session constructor loads the product scenario first, then parses
  scripted-control specs into session-owned controls, preserving pre-extraction
  side-effect/error order.
- `ParseArgs` still uses the shared parser for existing
  `--expect-player-tiles` count validation only.
- Output/error strings and normal scripted final-state behavior are intended
  unchanged.
- Verification passed in the source packet: `iggy_native_play` build, focused
  product CTest regex 10/10, valid native smoke with `multi_frame_guard_room.toml`
  scripted `east` and expected tile `5,1` printing
  `scripted final-state playerTile=5,1 nextFrameIndex=1 renderCommands=30 activeInputCount=0 heldInputCount=0`,
  and invalid-control smoke where `--scripted-controls nope` exits 1 after
  product load output and before `iggy_native_play: unknown scripted control: nope`.
- No runtime/product/scene/server/render-command API changes, gameplay/input/
  scripted-control semantic changes, CLI/debugger output string changes, SDL
  extraction, CLI parse/help extraction, `MapSdlKeyToProductControl` extraction,
  Vulkan setup/swapchain/render pass/pipeline/command buffer/buffer upload/
  destruction extraction, renderer class/skeleton extraction, mesh-buffer
  ownership change, `NativePlayMath.hpp` behavior change,
  `NativeSceneDrawList.hpp` behavior change, or glTF/assets/textures/material
  registry/animation/shader work is included.
- Renderer/swapchain/pipeline/command-buffer extraction and mesh-buffer
  ownership remain separate future packets.

Native Vulkan renderer skeleton extraction complete:
- `NativeVulkanRenderer.hpp/.cpp` defines app-local
  `iggy::native_play::NativeVulkanRenderer` with a minimal pimpl public surface.
- `NativeVulkanFrameInput` carries `Mat4 viewProjection` and a borrowed
  draw-item vector pointer.
- `NativeVulkanRenderer` exposes `initialize(SDL_Window *)`, `drawFrame(...)`,
  `markFramebufferResized()`, `aspectRatio()`, `waitIdle()`, and `cleanup()`.
- Vulkan lifetime/resources, swapchain, render pass, pipeline, depth,
  framebuffers, command pool, command buffers, sync, cube mesh, recording,
  acquire/submit/present, recreate, and cleanup code moved from
  `IggyNativePlay.cpp` to `NativeVulkanRenderer.cpp`.
- `IggyNativePlay.cpp` remains CLI/help/validation, MoltenVK fallback setup, SDL
  init/window/event loop/destruction/quit, SDL key mapping, product session
  calls, seconds/camera/draw-list orchestration, and per-frame renderer input
  owner.
- The renderer stores only a non-owning `SDL_Window *` for Vulkan interop and is
  not the SDL app shell.
- The renderer does not depend on `NativeProductSession`, runtime gameplay
  state, product loop/frame request, scripted controls, or
  `BuildNativeSceneDrawItems(...)`.
- Input lifetime is per-call: the app builds local draw items, passes a pointer
  through `NativeVulkanFrameInput`, and the renderer consumes it synchronously
  without storing it.
- The cube mesh moved mechanically into the renderer because command recording
  and draw submission moved; this is not a generalized mesh/resource
  abstraction.
- CMake registers `apps/native_play/NativeVulkanRenderer.cpp` for
  `iggy_native_play`.
- No debugger CLI/output string changes, SDL app-shell extraction beyond
  delegated renderer calls/window/resize/wait/cleanup, gameplay/product/session/
  input/scripted-control semantic changes, runtime/product/scene/server/
  render-command API changes, `NativeProductSession` dependency in the renderer,
  `NativePlayMath.hpp` behavior change, `NativeSceneDrawList.hpp` behavior
  change, shader behavior/interface change, new shaders, generalized mesh/
  resource registry, glTF/assets/textures/material registry/animation work,
  Linux/dGPU validation policy, or backend abstraction is included.
- App shell/CLI extraction, real static mesh loading, model files, materials/
  textures, descriptor/sampler policy, asset package discovery, authoring asset
  policy, Linux/dGPU validation, and backend abstraction remain separate future
  packets.

Native GPU mesh resource wrapper complete:
- `NativeVulkanRenderer.cpp` wraps the existing cube mesh GPU buffers in
  renderer-private `NativeVulkanBufferResource` and
  `NativeVulkanMeshResource` structs.
- `HasBuffer` and `HasMesh` centralize readiness checks for the existing cube
  mesh path.
- Buffer creation still uses caller-provided usage flags and memory properties;
  cube vertex/index upload remains host-visible/coherent.
- Cube data, all scene model ids mapping to the single cube mesh, draw order,
  shader interface, push constants, tint behavior, and `VK_INDEX_TYPE_UINT16`
  indexed draw parameters are preserved.
- Destruction is centralized through `destroyBuffer(...)` and
  `destroyMeshResource(...)`, preserving buffer-before-memory destruction,
  index-before-vertex cleanup, and reset-to-default idempotence.
- `NativeVulkanRenderer.hpp`, `IggyNativePlay.cpp`, CMake, shader files,
  runtime/product/scene APIs, and `NativeSceneDrawList.hpp` are unchanged.
- This is renderer-private ownership cleanup only: no public renderer API
  change, app-shell behavior change, mesh registry, model-slot binding, resource
  catalog, asset loader, new file IO policy, staging/device-local upload policy,
  glTF/assets/textures/materials/animation, or shader change is included.
- The pre-existing failure-path behavior where a throw between `vkCreateBuffer`
  and ownership assignment can leak a transient resource remains future RAII/
  allocation exception-safety work; it was not introduced by this packet.

Native pipeline/shader resource wrapper complete:
- `NativeVulkanRenderer.cpp` wraps the existing render pass, pipeline layout,
  graphics pipeline, and shader module handles in renderer-private
  `NativeVulkanShaderModuleResource` and `NativeVulkanPipelineResource`
  structs.
- `HasShaderModule` and `HasPipeline` centralize readiness checks for the
  existing shader module and pipeline handles.
- `createRenderPass()` now fills `pipeline_.renderPass`.
- `createShaderModule(...)` returns a wrapped shader module resource, and
  `destroyShaderModule(...)` centralizes shader module destruction and reset.
- `createGraphicsPipeline()` still reads `cube.vert.spv` and `cube.frag.spv`,
  uses the same shader stage setup, `pName = "main"`, vertex input, fixed
  pipeline state, push constant range, pipeline layout semantics, and render
  pass semantics.
- Command recording and draw code now use `pipeline_.renderPass`,
  `pipeline_.layout`, and `pipeline_.graphics`.
- `destroyPipelineResource(...)` centralizes graphics pipeline, pipeline layout,
  and render pass destruction in that order and resets the resource.
- `NativeVulkanRenderer.hpp`, `IggyNativePlay.cpp`, CMake, shader files,
  runtime/product/scene APIs, product session, and `NativeSceneDrawList.hpp` are
  unchanged.
- This is renderer-private ownership cleanup only: no public renderer API
  change, app-shell behavior change, CLI/debugger output change, shader
  interface/source change, new shader, descriptor/sampler/material/texture
  policy, model-slot binding, resource catalog, asset/glTF loader, staging/
  device-local upload policy, Linux/dGPU validation policy, or backend
  abstraction is included.

Native model-slot/cube fallback registry complete:
- `NativeVulkanRenderer.cpp` adds renderer-private `NativeVulkanModelSlot` and
  `NativeVulkanModelRegistry`.
- `NativeSceneModelId` now maps to renderer-private model slots, then model
  slots resolve to a mesh binding.
- The initial `createSceneMeshes()` registry bound `Floor`, `Wall`, `NpcActor`,
  and `Player` slots to the same cube fallback; later procedural mesh packets
  bound `Player` to the bean mesh and `NpcActor` to the NPC marker mesh; the
  later floor/wall asset binding packet supersedes the terrain slots when
  loaded assets validate.
- `meshForSceneModel(...)` delegates through `ModelSlotForSceneModel(...)` and
  `meshForModelSlot(...)`.
- Draw-item iteration and vector order remain unchanged.
- Preserved behavior includes the same `NativeSceneModelId`, mesh readiness
  behavior, cube mesh, draw parameters, shader pipeline, push constants, and
  tint behavior.
- This is renderer-private model-slot groundwork only: no public renderer API,
  `IggyNativePlay.cpp`, `NativeVulkanRenderer.hpp`, `NativeSceneDrawList.hpp`,
  CMake, shader, runtime/product/scene API, product session, test, CLI/debugger
  output, gameplay/product/session/input/scripted-control, descriptor/sampler,
  texture/material/asset/glTF, asset loader, model file IO, package discovery,
  resource catalog, authoring asset policy, staging/device-local upload,
  Linux/dGPU validation, or backend abstraction change is included.

Native static mesh asset data model complete:
- `NativeStaticMeshAsset.hpp` adds an app-local backend-free CPU mesh asset
  model.
- `NativeStaticMeshVertex` uses `Vec3 position` and
  `std::array<float, 3> color` to preserve the current vertex-color pipeline
  shape.
- `NativeStaticMeshAsset` stores vertices plus `std::uint16_t` indices.
- Inline validation covers non-empty vertices, non-empty indices, index range
  checks, and current `std::uint32_t` draw-count fit.
- `NativeCubeStaticMeshAsset()` preserves the previous cube positions, colors,
  and indices exactly.
- `NativeVulkanRenderer.cpp` consumes `NativeStaticMeshAsset` for cube upload.
- Vertex binding/attributes use `NativeStaticMeshVertex` while preserving two
  `vec3` shader inputs.
- Upload remains host-visible/coherent, and indexed draw remains
  `VK_INDEX_TYPE_UINT16`.
- Existing cube upload behavior remained unchanged in that packet; the cube
  fallback was still the `Floor`/`Wall` mesh at that stage while later
  procedural helpers used the same CPU mesh shape for player and NPC model
  slots. Later floor/wall loaded assets supersede those terrain slots when
  valid.
- This is no-loader static mesh data groundwork only: no `.cpp`, CMake, tests,
  public renderer API, app shell, product session, runtime/product/scene API,
  draw-list, shader, loader, file IO, glTF, material/texture/descriptor/sampler,
  resource catalog, staging/device-local upload, Linux/dGPU, backend abstraction,
  model slot binding behavior, CLI/debugger output, or gameplay/session/input/
  scripted-control change is included.
- Real file loading, glTF/static model parsing, asset registry/catalog,
  materials/textures/descriptors/samplers, model slot expansion beyond the
  procedural player/NPC meshes, staging/device-local upload, and shared render-
  server ownership remain future gates.

Native procedural bean mesh slot binding complete:
- `NativeStaticMeshAsset.hpp` adds `NativeBeanStaticMeshAsset()` as an in-memory
  procedural non-cube CPU mesh using the existing `NativeStaticMeshVertex`
  position/color shape and `std::uint16_t` indexed triangles.
- `NativeVulkanRenderer.cpp` creates a separate `playerMesh_` from that asset.
- `NativeVulkanModelSlot::Player` now binds to `playerMesh_`; `Floor` and
  `Wall` still used the cube fallback at that stage, and `NpcActor` is covered
  by the NPC marker binding below. Later floor/wall loaded assets supersede
  those terrain slots when valid.
- Upload remains host-visible/coherent through the existing mesh resource path.
- Vertex binding/attributes still expose the same two `vec3` shader inputs, and
  indexed draw remains `VK_INDEX_TYPE_UINT16`.
- Draw-item order, tints, camera, product/session behavior, CLI/debugger output,
  and public renderer API are unchanged.
- This is procedural in-memory mesh slot proof only: no loader, file IO, glTF/
  static model parsing, asset registry/catalog, material/texture/descriptor/
  sampler policy, shader change, staging/device-local upload, Linux/dGPU
  validation, backend abstraction, runtime/product/scene API, app shell, or
  gameplay behavior change is included.

Native procedural NPC mesh slot binding complete:
- `NativeStaticMeshAsset.hpp` adds `NativeNpcMarkerStaticMeshAsset()` as an
  in-memory procedural tapered marker CPU mesh using the existing
  `NativeStaticMeshVertex` position/color shape and `std::uint16_t` indexed
  triangles.
- `NativeVulkanRenderer.cpp` creates a separate `npcMesh_` from that asset.
- `NativeVulkanModelSlot::NpcActor` now binds to `npcMesh_`; `Player` stays
  bound to the bean mesh, and `Floor`/`Wall` still used the cube fallback at
  that stage. Later floor/wall loaded assets supersede those terrain slots when
  valid.
- Upload remains host-visible/coherent through the existing mesh resource path.
- Vertex binding/attributes still expose the same two `vec3` shader inputs, and
  indexed draw remains `VK_INDEX_TYPE_UINT16`.
- Draw-item order, tints, camera, product/session behavior, CLI/debugger output,
  and public renderer API are unchanged.
- This is procedural in-memory mesh slot proof only: no loader, file IO, glTF/
  static model parsing, asset registry/catalog, material/texture/descriptor/
  sampler policy, shader change, staging/device-local upload, Linux/dGPU
  validation, backend abstraction, runtime/product/scene API, app shell, or
  gameplay behavior change is included.

Native static mesh text loader complete:
- `NativeStaticMeshAssetLoader.hpp` adds an app-local header-only text loader
  for the existing `NativeStaticMeshAsset` CPU mesh shape.
- The text format supports comments, blank lines, `v x y z r g b` vertex
  records, and `tri i0 i1 i2` triangle records.
- `LoadNativeStaticMeshAssetText(...)` and
  `LoadNativeStaticMeshAssetFile(...)` return `NativeStaticMeshAssetLoadResult`
  with the parsed asset plus structured issues.
- Issue codes cover file-open failure, unknown directives, malformed vertices,
  malformed triangles, out-of-range indices, extra tokens, and invalid final
  meshes.
- `native_static_mesh_asset_loader_tests` covers valid text loading, valid file
  loading, missing files, unknown directives, malformed records, extra tokens,
  range failures, and invalid final mesh validation.
- CMake registers only the focused loader test; `iggy_native_play` behavior is
  unchanged.
- This is a minimal native text mesh loader only: no renderer slot binding to
  loaded files, CLI option, package discovery, glTF, asset registry/catalog,
  material/texture/descriptor/sampler policy, shader change, staging/device-
  local upload, runtime/product/scene API, app shell, or gameplay behavior
  change is included.

Native player mesh asset binding complete:
- `engine/apps/native_play/assets/player.igmesh` adds the first checked-in
  minimal text mesh asset.
- `iggy_native_play` now receives `IGGY_NATIVE_PLAY_ASSET_DIR` pointing at
  `apps/native_play/assets`.
- `NativeVulkanRenderer.cpp` loads `player.igmesh` through
  `LoadNativeStaticMeshAssetFile(...)` while creating scene meshes.
- A valid loaded asset becomes `playerMesh_` and remains bound to
  `NativeVulkanModelSlot::Player`.
- If loading fails or validates false, the existing procedural bean remains the
  silent fallback.
- `native_static_mesh_asset_loader_tests` now validates the checked-in player
  asset fixture.
- This binds one loaded text mesh to one renderer-private model slot only: no
  public renderer API, CLI option, package discovery, glTF/static model parsing,
  asset registry/catalog, material/texture/descriptor/sampler policy, shader
  change, staging/device-local upload, runtime/product/scene API, app shell
  behavior, CLI/debugger output, or gameplay behavior change is included.

Native NPC mesh asset binding complete:
- `engine/apps/native_play/assets/npc.igmesh` adds a checked-in minimal native
  NPC text mesh asset.
- `NativeVulkanRenderer.cpp` loads `npc.igmesh` through
  `LoadNativeStaticMeshAssetFile(...)` while creating scene meshes.
- A valid loaded asset becomes `npcMesh_` and remains bound to
  `NativeVulkanModelSlot::NpcActor`.
- If loading fails or validates false, the existing procedural NPC marker remains
  the silent fallback.
- `native_static_mesh_asset_loader_tests` now validates the checked-in NPC asset
  fixture.
- This binds one loaded text mesh to one renderer-private NPC model slot only:
  no public renderer API, CLI option, package discovery, glTF/static model
  parsing, asset registry/catalog, material/texture/descriptor/sampler policy,
  shader change, staging/device-local upload, runtime/product/scene API, app
  shell behavior, CLI/debugger output, or gameplay behavior change is included.

Native floor/wall mesh asset binding complete:
- `engine/apps/native_play/assets/floor.igmesh` and
  `engine/apps/native_play/assets/wall.igmesh` add checked-in native text mesh
  assets for the floor and wall slots.
- `NativeVulkanRenderer.cpp` loads both via
  `LoadNativeStaticMeshAssetFile(NativePlayAssetPath(...))` during scene mesh
  creation.
- `floorMesh_` and `wallMesh_` are created only for successful loaded assets.
- `NativeVulkanModelSlot::Floor` and `NativeVulkanModelSlot::Wall` register to
  those loaded meshes only when `HasMesh(...)` succeeds.
- If either file load fails or validates false, that slot reuses the existing
  `cubeMesh_` fallback; floor/wall fallback does not duplicate cube GPU mesh
  uploads.
- `native_static_mesh_asset_loader_tests` validates checked-in floor and wall
  fixture counts.
- Existing Player and NPC file bindings remain unchanged.
- This binds two loaded text mesh assets to renderer-private floor/wall slots
  only: no public renderer API, CMake, app-shell/CLI option, package discovery,
  glTF/static model parsing, asset registry/catalog, material/texture/
  descriptor/sampler policy, shader change, staging/device-local upload,
  Linux/dGPU policy, backend abstraction, runtime/product/scene/draw-list API,
  CLI/debugger output, or gameplay/session/input/scripted-control change is
  included.

Native static model slot policy complete:
- `NativeStaticModelPolicy.hpp` adds an app-local value-only policy under
  `engine/apps/native_play`.
- `NativeStaticModelSlot` covers `Floor`, `Wall`, `NpcActor`, and `Player`.
- `NativeStaticModelAssetRef` carries `{ slot, meshFilename }`.
- `NativeStaticModelPolicy` stores a `models` vector.
- `DefaultNativeStaticModelPolicy()` provides stable `.igmesh` filenames:
  `Floor -> floor.igmesh`, `Wall -> wall.igmesh`,
  `NpcActor -> npc.igmesh`, and `Player -> player.igmesh`.
- `FindNativeStaticModelAsset(...)` returns the first matching slot.
- The policy has no filesystem, file loading, parsing, GPU, or Vulkan
  knowledge.
- `NativeVulkanRenderer.cpp` consumes the policy only for filenames while
  preserving existing loaded `.igmesh` behavior and fallbacks.
- Focused policy tests cover stable default entries, lookup, missing slot,
  duplicate first-match behavior, and value-only filenames.
- This is not a glTF/glb parser, JSON/GLB parser dependency, shared render-server
  move, public renderer API change, app shell change, runtime/product/scene/
  server/draw-list API change, shader/material/texture/descriptor/sampler
  policy, staging/device-local upload policy, Linux/dGPU policy, backend
  abstraction, CLI/debugger output change, or gameplay/session/input/
  scripted-control change.
- The future glTF subset remains separately gated: one mesh, one primitive,
  triangles, required positions, optional vertex colors/default later, indexed
  `uint16` first, and no materials, textures, normals, UVs, animation, skins,
  scene graph, or transforms.

Native static model load report complete:
- `NativeStaticModelLoadReport.hpp` adds a backend-free app-local report builder
  over the existing value-only `NativeStaticModelPolicy` and existing `.igmesh`
  loader.
- Fixed report iteration covers `Floor`, `Wall`, `NpcActor`, and `Player`.
- Each slot entry records slot, mesh filename, load status, fallback kind, issue
  count, vertex count, and index count.
- Aggregate report counts track loaded, failed, and missing entries.
- Status values are `MissingPolicyRef`, `Loaded`, and `LoadFailed`.
- Report-only fallback mapping is `Floor -> Cube`, `Wall -> Cube`,
  `NpcActor -> ProceduralNpcMarker`, and `Player -> ProceduralBean`.
- The builder uses `FindNativeStaticModelAsset(policy, slot)` and
  `LoadNativeStaticMeshAssetFile(assetRoot / meshFilename)` only for explicit
  policy refs.
- Tests cover default checked-in assets and fixture counts (`floor` 4/6,
  `wall` 8/36, `npc` 7/30, `player` 6/24), missing policy refs, bad filename
  load failure, and no inference of unlisted assets.
- `NativeVulkanRenderer.cpp` was not touched; there is no renderer mutation or
  GPU/Vulkan/SDL behavior.
- This is not a glTF/glb parser, GLB binary parser, JSON parser, custom glTF
  subset parser, dependency fetch, package install, file discovery, package
  discovery, registry/catalog, model authoring policy, asset manifest, shared
  render-server move, public renderer API change, app shell change,
  runtime/product/scene/server/draw-list API change, shader/material/texture/
  descriptor/sampler policy, normals/UVs/animation/skins/scene graph/transforms
  work, staging/device-local upload policy, Linux/dGPU policy, backend
  abstraction, CLI/debugger output change, or gameplay/input/scripted-control
  change.

Native static model load report CLI dump complete:
- `iggy_native_play` now supports `--dump-static-model-load-report`.
- `LaunchOptions::dumpStaticModelLoadReport` plus parse/help text are wired in
  `IggyNativePlay.cpp`.
- Main dispatch builds
  `BuildNativeStaticModelLoadReport(DefaultNativeStaticModelPolicy(),
  IGGY_NATIVE_PLAY_ASSET_DIR)`, prints a compact stdout report, and exits before
  `NativeVulkanApp` construction/run.
- Exit code is 0 only when every fixed slot is loaded; it is nonzero if any
  fixed slot is missing or failed.
- The command does not require `--play`, scripted controls, SDL display
  availability, or Vulkan renderer initialization beyond normal binary linkage.
- Compact output starts with
  `static-model-load-report loaded=N failed=N missing=N`.
- Each slot row prints slot, filename or `<missing>`, status, fallback, vertices,
  indices, and issues.
- Current checked-in asset output includes Floor 4/6, Wall 8/36, NpcActor 7/30,
  and Player 6/24.
- Existing play/scripted/debug/final-state behavior and output are preserved
  except for the added help option.
- This is not a glTF/glb parser, GLB binary parser, JSON parser, custom glTF
  subset parser, dependency fetch, package install, web lookup, `.igmesh`
  schema change, file discovery, directory scanning, package discovery, asset
  registry/catalog, manifest expansion, model authoring policy,
  `NativeVulkanRenderer.cpp` change, public renderer API change,
  `NativeSceneDrawList.hpp` change, runtime/product/scene/server API change,
  gameplay/input/scripted-control semantic change, shader/material/texture/
  descriptor/sampler policy, normals/UVs/animation/skins/scene graph/transforms
  work, staging/device-local upload policy, Linux/dGPU policy, or backend
  abstraction.

Native static mesh text writer complete:
- `NativeStaticMeshAssetWriter.hpp` adds pure app-local text serialization.
- `WriteNativeStaticMeshAssetText(const NativeStaticMeshAsset &)` serializes the
  currently loaded `.igmesh` text format.
- Output is deterministic and newline-terminated:
  - fixed header comment `# Native static mesh asset`;
  - one `v x y z r g b` row per vertex in order;
  - one `tri a b c` row per three indices in order.
- Result/issue types are
  `NativeStaticMeshAssetWriteIssueCode::{InvalidMesh, NonTriangleIndexCount}`
  and `NativeStaticMeshAssetWriteResult::{text, issues, written()}`.
- The writer refuses non-serializable input without mutation or repair:
  invalid/empty meshes report `InvalidMesh` and no text, while valid indices
  with a non-multiple-of-three count report `NonTriangleIndexCount` and no text
  or `tri` rows.
- `IsNativeStaticMeshAssetValid(...)` semantics are unchanged.
- Tests cover deterministic triangle text plus reload, cube roundtrip
  representative data, procedural bean counts, empty invalid mesh,
  non-triangle index count, and deterministic repeated calls.
- This is not a glTF/glb/JSON parser, GLB binary parser, custom glTF subset
  parser, dependency fetch, package install, vendoring, web lookup, `.igmesh`
  schema expansion beyond serializing the current format, normals/UVs/materials/
  textures/descriptors/samplers/skins/animation/scene graph/transforms/metadata
  fields, file writing, file discovery, directory scanning, package discovery,
  registry/catalog, manifest expansion, authoring policy, renderer behavior
  change, `NativeVulkanRenderer.cpp` change, public renderer API change,
  draw-list/runtime/product/scene/server API change, app-shell/CLI change,
  gameplay/input/scripted-control change, or docs mixed into source.

Native static mesh fixture writer roundtrip complete:
- This is a test-only extension of `native_static_mesh_asset_writer_tests.cpp`.
- Writer roundtrip now covers checked-in renderer-bound `.igmesh` fixtures:
  `floor.igmesh` 4 vertices / 6 indices, `wall.igmesh` 8 / 36,
  `npc.igmesh` 7 / 30, and `player.igmesh` 6 / 24.
- Each fixture test loads with `LoadNativeStaticMeshAssetFile`, writes with
  `WriteNativeStaticMeshAssetText`, reloads with `LoadNativeStaticMeshAssetText`,
  writes again, and asserts canonical writer idempotence.
- Procedural NPC marker write/reload count coverage was added to pair with
  existing procedural bean coverage.
- CMake only adds `IGGY_NATIVE_PLAY_TEST_ASSET_DIR` to
  `native_static_mesh_asset_writer_tests`.
- No production source, renderer, app shell, asset fixture, CLI, shader,
  runtime/product/scene, or docs changes were in the source packet.
- This is not a glTF/glb/JSON parser, GLB/custom parser, dependency fetch,
  package install, vendoring, web lookup, `.igmesh` schema expansion or fixture
  rewrite, file writing/export CLI, normals/UVs/materials/textures/descriptors/
  samplers/skins/animation/scene graph/transforms/metadata fields, file
  discovery beyond explicit checked-in test filenames, directory scanning,
  package discovery, registry/catalog, manifest expansion, authoring package
  policy, renderer behavior change, `NativeVulkanRenderer.cpp` change, public
  renderer API change, draw-list/runtime/product/scene/server API change,
  app-shell/CLI change, gameplay/input/scripted-control change, or docs mixed
  into source.

Native static mesh built-in export CLI complete:
- `iggy_native_play` now supports `--dump-static-mesh-asset NAME`.
- Supported names are exactly `cube`, `bean`, and `npc-marker`.
- The command selects the existing built-in procedural mesh, serializes it
  through `WriteNativeStaticMeshAssetText(...)`, prints the writer's raw
  deterministic `.igmesh` text to stdout, and exits 0 before
  `NativeVulkanApp` construction, SDL initialization, or Vulkan launch.
- Unknown names fail nonzero with a compact error such as
  `iggy_native_play: unknown static mesh asset: nope`.
- Combining `--dump-static-model-load-report` with
  `--dump-static-mesh-asset` fails nonzero with a compact conflict error so
  stdout formats stay unambiguous.
- Existing help/report/scripted/final-state/product/session/render behavior is
  preserved except for the added help option.
- Sample output for `cube`, `bean`, and `npc-marker` starts with
  `# Native static mesh asset`; cube output includes cube vertex rows and a
  later `tri 0 1 2`, while bean and NPC marker output include their existing
  procedural vertex rows.
- This is not file writing, fixture rewriting, arbitrary asset path input,
  checked-in asset normalization, glTF/glb/JSON parsing, dependency work,
  schema expansion, material/texture/descriptor/sampler/normals/UV/animation/
  scene graph work, renderer behavior or API change, runtime/product/scene/
  server/draw-list API change, gameplay/scripted/final-state semantic change,
  or docs mixed into source.

Native static mesh export policy complete:
- `NativeStaticMeshExportPolicy.hpp` adds app-local value-only export metadata.
- `NativeStaticMeshBuiltInExportId` contains `Cube`, `Bean`, and `NpcMarker`.
- `NativeStaticMeshExportAssetRef` carries `id`, `name`, and
  `defaultFilename`; `NativeStaticMeshExportPolicy` carries `assets`.
- `DefaultNativeStaticMeshExportPolicy()` returns stable refs in order:
  `Cube` / `cube` / `cube.igmesh`, `Bean` / `bean` / `bean.igmesh`, and
  `NpcMarker` / `npc-marker` / `npc-marker.igmesh`.
- `FindNativeStaticMeshExportAsset(policy, name)` performs first-match lookup.
- `BuiltInNativeStaticMeshExportAsset(id)` maps ids to existing built-in CPU
  mesh assets.
- Existing `--dump-static-mesh-asset NAME` now resolves through the policy
  before writing raw `.igmesh`, preserving accepted names, unknown-name error,
  conflict behavior, and raw stdout output.
- Tests cover stable default order, exact names/filenames, basename-only
  filenames, lookup, missing-name null, duplicate first-match, and writer-valid
  built-in meshes.
- This is not file writing, `--output`, fixture rewrite/canonicalization,
  arbitrary asset path input, directory scanning, package discovery,
  registry/catalog/manifest expansion, source mutation, glTF/glb/JSON parser or
  dependency work, `.igmesh` schema expansion, material/texture/descriptor/
  sampler/normals/UV/animation/scene graph/metadata fields, renderer behavior,
  `NativeVulkanRenderer.cpp`, public renderer API, runtime/product/scene/server/
  draw-list API change, gameplay/scripted/final-state semantic change, or docs
  mixed into source.

Native static mesh output directory export CLI complete:
- `NativeStaticMeshFileExport.hpp` adds header-only app-local
  `ExportNativeStaticMeshAssetToDirectory(...)`.
- The helper validates policy lookup, existing output directory, directory type,
  target nonexistence, writer success, file open, and write success.
- The helper returns status/result data and does not print or throw for expected
  validation failures.
- `iggy_native_play --dump-static-mesh-asset NAME` now accepts optional
  `--output-dir DIR`.
- Without `--output-dir`, existing raw `.igmesh` stdout behavior is unchanged.
- With `--output-dir`, the CLI writes to `DIR/defaultFilename` from
  `NativeStaticMeshExportPolicy` and prints a compact status line such as
  `static-mesh-export name=cube output=/tmp/iggy-native-export-smoke/cube.igmesh bytes=523`.
- `--output-dir` requires `--dump-static-mesh-asset`; output-dir without dump
  fails with `iggy_native_play: --output-dir requires --dump-static-mesh-asset`.
- The existing `--dump-static-model-load-report` conflict remains:
  `iggy_native_play: --dump-static-model-load-report cannot be combined with --dump-static-mesh-asset`.
- Unknown asset failure remains
  `iggy_native_play: unknown static mesh asset: nope`.
- Existing target failure reports
  `iggy_native_play: static mesh export failed: TargetAlreadyExists output=/tmp/iggy-native-export-smoke/cube.igmesh issues=0`.
- Tests cover export to temp dir and reload, default policy exports, unknown
  asset, missing directory, file-not-directory output path, target already
  exists, and no parent directory creation.
- This is not arbitrary `--output PATH`, overwrite, `--force`, delete, rename,
  temp-file replacement, in-place canonicalization, checked-in fixture rewrites,
  production directory creation, package discovery, directory scanning,
  registry/catalog/manifest expansion, source mutation, glTF/glb/JSON parser or
  dependency work, `.igmesh` schema expansion, material/texture/descriptor/
  sampler/normals/UV/skins/animation/transforms/scene graph/metadata fields,
  renderer behavior, `NativeVulkanRenderer.cpp`, public renderer API, runtime/
  product/scene/server/draw-list API change, or gameplay/input/scripted-control/
  final-state semantic change.

Native static mesh built-in batch export CLI complete:
- `ExportNativeStaticMeshPolicyToDirectory(...)` exports every asset in
  `DefaultNativeStaticMeshExportPolicy()` to `DIR/defaultFilename`.
- The batch helper preflights output directory existence/type and all target
  filenames before writing, so common validation failures write no files.
- Batch result/entry structs carry aggregate status, output directory, exported
  count, total byte count, issue count, and per-asset file export results.
- `iggy_native_play --export-static-mesh-assets --output-dir DIR` writes
  `cube.igmesh`, `bean.igmesh`, and `npc-marker.igmesh`.
- Batch success prints compact status only, for example
  `static-mesh-export-batch output=/tmp/iggy-native-export-batch-smoke exported=3 bytes=33879`.
- Existing single-asset stdout and single-asset output-dir behavior remains
  unchanged.
- Batch requires `--output-dir`, cannot combine with
  `--dump-static-mesh-asset`, and cannot combine with
  `--dump-static-model-load-report`.
- Documented failures include:
  `iggy_native_play: --export-static-mesh-assets requires --output-dir`,
  `iggy_native_play: --export-static-mesh-assets cannot be combined with --dump-static-mesh-asset`,
  `iggy_native_play: --dump-static-model-load-report cannot be combined with --dump-static-mesh-asset`, and
  `iggy_native_play: --dump-static-model-load-report cannot be combined with --export-static-mesh-assets`.
- Re-running into existing output fails with
  `iggy_native_play: static mesh batch export failed: TargetAlreadyExists output=/tmp/iggy-native-export-batch-smoke/cube.igmesh issues=0`.
- This is not arbitrary `--output PATH`, overwrite/force/delete/rename,
  temp-file replacement, in-place canonicalization, checked-in fixture rewrite,
  production directory creation, package discovery/scanning, registry/catalog/
  manifest expansion, source mutation, glTF/glb/JSON parser/dependency work,
  `.igmesh` schema expansion, material/texture/descriptor/sampler/normals/UV/
  animation/scene graph/metadata fields, renderer behavior,
  `NativeVulkanRenderer.cpp`, public renderer API, runtime/product/scene/server/
  draw-list API change, or gameplay/scripted/final-state semantic change.

Native static mesh export report CLI complete:
- `NativeStaticMeshExportReport.hpp` adds header-only app-local
  `BuildNativeStaticMeshExportReport(...)` over
  `DefaultNativeStaticMeshExportPolicy()` or supplied policies.
- Report entries include export name, default filename, built-in id, writable
  status, issue count, vertex count, index count, and writer byte count.
- Aggregates include asset count, writable count, total bytes, and total issues.
- `iggy_native_play --dump-static-mesh-export-report` prints the report and
  exits before `NativeVulkanApp` construction or SDL/Vulkan startup.
- Sample output:
  `static-mesh-export-report assets=3 writable=3 bytes=33879 issues=0`;
  `asset=cube filename=cube.igmesh status=Writable vertices=8 indices=36 bytes=523 issues=0`;
  `asset=bean filename=bean.igmesh status=Writable vertices=234 indices=1296 bytes=23882 issues=0`;
  `asset=npc-marker filename=npc-marker.igmesh status=Writable vertices=98 indices=504 bytes=9474 issues=0`.
- Conflict outputs are:
  `iggy_native_play: --dump-static-mesh-export-report cannot be combined with --output-dir`,
  `iggy_native_play: --dump-static-mesh-export-report cannot be combined with --export-static-mesh-assets`,
  `iggy_native_play: --dump-static-mesh-export-report cannot be combined with --dump-static-mesh-asset`, and
  `iggy_native_play: --dump-static-mesh-export-report cannot be combined with --dump-static-model-load-report`.
- The report path does not write files, validate output directories/paths, touch
  renderer behavior, or inspect the filesystem.
- This is not arbitrary `--output PATH`, overwrite/force/create-directory
  policy, checked-in fixture rewrite/canonicalization, package discovery/
  scanning, registry/catalog/manifest expansion, source mutation, glTF/glb/JSON
  parser/dependency work, `.igmesh` schema expansion, material/texture/
  descriptor/sampler/normals/UV/animation/scene graph/metadata fields,
  renderer behavior, `NativeVulkanRenderer.cpp`, public renderer API, runtime/
  product/scene/server/draw-list API change, or gameplay/input/scripted-control/
  final-state semantic change.

Native static mesh export policy validation complete:
- `ValidateNativeStaticMeshExportPolicy(...)` adds backend-free and filesystem-
  free validation in `NativeStaticMeshExportPolicy.hpp`.
- Structured issue codes are
  `NativeStaticMeshExportPolicyValidationIssueCode::{EmptyName, DuplicateName, EmptyDefaultFilename, DefaultFilenameContainsSeparator, DuplicateDefaultFilename}`.
- `NativeStaticMeshExportPolicyValidationIssue` records issue details, and
  `NativeStaticMeshExportPolicyValidationResult::valid()` reports clean policy
  status.
- The default export policy remains unchanged: `cube` / `cube.igmesh`, `bean` /
  `bean.igmesh`, and `npc-marker` / `npc-marker.igmesh`.
- `NativeStaticMeshFileExport.hpp` validates supplied policies before single or
  batch file export performs asset lookup, directory checks, target preflight,
  writer work, or writes.
- `NativeStaticMeshFileExportStatus::InvalidPolicy` plus native CLI status-name
  text now covers invalid policies.
- Invalid single export and invalid batch export both return `InvalidPolicy`
  with issue counts and write no files.
- Valid default export report and valid default batch export output remain
  unchanged.
- This docs packet does not change source, tests, CMake, assets, shaders,
  runtime, package/export manifests, sidecar output, package discovery,
  registry/catalog/manifest expansion, source mutation, authoring package
  policy, checked-in fixture canonicalization, overwrite/force/create-directory/
  temp replacement policy, renderer loading cleanup, `NativeVulkanRenderer.cpp`,
  glTF/glb/JSON parser/dependency work, `.igmesh` schema/writer/loader/report
  byte math, valid default CLI output, gameplay/scripted/final-state behavior,
  or Linux/dGPU validation.

Native static mesh export manifest text builder complete:
- `NativeStaticMeshExportManifest.hpp` adds header-only app-local
  `BuildNativeStaticMeshExportManifestText(...)`.
- The builder validates the supplied `NativeStaticMeshExportPolicy`, then uses
  `BuildNativeStaticMeshExportReport(...)` for stable asset counts and writer
  byte counts.
- `NativeStaticMeshExportManifestResult` reports `Built`, `InvalidPolicy`, or
  `WriterFailed`.
- `iggy_native_play --dump-static-mesh-export-manifest` prints deterministic
  manifest text to stdout and exits before `NativeVulkanApp` construction or
  SDL/Vulkan startup.
- Sample output:
  `static-mesh-export-manifest version=1 assets=3 bytes=33879`;
  `asset=cube filename=cube.igmesh vertices=8 indices=36 bytes=523`;
  `asset=bean filename=bean.igmesh vertices=234 indices=1296 bytes=23882`;
  `asset=npc-marker filename=npc-marker.igmesh vertices=98 indices=504 bytes=9474`.
- The manifest mode conflicts with `--output-dir`,
  `--export-static-mesh-assets`, `--dump-static-mesh-export-report`,
  `--dump-static-mesh-asset`, and `--dump-static-model-load-report`.
- Existing valid export report and batch export output remain unchanged.
- This docs packet does not change source, tests, CMake, assets, shaders,
  runtime, sidecar file writes, package/export manifest files on disk,
  overwrite/create-directory/temp-file policy, arbitrary output paths, checked-
  in fixture canonicalization/rewrite, package discovery, directory scanning,
  registry/catalog expansion, source mutation, material/texture/schema changes,
  JSON/glTF/glb parser/dependency work, renderer behavior,
  `NativeVulkanRenderer.cpp`, native app CMake source registration, gameplay/
  scripted/final-state behavior, or docs mixed into source.

Native static mesh batch manifest sidecar export complete:
- Batch export now writes policy mesh files plus one manifest sidecar.
- The sidecar filename is exactly `static-mesh-export-manifest.txt`.
- The sidecar content is exactly
  `BuildNativeStaticMeshExportManifestText(policy).text`.
- Batch export preflights the sidecar target before mesh writes and applies the
  same no-overwrite `TargetAlreadyExists` policy.
- `NativeStaticMeshFileExportBatchResult` now reports `manifestOutputPath` and
  `manifestByteCount`.
- CLI batch success output now includes stable `manifest=...` and
  `manifestBytes=...` fields, for example
  `static-mesh-export-batch output=/tmp/iggy-native-sidecar-smoke-78 exported=3 bytes=33879 manifest=/tmp/iggy-native-sidecar-smoke-78/static-mesh-export-manifest.txt manifestBytes=272`.
- Manifest sidecar content has the same deterministic shape as the dump command:
  `static-mesh-export-manifest version=1 assets=3 bytes=33879`;
  `asset=cube filename=cube.igmesh vertices=8 indices=36 bytes=523`;
  `asset=bean filename=bean.igmesh vertices=234 indices=1296 bytes=23882`;
  `asset=npc-marker filename=npc-marker.igmesh vertices=98 indices=504 bytes=9474`.
- Existing sidecar targets fail before mesh writes with compact output such as
  `iggy_native_play: static mesh batch export failed: TargetAlreadyExists output=/tmp/iggy-native-sidecar-existing-78/static-mesh-export-manifest.txt issues=0`.
- Single-asset stdout and single-asset output-dir export remain unchanged; single
  output-dir export writes no sidecar.
- This docs packet does not change source, tests, CMake, assets, shaders,
  runtime, single-export sidecar behavior, package discovery/scanning, registry/
  catalog expansion, package semantics, source mutation, overwrite/force/create-
  directory/temp replacement, checked-in fixture rewrites/canonicalization,
  renderer behavior, `NativeVulkanRenderer.cpp`, JSON/glTF/glb parser/dependency
  work, `.igmesh` schema expansion, materials/textures/normals/UVs/animation/
  schema work, gameplay/scripted/final-state behavior, or next research/scout
  source implementation.

Native static mesh export directory verification CLI complete:
- Added read-only `VerifyNativeStaticMeshExportDirectory(policy, directory)`.
- The verifier validates the supplied export policy, requires an existing output
  directory, compares `static-mesh-export-manifest.txt` exactly to
  `BuildNativeStaticMeshExportManifestText(policy).text`, then loads only
  expected policy files and checks vertex/index counts against the export report.
- Extra unrelated files are ignored; no directory scanning/discovery semantics
  are introduced.
- Added `iggy_native_play --verify-static-mesh-export --output-dir DIR`, exiting
  before `NativeVulkanApp`, SDL, or Vulkan startup.
- Success output shape:
  `static-mesh-export-verify output=/tmp/iggy-native-verify-smoke-79 verified=3 manifest=ok`.
- Missing manifest failure shape:
  `iggy_native_play: static mesh export verification failed: MissingManifest output=/tmp/iggy-native-verify-missing-79/static-mesh-export-manifest.txt issues=0`.
- Single-export directories fail with the same `MissingManifest` shape because
  single output-dir export writes no sidecar.
- Verify conflicts with `--export-static-mesh-assets`,
  `--dump-static-mesh-asset`, `--dump-static-mesh-export-report`,
  `--dump-static-mesh-export-manifest`, and
  `--dump-static-model-load-report`.
- This docs packet does not change source, tests, CMake, assets, shaders,
  runtime, directory scanning/discovery beyond expected policy files, package
  discovery, registry/catalog/package semantics, source mutation, overwrite/
  force/create-directory/temp replacement policy, arbitrary output paths beyond
  existing `--output-dir`, fixture rewrites/canonicalization, renderer behavior,
  `NativeVulkanRenderer.cpp`, JSON/glTF/glb parser/dependency work, `.igmesh`
  schema/material/texture/normal/UV/animation behavior, gameplay/scripted/
  final-state behavior, or next research/scout source implementation.

Native static mesh export verification report CLI complete:
- Added a read-only report builder around
  `VerifyNativeStaticMeshExportDirectory(policy, directory)`.
- Added `iggy_native_play --dump-static-mesh-export-verification-report
  --output-dir DIR`, exiting before `NativeVulkanApp`, SDL, or Vulkan startup.
- Success prints a summary plus per-asset rows for the existing default export
  policy:
  `static-mesh-export-verification-report status=Verified output=/tmp/iggy-native-verify-report-smoke-80.PBsYB2 verified=3 issues=0`;
  `asset=cube filename=cube.igmesh status=Verified vertices=8 expectedVertices=8 indices=36 expectedIndices=36 issues=0`;
  `asset=bean filename=bean.igmesh status=Verified vertices=234 expectedVertices=234 indices=1296 expectedIndices=1296 issues=0`;
  `asset=npc-marker filename=npc-marker.igmesh status=Verified vertices=98 expectedVertices=98 indices=504 expectedIndices=504 issues=0`.
- Failed verification prints the report, then returns nonzero through the
  existing compact `iggy_native_play:` error style. Missing manifest output
  includes
  `static-mesh-export-verification-report status=MissingManifest output=/tmp/iggy-native-verify-report-missing-80.EG7aZh verified=0 issues=0 problem=/tmp/iggy-native-verify-report-missing-80.EG7aZh/static-mesh-export-manifest.txt`
  followed by
  `iggy_native_play: static mesh export verification report failed: MissingManifest output=/tmp/iggy-native-verify-report-missing-80.EG7aZh/static-mesh-export-manifest.txt issues=0`.
- Existing `--verify-static-mesh-export` output and behavior are preserved.
- Report mode conflicts with `--verify-static-mesh-export`,
  `--export-static-mesh-assets`, `--dump-static-mesh-asset`,
  `--dump-static-mesh-export-report`, `--dump-static-mesh-export-manifest`, and
  `--dump-static-model-load-report`, and requires `--output-dir`.
- This docs packet does not change source, tests, CMake, assets, shaders,
  runtime, `NativeVulkanRenderer.cpp`, renderer behavior, shader behavior, asset
  fixtures, runtime/product/scene/server APIs, docs-in-source, native app CMake
  source registration, writes/repair/scanning/package/catalog/parser/schema/
  material/texture behavior, gameplay/scripted/final-state behavior, or next
  research/scout source implementation.

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

Product input target context projection complete:
- `RuntimeGameplayProductInputTargetContext` starts from a copied
  `PlayerInputBindingContext2D` and an already-computed
  `RuntimeGameplayProductInteractionTargetQueryResult`.
- Valid `TargetFound` reports with `hasTarget = true` and a non-empty target id
  set `hasHoveredTargetId`, replace `hoveredTargetId`, copy the diagnostic
  hovered id, and return `TargetProjected`.
- `NotLoaded`, `MissingQuery`, `TargetNotFound`, invalid `TargetFound`, and empty
  target ids return `Unchanged` with the base context copied exactly.
- Unchanged paths preserve existing hover, selected target, current player tile,
  and input gates; valid found paths replace only hover.
- The helper never sets or clears selected target fields and does not require
  `reachable`.
- It does not mutate query reports, product/gameplay/runtime interaction state,
  input frames, accumulator state, binding input, frame request/play surface,
  input adapter behavior, `RuntimeGameplayProductInputContext::build(state)`, or
  command/effect execution.

Product input frame target context enrichment complete:
- `RuntimeGameplayProductInputFrameTargetContext` is an opt-in runtime/product
  pre-frame helper between accumulator output and frame request.
- Input carries product play mode state, product input frame,
  `InteractionTargetSpatialQuery2DConfig`, and `InteractionReach2DConfig`.
- Result carries `Unchanged`, `NoEligiblePrimaryTile`, or `TargetProjected`,
  copied/enriched frame, primary tile diagnostics, nested product target query
  result, and nested input target context result.
- The helper starts by copying the input frame and scanning events in order for
  eligible `PrimaryTile` pressed events with `hasTile = true`.
- The latest eligible `PrimaryTile` wins; event index and tile are recorded.
- Missing tile payloads and `PrimaryTile` releases are ineligible and preserved
  for existing adapter behavior.
- The chosen tile queries `RuntimeGameplayProductInteractionTargetQuery` as
  `TileCenter` with forwarded spatial/reach configs.
- The helper applies `RuntimeGameplayProductInputTargetContext` to the input
  frame's base binding context and the query result.
- It replaces only the copied frame binding context; all events are preserved
  unchanged and in order.
- It returns `TargetProjected` only when target-context projection succeeds;
  otherwise it returns `Unchanged` while preserving diagnostics.
- Reach is diagnostic only and not a projection gate.
- It does not synthesize `Interact`/`Inspect`, inject target ids into events,
  change `PrimaryTile` semantics, mutate input frame/play state/accumulator/
  product/gameplay state/query result/base context, or change adapter,
  command/effect, frame request, or play-surface behavior.

Qt product input frame target context consumer complete:
- `IggyQtShellWindow::runProductFrameRequestOnce()` is the thin Qt caller of
  `RuntimeGameplayProductInputFrameTargetContext`.
- Manual `Product Step` and Qt `Product Frame Pump` both use it because both
  already share that one-frame helper.
- Qt calls `RuntimeGameplayProductInputFrameTargetContext {}.enrich(...)` after
  accumulator `buildFrame(...)` and after `productInputAccumulator_ = frame.state`,
  but before `RuntimeGameplayProductFrameRequest {}.run(input)`.
- The call uses `productPlayState_`, `frame.frame`, and default spatial/reach
  configs.
- Qt passes `enrichedFrame.frame` to
  `RuntimeGameplayProductFrameRequestInput::inputFrame`.
- Accumulator one-shot drain and held-control behavior are unchanged because
  `productInputAccumulator_ = frame.state` remains before the frame request.
- On `NoEligiblePrimaryTile` or `Unchanged`, the helper returns a copied unchanged
  frame and Qt passes that frame without branching.
- Qt stores the latest transient helper result, sets the has flag, wires
  `context_.latestProductInputFrameTargetContext` to the stable member, and
  exposes compact read-only product panel rows.
- Product Input Focus disable clears accumulator state plus latest target-context
  diagnostics/pointer; enabling focus does not synthesize diagnostics.
- Widget/body refreshes do not clear the member; the pointer remains stable
  through rebuilds.
- No diagnostics are exposed in settings, logs, or status text.
- Product Input Focus gating, mouse event handling, keyboard mapping, pump
  timing, and manual Step availability remain unchanged.
- Qt does not own target lookup semantics and introduces no Qt types into
  runtime/product APIs.

Product input target-context diagnostics projection complete:
- `UiFeatureContext` carries a const diagnostics pointer, but
  `uiFeatureContextHasProductPlayMode(...)` does not include it, so diagnostics
  alone do not create product play context.
- `UiRuntimeWorkspaceModel` passes diagnostics into the product panel only when
  product play context is already present.
- `UiProductPlayModePanelModel` adds compact `targetContext` rows and does not
  require latest-frame rows.
- No diagnostics pointer produces no target-context rows.
- Rows are source-shaped and compact: `targetContext.status`,
  `targetContext.hasPrimaryTileEvent`, conditional
  `targetContext.primaryTileEventIndex`, conditional `targetContext.primaryTile`,
  `targetContext.target.status`, `targetContext.target.hasTarget`, conditional
  `targetContext.target.targetId`, `targetContext.target.hasPlayer`,
  `targetContext.target.hasReach`, conditional `targetContext.target.reachable`,
  and `targetContext.projection.status`.
- The projection does not dump copied frames, all events, full target payloads,
  or nested structs.

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
  context, stores the returned accumulator state, enriches a copied frame with
  `RuntimeGameplayProductInputFrameTargetContext`, and builds
  `RuntimeGameplayProductFrameRequestInput` from current `productPlayState_`,
  enriched frame output, and shell-owned camera config.
- It calls `RuntimeGameplayProductFrameRequest {}.run(input)` exactly once.
- It updates only replaceable app-shell transient state: `productPlayState_`,
  `latestProductPlayModeFrame_` plus the stable product play panel context
  pointer, and `productPresentationCamera_` for the next request.
- It preserves held movement across steps and drains one-shots after each
  executed Step; target-context enrichment happens after accumulator state is
  stored, so accumulator behavior is unchanged.
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
- Should frame request/play surface ever own target-context enrichment, or should
  it remain an app-shell pre-frame caller decision?
- Should target-context diagnostics grow beyond compact panel rows and the thin
  Qt target marker into richer overlays/labels, logs, status text, or deeper
  inspection?
- Who owns hover lifecycle, clearing, and selected-target workflows beyond this
  enrichment-only helper?
- Should explicit interact target synthesis consume a `TargetFound` report, and
  should it require `reachable=true`?
- Should a later point-vs-tile policy add `PrimaryPoint` or world-point payloads
  beyond the current `PrimaryTile`-only click behavior?
- Should selected target projection be added later, and what hover lifecycle or
  ownership gate would be required around the current enrichment helper?
- What GPU resource ownership, renderer expansion, textured sprite/animation/
  material/asset policy, or canvas polish should replace or extend the current
  debug/material actor quads and temporary Qt drawer?
- Who owns pause/retry/reset?
- What state must save/load for the first playable slice?
- What does completion/failure mean in the first slice?

Output:
- one optional richer diagnostics, overlays/labels, or frame request/play-surface
  ownership packet over the product query/context helpers, if approved;
- one GPU resource wrapper, renderer expansion, or textured sprite / animation /
  material policy packet, if approved;
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
- do not treat Qt `productViewport_` as runtime render semantics, target
  discovery, runtime truth, or readiness; do not add event handling beyond the
  approved viewport-only focused ready left-click `PrimaryTile` path, synthesize
  `PrimaryPoint`, attach world-point payloads, draw anything beyond existing
  latest-frame quad render commands in the Qt drawer, add canvas polish, or
  persist viewport geometry/state in runtime/session/gameplay/product-loop/
  play-mode saves, settings, snapshots, or scene/UI model truth;
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
product gameplay actor debug/material quad projection integrated; thin Qt
product viewport render command drawer integrated as temporary latest-frame quad
drawing; thin Qt target highlight overlay integrated as a visual annotation over
latest diagnostics; textured sprites/animation/material/asset policy, richer
overlays/labels, other model-slot file binding, glTF/glb parsing under the future
constrained subset, asset registry/catalog, materials/textures/descriptors/samplers,
additional model slot expansion, package/authoring asset policy, renderer
expansion, backend validation, interaction execution, product UX/save semantics,
and UI execution beyond launch/focus/input capture/manual step/pump/viewport
ownership/primary-tile mouse input remain separate gates.

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
     event/render target boundary.
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
20. Product input target context projection. Complete:
   - runtime/product enrichment helper that projects a valid found query target
     into transient hovered binding context while preserving base selected target,
     unchanged hover, current player tile, and gates; no lifecycle, execution,
     persistence, or wiring change.
21. Product input frame target context enrichment. Complete:
   - runtime/product opt-in pre-frame helper that finds the latest eligible
     `PrimaryTile` pressed event, queries target/reach, enriches only copied frame
     binding context, and preserves events; no auto frame request/play-surface
     wiring or `PrimaryTile` to `Interact` conversion.
22. Qt product input frame target context consumer. Complete:
   - shared Qt one-frame path calls the opt-in helper after accumulator
     `buildFrame(...)` / state storage and before frame request, passes the
     enriched frame forward, and stores/exposes only compact target-context rows.
23. Product input target-context diagnostics projection. Complete:
   - Qt stores the latest transient helper result, wires a stable const pointer
     into product play UI context, and `UiProductPlayModePanelModel` projects
     compact `targetContext` rows only when product play context exists.
24. Thin Qt product viewport render command drawer. Complete:
   - `QFrame#productViewport` paints existing latest-frame quad render commands
     as temporary untextured debug/material rectangles.
25. Thin Qt product target highlight overlay. Complete:
   - `ProductViewportWidget` paints a Qt-only outline/tint marker from latest
     target-context diagnostics after command quads.
26. Native scripted controls/debugger. Complete:
   - `iggy_native_play` can inject scripted controls through the same product
     input path as keyboard controls, print per-step diagnostics, validate
     expected player tiles, dump final scripted state, and quit after the script.
27. Native scene draw-list extraction. Complete:
   - `NativeSceneDrawList.hpp` builds backend-neutral app-local draw items from
     nullable product runtime state plus seconds; Vulkan command recording
     consumes the returned items as before.
28. Native product session extraction. Complete:
   - `NativeProductSession` owns app-local product load/play/input/script/final
     dump/session tick flow while `IggyNativePlay.cpp` keeps SDL, CLI/help, and
     draw-list/camera orchestration responsibilities.
29. Native Vulkan renderer skeleton extraction. Complete:
   - `NativeVulkanRenderer` owns app-local Vulkan lifetime/swapchain/render
     pass/pipeline/depth/framebuffer/command/sync/cube-mesh recording and
     cleanup behind a pimpl surface while `IggyNativePlay.cpp` remains CLI/help,
     SDL app shell, product session, seconds/camera/draw-list orchestration, and
     per-frame renderer input owner.
30. Native GPU mesh resource wrapper. Complete:
   - `NativeVulkanRenderer.cpp` wraps the existing cube mesh vertex/index GPU
     buffers in renderer-private resource structs, centralizes readiness and
     idempotent destruction, and preserves host-visible/coherent upload, cube
     data, model mapping, draw parameters, shader interface, and public renderer
     API.
31. Native pipeline/shader resource wrapper. Complete:
   - `NativeVulkanRenderer.cpp` wraps render pass, pipeline layout, graphics
     pipeline, and shader module handles in renderer-private resource structs,
     centralizes shader module and pipeline destruction/reset, and preserves
     shader filenames, shader interface/source, fixed pipeline state, render pass
     semantics, command recording, draw behavior, and public renderer API.
32. Native model-slot/cube fallback registry. Complete:
   - `NativeVulkanRenderer.cpp` maps `NativeSceneModelId` through renderer-private
     model slots to the existing cube fallback mesh while preserving draw-item
     order, cube mesh behavior, shader pipeline, push constants, and tinting.
33. Native static mesh asset data model. Complete:
   - `NativeStaticMeshAsset.hpp` defines backend-free CPU static mesh data and
     the cube asset; `NativeVulkanRenderer.cpp` consumes it for the existing cube
     fallback without adding loaders, file IO, materials, or slot binding
     changes.
34. Debug overlay projection:
   - trace/final rows, AI map, collision, path, interactions, inventory.
35. UI presentation adapter:
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
31. Runtime/product input target context projection is integrated.
32. Runtime/product input frame target context enrichment is integrated.
33. Qt product input frame target context consumer is integrated.
34. Product input target-context diagnostics projection is integrated.
35. Thin Qt product viewport render command drawer is integrated.
36. Thin Qt product target highlight overlay is integrated.
37. Native no-Qt scripted controls/debugger is integrated.
38. Native scene draw-list extraction is integrated.
39. Native product session extraction is integrated.
40. Native Vulkan renderer skeleton extraction is integrated.
41. Native GPU mesh resource wrapper is integrated.
42. Native pipeline/shader resource wrapper is integrated.
43. Native model-slot/cube fallback registry is integrated.
44. Native static mesh asset data model is integrated.
45. Native procedural bean mesh slot binding is integrated.
46. Native procedural NPC mesh slot binding is integrated.
47. Native static mesh text loader is integrated.
48. Native player mesh asset binding is integrated.
49. Native NPC mesh asset binding is integrated.
50. Native floor/wall mesh asset binding is integrated.
51. Native static model slot policy is integrated.
52. Native static model load report is integrated.
53. Native static model load report CLI dump is integrated.
54. Native static mesh text writer is integrated.
55. Native static mesh fixture writer roundtrip tests are integrated.
56. Native static mesh built-in export CLI is integrated.
57. Native static mesh export policy is integrated.
58. Native static mesh output directory export CLI is integrated.
59. Native static mesh built-in batch export CLI is integrated.
60. Native static mesh export report CLI is integrated.
61. Native static mesh export policy validation is integrated.
62. Native static mesh export manifest text builder CLI is integrated.
63. Native static mesh batch manifest sidecar export is integrated.
64. Native static mesh export directory verification CLI is integrated.
65. Native static mesh export verification report CLI is integrated.
66. Dispatch richer diagnostics display, overlays/labels, frame request/
    play-surface ownership, explicit interact target synthesis, reach-gated
    interaction execution, hover lifecycle, selected-target workflow,
    point-vs-tile policy, other model-slot file binding, glTF/glb parsing under
    the constrained subset, asset registry/catalog, materials/textures/descriptors/
    samplers, non-cube model slot expansion, package/authoring asset policy,
    renderer expansion, render projection gaps, backend validation, further
    input mapping, or a focused-input follow-up, depending on planner scope.

Do not broaden the next Product Loop packet into pause/retry/reset,
completion/failure, save/load productization, product-loop signature changes,
command/gate execution, presentation state persistence, Qt/UI/CLI behavior,
raw OS event types, raw input persistence, textured sprite/animation/material
policy, target context wiring/lifecycle/reach beyond the approved read-only
product query/enrichment/diagnostics helpers, automatic frame request/play-surface
ownership of enrichment, diagnostics persistence or exposure beyond compact
read-only panel rows and thin target marker, additional Qt mouse behavior, render
command drawing beyond the approved Qt latest-frame drawer, canvas polish, or new
gameplay semantics unless the user explicitly reprioritizes.
Do not treat native scripted controls/debugger as gameplay semantics, runtime/
product API, Qt path, persistence, or render asset/material/glTF policy.
Do not treat native scene draw-list extraction as Vulkan mesh-buffer ownership,
renderer/swapchain/pipeline/command-buffer extraction, runtime/product/scene/
server/render-command API change, SDL/input/scripted-control/free-play/gameplay
stepping change, CLI/debugger docs/output change, or glTF/assets/textures/
materials/animation/shader policy.
Do not treat native product session extraction as runtime/product/scene/server/
render-command API change, gameplay/input/scripted-control semantic change,
CLI/debugger output string change, SDL extraction, CLI parse/help extraction,
`MapSdlKeyToProductControl` extraction, Vulkan setup/swapchain/render pass/
pipeline/command buffer/buffer upload/destruction extraction, renderer
class/skeleton extraction, mesh-buffer ownership change, `NativePlayMath.hpp` or
`NativeSceneDrawList.hpp` behavior change, or glTF/assets/textures/material
registry/animation/shader work.
Do not treat native Vulkan renderer skeleton extraction as debugger CLI/output
string changes, SDL app-shell extraction beyond delegated renderer calls/window/
resize/wait/cleanup, SDL init/window/event-loop ownership transfer,
gameplay/product/session/input/scripted-control semantic changes, runtime/
product/scene/server/render-command API changes, renderer dependency on
`NativeProductSession` or runtime gameplay state, `NativePlayMath.hpp` behavior
change, `NativeSceneDrawList.hpp` behavior change, shader behavior/interface
change, generalized mesh/resource registry, glTF/assets/textures/material
registry/animation work, Linux/dGPU validation policy, or backend abstraction.
Do not treat native GPU mesh resource wrapping as public renderer API change,
`IggyNativePlay.cpp` app-shell behavior change, CLI/debugger output change, SDL
app-shell extraction, gameplay/product/session/input/scripted-control semantic
change, runtime/product/scene/server/render-command API change,
`NativeSceneDrawList.hpp` model-id or draw-order change, shader interface/source
change, new shader, glTF/assets/textures/materials/animation, mesh registry,
model-slot binding, resource catalog, resource handles, asset loader, new file
IO policy, staging buffer/device-local upload policy, Linux/dGPU validation
policy, or backend abstraction.
Do not treat native pipeline/shader resource wrapping as public renderer API
change, `IggyNativePlay.cpp` app-shell behavior change, CLI/debugger output
change, SDL app-shell extraction, gameplay/product/session/input/scripted-
control semantic change, runtime/product/scene/server/render-command API change,
`NativeSceneDrawList.hpp` model-id or draw-order change, shader interface/source
change, new shader, descriptor/sampler/material/texture policy, model-slot
binding, resource catalog, asset/glTF loader, new file IO policy, staging
buffer/device-local upload policy, Linux/dGPU validation policy, or backend
abstraction.
Do not treat native model-slot/cube fallback registry as public renderer API
change, `IggyNativePlay.cpp` app-shell behavior change,
`NativeVulkanRenderer.hpp` change, `NativeSceneDrawList.hpp` model-id or
draw-order change, CMake change, shader source/interface change, new shader,
runtime/product/scene API change, product session change, test change,
CLI/debugger output change, gameplay/product/session/input/scripted-control
change, descriptor/sampler work, textures/materials/assets/glTF, asset loader,
model file IO, package discovery, resource catalog, authoring asset policy,
staging/device-local upload, Linux/dGPU validation, or backend abstraction.
Do not treat native static mesh asset data model as `.cpp` behavior expansion,
CMake changes, tests, public renderer API changes, app-shell changes, product
session changes, runtime/product/scene API changes, draw-list changes, shader
changes, loader work, file IO, glTF/static model parsing, material/texture/
descriptor/sampler policy, resource catalog, staging/device-local upload,
Linux/dGPU validation, backend abstraction, model slot binding behavior changes,
CLI/debugger output changes, or gameplay/session/input/scripted-control changes.
Do not treat native procedural bean mesh slot binding as public renderer API
change, app-shell change, product session change, runtime/product/scene API
change, draw-list change, shader change, loader work, file IO, glTF/static model
parsing, material/texture/descriptor/sampler policy, resource catalog, staging/
device-local upload, Linux/dGPU validation, backend abstraction, CLI/debugger
output change, or gameplay/session/input/scripted-control change.
Do not treat native procedural NPC mesh slot binding as public renderer API
change, app-shell change, product session change, runtime/product/scene API
change, draw-list change, shader change, loader work, file IO, glTF/static model
parsing, material/texture/descriptor/sampler policy, resource catalog, staging/
device-local upload, Linux/dGPU validation, backend abstraction, CLI/debugger
output change, or gameplay/session/input/scripted-control change.
Do not treat native static mesh text loading as renderer slot binding to loaded
files, public renderer API change, app-shell/CLI option change, product session
change, runtime/product/scene API change, draw-list change, shader change,
glTF/static model parsing, asset registry/catalog, material/texture/descriptor/
sampler policy, resource catalog, staging/device-local upload, Linux/dGPU
validation, backend abstraction, CLI/debugger output change, or gameplay/
session/input/scripted-control change.
Do not treat native player mesh asset binding as public renderer API change,
app-shell/CLI option change, product session change, runtime/product/scene API
change, draw-list change, shader change, glTF/static model parsing, asset
registry/catalog, material/texture/descriptor/sampler policy, resource catalog,
staging/device-local upload, Linux/dGPU validation, backend abstraction,
CLI/debugger output change, or gameplay/session/input/scripted-control change.

Do not treat native NPC mesh asset binding as public renderer API change,
app-shell/CLI option change, product session change, runtime/product/scene API
change, draw-list change, shader change, glTF/static model parsing, asset
registry/catalog, material/texture/descriptor/sampler policy, resource catalog,
staging/device-local upload, Linux/dGPU validation, backend abstraction,
CLI/debugger output change, or gameplay/session/input/scripted-control change.
