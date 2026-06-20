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
  a thin Qt-only target highlight from latest target-context diagnostics. Native
  `iggy_native_play` now has no-Qt scripted controls/debugger support for
  deterministic product-path stepping: `--scripted-controls LIST`,
  `--scripted-control-interval-ms`, `--debug-scripted-controls`,
  `--dump-final-state`, `--expect-player-tiles 'x,y;x,y'`, and
  `--quit-after-script`. Native no-Qt renderer prep now has a backend-neutral
  app-local scene draw-list extraction in `NativeSceneDrawList.hpp`, keeping
  draw-item ordering and transforms out of Vulkan command recording without
  extracting mesh-buffer ownership or renderer/swapchain/pipeline resources.
  Native no-Qt product play state/request/script orchestration now lives in
  app-local `NativeProductSession`, while `IggyNativePlay.cpp` remains the SDL,
  CLI/help, draw-list/camera orchestration, and app shell owner. Native Vulkan
  renderer skeleton extraction now moves Vulkan lifetime/swapchain/render pass/
  pipeline/depth/framebuffer/command/sync/cube-mesh recording and cleanup into
  app-local `NativeVulkanRenderer`, while `IggyNativePlay.cpp` still owns
  CLI/help/validation, MoltenVK fallback setup, SDL lifecycle/event loop,
  product session calls, seconds/camera/draw-list orchestration, and per-frame
  renderer input. Native GPU mesh resource wrapping now keeps the existing cube
  mesh path renderer-private while centralizing vertex/index buffer readiness and
  idempotent destruction without changing upload policy, draw behavior, model
  mapping, shaders, or public renderer API. Native pipeline/shader resource
  wrapping now keeps the existing render pass, pipeline layout, graphics
  pipeline, and shader module handles renderer-private while centralizing
  shader module and pipeline destruction/reset without changing shader files,
  shader interfaces, pipeline state, draw behavior, or public renderer API.
  Native model-slot/cube fallback registry groundwork now maps
  `NativeSceneModelId` through renderer-private model slots to the existing cube
  mesh fallback without changing draw-item order, cube mesh data, shader
  interface, push constants, tint behavior, or public renderer API. Native
  static mesh asset data groundwork now moves the cube's CPU positions, colors,
  and `std::uint16_t` indices into backend-free `NativeStaticMeshAsset` data
  while preserving the vertex-color pipeline shape, host-visible/coherent upload,
  model slot cube fallback behavior, and indexed draw behavior. Native
  procedural bean mesh binding now adds a second CPU static mesh helper and binds
  only the `Player` model slot to that non-cube mesh. Native procedural NPC mesh
  binding now adds a separate marker mesh for `NpcActor`, leaving floor and wall
  slots on the cube fallback. Native static mesh text loading now adds an
  app-local `.igmesh`-style text reader for position/color vertices and
  `std::uint16_t` triangles, with focused parser/file tests. Native player mesh
  asset binding now ships `apps/native_play/assets/player.igmesh`, loads it
  through that text loader, binds it to the `Player` slot when valid, and keeps
  the procedural bean as fallback. Native NPC mesh asset binding now ships
  `apps/native_play/assets/npc.igmesh`, loads it through the same text loader,
  binds it to the `NpcActor` slot when valid, and keeps the procedural NPC
  marker as fallback. Native floor/wall mesh asset binding now ships
  `apps/native_play/assets/floor.igmesh` and `wall.igmesh`, loads both through
  `LoadNativeStaticMeshAssetFile(NativePlayAssetPath(...))` during scene mesh
  creation, creates `floorMesh_` / `wallMesh_` only for successful loaded
  assets, and registers `Floor` / `Wall` to those meshes only when
  `HasMesh(...)` succeeds; fallback reuses the existing `cubeMesh_` without
  duplicate cube GPU uploads, and player/NPC bindings remain unchanged. Native
  static model slot policy now adds app-local value-only
  `NativeStaticModelPolicy.hpp`: `NativeStaticModelSlot` covers `Floor`, `Wall`,
  `NpcActor`, and `Player`; `NativeStaticModelAssetRef` carries `{ slot,
  meshFilename }`; `DefaultNativeStaticModelPolicy()` maps those slots to stable
  `.igmesh` filenames; and `FindNativeStaticModelAsset(...)` performs first-match
  lookup. `NativeVulkanRenderer.cpp` consumes the policy for filenames while
  preserving the current loaded `.igmesh` behavior and fallbacks. The policy has
  no filesystem, parser, GPU, or Vulkan knowledge. Native static model slot text
  helper extraction now centralizes `NativeStaticModelSlotText(...)` beside the
  model slot enum; static model load report `slot=...` rendering uses it while
  preserving report output byte-for-byte. Native static model load reporting now
  adds backend-free app-local `NativeStaticModelLoadReport.hpp`,
  which reports over the existing value-only policy and `.igmesh` loader without
  mutating renderer state. It iterates fixed slots in `Floor`, `Wall`,
  `NpcActor`, `Player` order, records per-slot filename/status/fallback/issues/
  vertex/index counts, aggregates loaded/failed/missing counts, and maps report
  fallbacks to `Cube`, `Cube`, `ProceduralNpcMarker`, and `ProceduralBean`.
  Native static model load status text helper extraction now centralizes
  `NativeStaticModelLoadStatusText(...)` beside the load status enum, and static
  model load report `status=...` rendering uses it while preserving report
  output byte-for-byte.
  Native static model fallback kind text helper extraction now centralizes
  `NativeStaticModelFallbackKindText(...)` beside the fallback kind enum, and
  static model load report `fallback=...` rendering uses it while preserving
  report output byte-for-byte.
  Native static model load report text renderer extraction now adds
  `BuildNativeStaticModelLoadReportText(...)`; app-shell report printing
  delegates to it while preserving the static model load report format
  byte-for-byte.
  `NativeVulkanRenderer.cpp` was not touched. Native static model load report
  CLI dumping now adds `iggy_native_play --dump-static-model-load-report`; the
  app builds `BuildNativeStaticModelLoadReport(DefaultNativeStaticModelPolicy(),
  IGGY_NATIVE_PLAY_ASSET_DIR)`, prints a compact aggregate plus one slot row per
  fixed slot, and exits before `NativeVulkanApp` construction/run. The command
  returns 0 only when all fixed slots load, returns nonzero for missing/failed
  slots, and does not require `--play`, scripted controls, SDL display
  availability, or Vulkan renderer initialization beyond normal binary linkage.
  Native static mesh text writing now adds pure app-local
  `NativeStaticMeshAssetWriter.hpp`, serializing the current `.igmesh` text
  format deterministically with a fixed header comment, vertex rows, and
  triangle rows; invalid or non-triangle input reports issues and emits no text.
  Test-only fixture roundtrip coverage now loads each checked-in renderer-bound
  `.igmesh` fixture, writes it, reloads it, writes it again, and asserts
  canonical writer idempotence for floor, wall, NPC, and player assets; it also
  adds procedural NPC marker write/reload count coverage.
  Native static mesh built-in export CLI dumping now adds
  `iggy_native_play --dump-static-mesh-asset NAME` for the existing procedural
  `cube`, `bean`, and `npc-marker` meshes. The app serializes the selected
  built-in mesh with `WriteNativeStaticMeshAssetText(...)`, prints raw
  deterministic `.igmesh` text to stdout, and exits before `NativeVulkanApp`,
  SDL, or Vulkan launch; unknown names and conflicts with
  `--dump-static-model-load-report` fail nonzero with compact errors.
  Native static mesh export policy now adds value-only
  `NativeStaticMeshExportPolicy.hpp` for the built-in export names, default
  filenames, and id-to-CPU-mesh mapping; the existing dump CLI resolves through
  that policy while preserving accepted names, raw stdout, unknown-name errors,
  and conflict behavior.
  Native static mesh output-directory export now adds header-only
  `NativeStaticMeshFileExport.hpp` and optional
  `--output-dir DIR` for `--dump-static-mesh-asset NAME`, writing
  `DIR/defaultFilename` only when the directory already exists and the target
  does not; stdout dumping remains unchanged without `--output-dir`.
  Native static mesh file export status text helper extraction now centralizes
  `NativeStaticMeshFileExportStatusText(...)` beside
  `NativeStaticMeshFileExportStatus`; single-export and batch-export compact CLI
  failure paths use it while preserving status strings and failure prefixes.
  Native static mesh single file export success text renderer extraction now
  adds `BuildNativeStaticMeshFileExportSuccessText(...)`; single-file output-dir
  success stdout delegates to it after the existing `Exported` check while
  preserving unknown/non-`Exported` failures and batch text.
  Native static mesh single file export failure text renderer extraction now
  adds `BuildNativeStaticMeshFileExportFailureText(...)`; the non-`Exported`
  single-file failure branch delegates the message body after preserving the
  unknown-asset special case, with the app prefix/newline still supplied by the
  existing exception/catch path.
  Native static mesh batch export success text renderer extraction now adds
  `BuildNativeStaticMeshFileExportBatchSuccessText(...)`; batch success stdout
  delegates to it after the existing `Exported` status check while preserving
  batch failure output selection and compact failure text.
  Native static mesh batch export failure text renderer extraction now adds
  `BuildNativeStaticMeshFileExportBatchFailureText(...)`; batch failure stderr
  body delegates through it while preserving output-path and issue-count
  selection plus the app-level prefix/newline.
  Native static mesh built-in batch export now adds
  `ExportNativeStaticMeshPolicyToDirectory(...)` and
  `iggy_native_play --export-static-mesh-assets --output-dir DIR`, preflighting
  the output directory and all default targets before writing
  `cube.igmesh`, `bean.igmesh`, and `npc-marker.igmesh`.
  Native static mesh export reporting now adds
  `NativeStaticMeshExportReport.hpp` and
  `iggy_native_play --dump-static-mesh-export-report`, reporting built-in export
  writability, counts, and bytes without writing files or inspecting output
  directories.
  Native static mesh export report status text helper extraction now centralizes
  `NativeStaticMeshExportReportStatusText(...)` beside the report status enum;
  export report row rendering uses it while preserving the default
  `status=Writable` report output byte-for-byte.
  Native static mesh export report text renderer extraction now adds
  `BuildNativeStaticMeshExportReportText(...)`; app-shell report printing
  delegates to it while preserving export report text byte-for-byte.
  Native static mesh export policy validation now adds backend-free
  `ValidateNativeStaticMeshExportPolicy(...)`, structured validation issues,
  and `InvalidPolicy` rejection for single/batch file export before lookup,
  directory checks, target preflight, writer work, or writes.
  Native static mesh export manifest text building now adds
  `NativeStaticMeshExportManifest.hpp` and
  `iggy_native_play --dump-static-mesh-export-manifest`, producing deterministic
  manifest text from validated export policy/report data without writing files
  or starting NativeVulkanApp, SDL, or Vulkan.
  Native static mesh export manifest status text helper extraction now
  centralizes `NativeStaticMeshExportManifestStatusText(...)` beside the mesh
  export manifest status enum; the dump path compact failure text uses it while
  preserving successful manifest output byte-for-byte.
  Native static mesh export manifest failure text renderer extraction now adds
  `BuildNativeStaticMeshExportManifestFailureText(...)`; the dump path delegates
  the non-written failure body through it after the existing `!result.written()`
  check while preserving app-level prefix/newline behavior.
  Native static mesh batch export now writes `cube.igmesh`, `bean.igmesh`,
  `npc-marker.igmesh`, and one `static-mesh-export-manifest.txt` sidecar whose
  content is exactly `BuildNativeStaticMeshExportManifestText(policy).text`,
  while single-asset stdout/output-dir export remains unchanged and writes no
  sidecar.
  Native static mesh export directory verification now adds read-only
  `VerifyNativeStaticMeshExportDirectory(...)` and
  `iggy_native_play --verify-static-mesh-export --output-dir DIR`, validating
  the manifest sidecar and expected policy files/counts before NativeVulkanApp,
  SDL, or Vulkan startup while ignoring unrelated extra files.
  Native static mesh export verification success text renderer extraction now
  adds `BuildNativeStaticMeshExportDirectoryVerificationSuccessText(...)`; the
  verify CLI delegates only successful stdout after `result.verified()` is true
  while preserving all failure rendering and verifier semantics.
  Native static mesh export verification failure text renderer extraction now
  adds `BuildNativeStaticMeshExportDirectoryVerificationFailureText(...)`; the
  verify CLI delegates only the non-verified failure body through it while
  preserving problem-path selection, app-level prefix/newline behavior, and
  success output.
  Native static mesh export verification reporting now adds a read-only report
  builder around the existing verifier and
  `iggy_native_play --dump-static-mesh-export-verification-report --output-dir DIR`,
  printing summary and per-asset rows before startup while preserving existing
  `--verify-static-mesh-export` output and behavior.
  Native static mesh export package policy now adds value-only
  `NativeStaticMeshExportPackagePolicy` metadata and deterministic validation
  for the export package format id, version, manifest filename, and nested mesh
  export policy without filesystem access, package IO, CLI changes, or renderer
  behavior changes.
  Native static mesh export package manifest text building now adds
  `NativeStaticMeshExportPackageManifest.hpp` and
  `iggy_native_play --dump-static-mesh-export-package-manifest`, producing
  deterministic no-write package-manifest text after package-policy validation
  before NativeVulkanApp, SDL, or Vulkan startup.
  Native static mesh export package manifest status text helper extraction now
  centralizes `NativeStaticMeshExportPackageManifestStatusText(...)` beside the
  package manifest status enum; the package manifest dump compact failure text
  uses it while preserving successful package manifest output byte-for-byte.
  Native static mesh export package manifest failure text renderer extraction
  now adds `BuildNativeStaticMeshExportPackageManifestFailureText(...)`; the
  package manifest dump path delegates the non-written failure body through it
  after the existing `!result.written()` check while preserving app-level
  prefix/newline behavior.
  Native static mesh batch package manifest sidecar export now writes
  `static-mesh-export-package-manifest.txt` alongside batch-exported meshes and
  `static-mesh-export-manifest.txt`, preflighting the package sidecar before
  asset writes while leaving single export, the no-write package-manifest dump,
  unchanged.
  Native static mesh package sidecar verification now requires
  `static-mesh-export-package-manifest.txt` in export directories: verifier and
  verification-report paths check exact package-manifest text after mesh
  manifest equality and before asset geometry checks.
  Native static mesh verification summary sidecar diagnostics now expose
  manifest/package manifest paths and verified flags in the read-only
  verification result; report summaries print `manifest=... packageManifest=...`
  and `--verify-static-mesh-export` success prints `packageManifest=ok`.
  Native static mesh package manifest text reading now adds a dependency-free,
  filesystem-free in-memory reader for the generated package-manifest grammar,
  validating header and asset rows without CLI, export, verification, renderer,
  file IO, or policy reconstruction integration.
  Native static mesh package manifest file reading now adds an explicit-path
  wrapper that opens only the supplied file in binary mode, delegates to the text
  reader, and reports `FileOpenFailed` without directory inference or verifier/
  CLI integration.
  Native static mesh package manifest verification reader diagnostics now wire
  that file reader into explicit-directory verification after package sidecar
  existence and before exact deterministic text comparison, reporting malformed
  sidecars as `PackageManifestReadFailed` with `packageManifest=invalid`.
  Native static mesh verification package read issue rows now preserve
  structured package manifest reader issues on verifier failures and print
  deterministic `packageManifestReadIssue ...` rows in the verification report.
  Native static mesh package manifest read issue text helper extraction now
  centralizes `NativeStaticMeshExportPackageManifestReadIssueCodeText(...)` in
  `NativeStaticMeshExportPackageManifest.hpp`; verification and package-directory
  report package read issue rows use it while the nested mesh manifest issue
  mapper remains separate for its different enum.
  Native static mesh export verification report text renderer extraction now
  moves existing verification report serialization into
  `BuildNativeStaticMeshExportDirectoryVerificationReportText(const NativeStaticMeshExportDirectoryVerificationReport &report)`;
  the full builder still verifies with `VerifyNativeStaticMeshExportDirectory(...)`
  and assigns `report.text` from the helper with byte-for-byte text preserved.
  Native static mesh export verification report data builder extraction now adds
  `BuildNativeStaticMeshExportDirectoryVerificationReportData(const NativeStaticMeshExportPolicy &policy, const std::filesystem::path &directory)`;
  the data builder stores the verifier result in `report.verification` and
  returns with `report.text` empty, while the full builder still assigns text
  through the report text renderer helper.
  Native static mesh verification report builder parity coverage now extends
  existing data-builder/full-builder and text-renderer parity assertions across
  missing output directory, file-not-directory, manifest mismatch, package
  manifest mismatch, corrupt asset/load failure, and extra-file-ignored branches
  without changing production source or report behavior.
  Native static mesh package directory reading now adds a header-only explicit
  directory reader that reads `static-mesh-export-package-manifest.txt` and
  projects nested manifest and asset paths without checking file existence,
  loading meshes, scanning directories, or integrating with CLI/verification.
  Native static mesh package directory reporting now adds a header-only no-write
  report builder over that reader, printing a summary, optional package manifest
  read issue rows, and parsed asset rows without inspecting beyond the reader.
  Native static mesh package directory report CLI now adds
  `iggy_native_play --dump-static-mesh-export-package-directory-report --output-dir DIR`,
  dispatching before NativeVulkanApp/SDL/Vulkan startup and returning success
  only when the package directory report read succeeds.
  Native static mesh package directory presence diagnostics now append
  `manifestExists=1|0` and per-asset `exists=1|0` facts to that report without
  changing read success, CLI exit semantics, verification, export, or loading.
  Native static mesh package directory file fact diagnostics now add non-throwing
  regular-file and byte-count facts for the package sidecar, nested manifest,
  and declared assets while keeping missing paths, directories, and size
  failures as `regularFile=0 bytes=0`.
  Native static mesh export manifest reading now has in-memory text parsing plus
  an explicit supplied-path file wrapper that opens binary, reports
  `FileOpenFailed` on missing files, and still avoids package-directory,
  verification, export, or CLI integration.
  Native static mesh export manifest read issue text helper extraction now
  centralizes `NativeStaticMeshExportManifestReadIssueCodeText(...)` beside the
  mesh export manifest reader enum in `NativeStaticMeshExportManifest.hpp`;
  package-directory report nested mesh `manifestReadIssue` rows use it while the
  package manifest issue helper remains separate for its different enum.
  Native static mesh package directory read status text helper extraction now
  moves `NativeStaticMeshExportPackageDirectoryReadStatusText(...)` to
  `NativeStaticMeshExportPackageDirectoryReader.hpp` beside the read status and
  result boundary; package-directory report summaries and compact CLI failure
  status text keep using the same helper name through includes.
  Native static mesh package directory manifest read diagnostics now parse the
  already-projected nested mesh manifest path from the package directory report,
  adding `manifestRead=...` summary facts and deterministic `manifestReadIssue`
  rows without changing `readOk()`, CLI exit behavior, verification, export, or
  package directory reader data.
  Native static mesh package directory manifest asset rows now emit parsed
  nested mesh manifest `manifestAsset=...` rows when manifest reading succeeds,
  still without package-vs-mesh comparison semantics or status/exit changes.
  Native static mesh package directory manifest row comparison diagnostics now
  compare package sidecar rows to parsed mesh manifest rows by asset name and
  filename only, adding `manifestMatches`/`manifestMismatches` summary facts and
  deterministic `manifestComparison` rows without acceptance, verification, or
  nonzero semantics.
  Native static mesh package directory comparison issue count diagnostics now
  append `manifestComparisonIssues=N` whenever comparison runs, matching the
  emitted `manifestComparison` row count without changing core `issues=`,
  `readOk()`, status, CLI exit, verification, or package acceptance semantics.
  Native static mesh package directory manifest comparison helper extraction now
  moves the package-vs-nested-mesh-manifest row comparison loops into
  `NativeStaticMeshExportPackageDirectoryManifestComparisonResult` and
  `CompareNativeStaticMeshExportPackageDirectoryManifestRows(...)` while
  preserving report text/order, counts, statuses, CLI exit behavior, and
  verification/export behavior.
  Native static mesh package directory structured manifest diagnostics now expose
  `manifestReadAttempted`, `manifestRead`, and `manifestComparison` on
  `NativeStaticMeshExportPackageDirectoryReport`, routing existing report output
  through those fields without changing text, rows, statuses, `readOk()`,
  `issues=`, CLI exit behavior, verification, package acceptance, or export
  behavior.
  Native static mesh package directory structured file facts now expose
  `NativeStaticMeshExportPackageDirectoryPathFacts`,
  `ReadNativeStaticMeshExportPackageDirectoryPathFacts(...)`,
  `NativeStaticMeshExportPackageDirectoryAssetFacts`, and stored package
  manifest, nested manifest, and package-declared asset facts on
  `NativeStaticMeshExportPackageDirectoryReport`, while preserving report text
  exactly.
  Native static mesh package directory report text renderer extraction now moves
  existing serialization into
  `BuildNativeStaticMeshExportPackageDirectoryReportText(const NativeStaticMeshExportPackageDirectoryReport &report)`;
  the builder still collects structured data, assigns `report.text` from the
  helper, and preserves report text byte-for-byte.
  Native static mesh package directory report data builder extraction now moves
  structured collection into
  `BuildNativeStaticMeshExportPackageDirectoryReportData(const std::filesystem::path &directory)`,
  which returns diagnostics with empty `text`; the full builder calls it and then
  assigns text through the renderer helper without changing report behavior.
  Native static mesh package directory report builder parity coverage now extends
  existing data-builder/full-builder and text-renderer parity assertions across
  missing-from-manifest, missing-from-package, filename mismatch, missing
  directory, file-not-directory, malformed package sidecar, malformed nested
  manifest, directory-at-declared-asset, and extra-file-ignored branches without
  changing production source or package-directory report behavior.
  Next product runtime work is deciding whether frame request/play surface should own
  enrichment, whether richer overlays/labels or diagnostics should be surfaced,
  or whether explicit interaction intent should be synthesized, then interaction
  execution if approved, app-shell/CLI extraction, other model-slot file binding,
  glTF/glb parsing under the future constrained subset, asset registry/catalog,
  materials/textures/descriptors/samplers,
  package/authoring asset policy, backend validation/abstraction,
  pause/retry/reset policy, completion/failure evaluation, and save/load UX.
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
Native scripted controls/debugger are complete for no-Qt product play:
`iggy_native_play --play PATH --scripted-controls LIST` injects scripted controls
through the same product input path as keyboard controls, supports
`--scripted-control-interval-ms`, `--debug-scripted-controls`,
`--dump-final-state`, `--expect-player-tiles 'x,y;x,y'`, and
`--quit-after-script`, and reports expectation mismatches with nonzero exit plus
the actual player tile. Final-state dump prints player tile, next frame index,
render command count, active input count, and held input count after the scripted
sequence completes.
Native Scene Draw List Extraction is complete for no-Qt renderer prep:
`NativeSceneDrawList.hpp` defines `NativeSceneModelId`, `NativeSceneDrawItem`,
`NativeSceneDrawListInput`, pure transform helpers, and
`BuildNativeSceneDrawItems(...)` under `iggy::native_play`. `IggyNativePlay.cpp`
adapts `product_->play.state.loop.currentState` through `nativeSceneDrawState()`
and records Vulkan commands from the returned draw items as before. Null state
or product state without a player keeps the fallback rotating player cube; floor
cubes are emitted for all map tiles in y/x order; wall cubes are emitted only
for non-walkable tiles through existing `tileAt(...)` behavior; present modern
NPC actors are emitted in registry order; the player is appended last; and
transforms, tints, inclusion policy, and vector order are intended unchanged.
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
- Native scripted controls/debugger are no-Qt app-shell tooling:
  scripted controls inject the same product input path as keyboard controls, and
  `--debug-scripted-controls` prints before/after player tile, frame request,
  play mode, surface, and loop statuses, input and ignored event counts,
  accepted/blocked/rejected counts, `npcMoved`, and render command count.
  `--dump-final-state` prints player tile, next frame index, render command
  count, active input count, and held input count after the script completes.
  This does not change gameplay semantics, runtime/product APIs, persistence,
  Qt, or render asset/material/glTF policy.
- Native scene draw-list extraction is no-Qt app-local renderer prep:
  `BuildNativeSceneDrawItems({ nativeSceneDrawState(), seconds })` produces
  backend-neutral draw items from nullable runtime gameplay state and seconds,
  while Vulkan command recording consumes those draw items as before. This is a
  header-only extraction with no CMake change, no runtime/product/scene/server/
  render-command API change, no SDL/input/scripted-control/free-play/gameplay
  stepping change, no CLI/debugger output/docs change in the source packet, no
  glTF/assets/textures/materials/animation/shader change, and no intended visual
behavior change. Vulkan mesh-buffer ownership and renderer/swapchain/pipeline/
command-buffer extraction remain separate later packets.
Native Product Session Extraction is complete for no-Qt app-shell separation:
`NativeProductSession.hpp/.cpp` add app-local `iggy::native_play::NativeProductSession`
and `NativeProductSessionConfig`. The session owns/delegates product load/play
state, product input accumulator, active movement controls, latest product frame
and has flag, presentation camera and has flag, scripted-control cadence/state,
final dump state, movement guard, camera request config, and one-frame product
request/tick flow. `IggyNativePlay.cpp` keeps app-shell responsibilities:
`LaunchOptions`/CLI parsing/help, SDL key mapping/event loop/window lifecycle,
and draw-list/camera orchestration remain there; later native renderer skeleton
extraction moves Vulkan lifetime/draw submission behind `NativeVulkanRenderer`.
`NativeProductSessionConfig` carries raw scripted-control specs, and the
session constructor loads the product scenario before parsing scripted controls,
preserving pre-extraction side-effect/error ordering; `ParseArgs` still uses the
shared parser only for `--expect-player-tiles` count validation.
Native Vulkan Renderer Skeleton Extraction is complete for no-Qt renderer prep:
`NativeVulkanRenderer.hpp/.cpp` add app-local
`iggy::native_play::NativeVulkanRenderer` with a minimal pimpl public surface.
`NativeVulkanFrameInput` carries `Mat4 viewProjection` plus a borrowed draw-item
vector pointer, and the renderer exposes `initialize(SDL_Window *)`,
`drawFrame(...)`, `markFramebufferResized()`, `aspectRatio()`, `waitIdle()`, and
`cleanup()`. Vulkan lifetime/resources, swapchain, render pass, pipeline, depth,
framebuffers, command pool, command buffers, sync, cube mesh, recording,
acquire/submit/present, recreate, and cleanup moved from `IggyNativePlay.cpp`
to `NativeVulkanRenderer.cpp`. `IggyNativePlay.cpp` remains CLI/help/validation,
MoltenVK fallback setup, SDL init/window/event loop/destruction/quit, SDL key
mapping, product session calls, seconds/camera/draw-list orchestration, and
per-frame renderer input owner. The renderer stores only a non-owning
`SDL_Window *` for Vulkan interop, consumes per-call draw input synchronously,
does not depend on product/session/scripted controls or
`BuildNativeSceneDrawItems(...)`, and does not introduce generalized mesh/
resource ownership beyond the mechanical cube mesh move.
Native GPU Mesh Resource Wrapper is complete for no-Qt renderer prep:
`NativeVulkanRenderer.cpp` now wraps the existing cube mesh GPU buffers in
renderer-private `NativeVulkanBufferResource` and `NativeVulkanMeshResource`
structs, with `HasBuffer`/`HasMesh` readiness checks. Buffer creation still uses
caller-provided usage flags and host-visible/coherent upload memory for the cube
vertex/index buffers. Cube data, all scene model ids mapping to the single cube
mesh, draw order, shader interface, push constants, tint behavior, and
`VK_INDEX_TYPE_UINT16` indexed draw parameters are unchanged. Cleanup is
centralized through `destroyBuffer(...)` and `destroyMeshResource(...)`, with
buffer-before-memory destruction, index-before-vertex mesh cleanup, and
reset-to-default idempotence. `NativeVulkanRenderer.hpp`, `IggyNativePlay.cpp`,
CMake, shaders, runtime/product/scene APIs, and draw-list data are unchanged.
This is a private renderer cleanup, not a mesh registry, model-slot binding,
asset loader, resource catalog, staging/device-local upload policy, glTF/
texture/material/animation packet, or public renderer API change. The
pre-existing allocation failure-path risk between `vkCreateBuffer` and ownership
assignment remains future RAII/exception-safety work, not a new blocker.
Native Pipeline/Shader Resource Wrapper is complete for no-Qt renderer prep:
`NativeVulkanRenderer.cpp` now wraps the existing render pass, pipeline layout,
graphics pipeline, and shader module handles in renderer-private resource
structs. `createRenderPass()` fills `pipeline_.renderPass`;
`createShaderModule(...)` returns a wrapped shader module; shader modules are
destroyed and reset through `destroyShaderModule(...)`; draw and command
recording use `pipeline_.layout`, `pipeline_.renderPass`, and
`pipeline_.graphics`; and `destroyPipelineResource(...)` destroys graphics
pipeline, pipeline layout, and render pass in that order before resetting the
resource. Shader filenames, shader file-read behavior, stage setup,
`pName = "main"`, vertex input, fixed pipeline state, push constant range,
render pass attachments/dependency/layouts, command-buffer bind behavior, and
swapchain recreate behavior are unchanged. `NativeVulkanRenderer.hpp`,
`IggyNativePlay.cpp`, CMake, shader files, runtime/product/scene APIs, product
session, and draw-list data are unchanged. This is renderer-private ownership
cleanup, not a public renderer API change, descriptor/sampler/material/texture
packet, model-slot binding, asset/glTF loader, shader-interface change, staging
upload policy, Linux/dGPU validation policy, or backend abstraction.
Native Model Slot / Cube Fallback Registry is complete for no-Qt renderer prep:
`NativeVulkanRenderer.cpp` now has renderer-private `NativeVulkanModelSlot` and
`NativeVulkanModelRegistry` types. `NativeSceneModelId` maps to model slots,
then model slots resolve to mesh bindings. The initial registry bound every slot
to the cube fallback; later procedural mesh packets bound `Player` to the
bean mesh and `NpcActor` to the NPC marker mesh, and the later floor/wall asset
binding packet supersedes the terrain slots when loaded assets validate.
`meshForSceneModel(...)` delegates through
`ModelSlotForSceneModel(...)` and `meshForModelSlot(...)`, while draw-item
iteration/order remains unchanged. This preserves the same `NativeSceneModelId`
surface, mesh readiness behavior, draw parameters, shader pipeline, push
constants, and tint behavior. This is renderer-private model-slot groundwork
only: no public renderer API, app-shell, draw-list, shader, runtime/product/
scene, CLI/debugger, gameplay, asset loading, mesh file IO, descriptors/
samplers, textures/materials, glTF, package discovery, staging/device-local
upload, Linux/dGPU validation, or backend abstraction work is included.
Native Static Mesh Asset Data Model is complete for no-Qt renderer prep:
`NativeStaticMeshAsset.hpp` adds an app-local backend-free CPU mesh asset model.
`NativeStaticMeshVertex` stores `Vec3 position` plus
`std::array<float, 3> color`, preserving the current two-`vec3` vertex-color
pipeline shape, and `NativeStaticMeshAsset` stores vertices plus
`std::uint16_t` indices. Inline validation covers non-empty vertices, non-empty
indices, index range checks, and current `std::uint32_t` draw-count fit.
`NativeCubeStaticMeshAsset()` preserves the previous cube positions, colors, and
indices exactly. `NativeVulkanRenderer.cpp` now consumes `NativeStaticMeshAsset`
for cube upload; vertex binding/attributes use `NativeStaticMeshVertex`; upload
remains host-visible/coherent; indexed draw remains `VK_INDEX_TYPE_UINT16`; and
the cube fallback was still the floor/wall mesh at that stage while later
procedural helpers used the same CPU mesh shape for player and NPC model slots.
The later floor/wall loaded asset binding supersedes the terrain slots when
those files validate.
This is no-loader static mesh data groundwork only: no `.cpp`, CMake, tests,
public renderer API, app shell, product session, runtime/product/scene API,
draw-list, shader, loader, file IO, glTF, material/texture/descriptor/sampler,
resource catalog, staging/device-local upload, Linux/dGPU, backend abstraction,
model slot binding behavior, CLI/debugger output, or gameplay/session/input/
scripted-control change is included.
Native Procedural Bean Mesh Slot Binding is complete for no-Qt renderer prep:
`NativeStaticMeshAsset.hpp` adds `NativeBeanStaticMeshAsset()`, an in-memory
procedural non-cube mesh using the existing `NativeStaticMeshVertex` position/
color shape and `std::uint16_t` indexed triangles. `NativeVulkanRenderer.cpp`
creates a separate `playerMesh_` from that CPU asset and binds only
`NativeVulkanModelSlot::Player` to it; `Floor` and `Wall` still used the cube
fallback at that stage, and `NpcActor` is covered by the NPC marker binding
below. Later floor/wall loaded assets supersede those terrain slots when valid.
Upload still uses the existing
host-visible/coherent mesh resource path, vertex binding/attributes remain the
same two `vec3` shader inputs, indexed draw remains `VK_INDEX_TYPE_UINT16`, and
draw-item order, tints, camera, product/session behavior, CLI/debugger output,
and public renderer API are unchanged. This is procedural in-memory mesh slot
proof only: no loader, file IO, glTF/static model parsing, asset registry/
catalog, material/texture/descriptor/sampler policy, shader change, staging/
device-local upload, Linux/dGPU validation, backend abstraction, runtime/
product/scene API, app shell, or gameplay behavior change is included.
Native Procedural NPC Mesh Slot Binding is complete for no-Qt renderer prep:
`NativeStaticMeshAsset.hpp` adds `NativeNpcMarkerStaticMeshAsset()`, an
in-memory procedural tapered marker mesh using the same position/color vertex
shape and `std::uint16_t` indexed triangles. `NativeVulkanRenderer.cpp` creates
a separate `npcMesh_` from that CPU asset and binds
`NativeVulkanModelSlot::NpcActor` to it; `Player` remains bound to the bean
mesh, while `Floor` and `Wall` still used the cube fallback at that stage.
Later floor/wall loaded assets supersede those terrain slots when valid. Upload
still uses the existing
host-visible/coherent mesh resource path, vertex binding/attributes remain the
same two `vec3` shader inputs, indexed draw remains `VK_INDEX_TYPE_UINT16`, and
draw-item order, tints, camera, product/session behavior, CLI/debugger output,
and public renderer API are unchanged. This is procedural in-memory mesh slot
proof only: no loader, file IO, glTF/static model parsing, asset registry/
catalog, material/texture/descriptor/sampler policy, shader change, staging/
device-local upload, Linux/dGPU validation, backend abstraction, runtime/
product/scene API, app shell, or gameplay behavior change is included.
Native Static Mesh Text Loader is complete for no-Qt renderer prep:
`NativeStaticMeshAssetLoader.hpp` adds an app-local header-only text loader for
the existing `NativeStaticMeshAsset` CPU mesh shape. It parses comments and
blank lines plus `v x y z r g b` vertex records and `tri i0 i1 i2` triangle
records into `NativeStaticMeshVertex` data and `std::uint16_t` indices.
`LoadNativeStaticMeshAssetText(...)` and `LoadNativeStaticMeshAssetFile(...)`
return `NativeStaticMeshAssetLoadResult` with structured issues for file-open
failure, unknown directives, malformed vertices, malformed triangles, out-of-
range indices, extra tokens, and invalid final meshes. A focused
`native_static_mesh_asset_loader_tests` target covers valid text/file loading
and the main failure modes. This is a minimal native text mesh loader only: no
renderer slot binding to loaded files, CLI option, package discovery, glTF,
asset registry/catalog, material/texture/descriptor/sampler policy, shader
change, staging/device-local upload, runtime/product/scene API, app shell, or
gameplay behavior change is included.
Native Player Mesh Asset Binding is complete for no-Qt renderer prep:
`engine/apps/native_play/assets/player.igmesh` adds the first checked-in
minimal text mesh asset. `iggy_native_play` now receives an
`IGGY_NATIVE_PLAY_ASSET_DIR` compile definition, and `NativeVulkanRenderer.cpp`
loads `player.igmesh` through `LoadNativeStaticMeshAssetFile(...)` when creating
scene meshes. A valid loaded asset becomes `playerMesh_` and remains bound to
`NativeVulkanModelSlot::Player`; if loading fails or validates false, the
existing procedural bean remains the silent fallback. The loader test now
validates the checked-in player asset fixture. This binds one loaded text mesh
to one renderer-private model slot only: no public renderer API, CLI option,
package discovery, glTF/static model parsing, asset registry/catalog,
material/texture/descriptor/sampler policy, shader change, staging/device-local
upload, runtime/product/scene API, app shell behavior, CLI/debugger output, or
gameplay behavior change is included.
Native NPC Mesh Asset Binding is complete for no-Qt renderer prep:
`engine/apps/native_play/assets/npc.igmesh` adds a checked-in minimal native NPC
text mesh asset. `NativeVulkanRenderer.cpp` loads `npc.igmesh` through
`LoadNativeStaticMeshAssetFile(...)` when creating scene meshes. A valid loaded
asset becomes `npcMesh_` and remains bound to `NativeVulkanModelSlot::NpcActor`;
if loading fails or validates false, the existing procedural NPC marker remains
the silent fallback. The loader test now validates the checked-in NPC asset
fixture. This binds one loaded text mesh to one renderer-private NPC model slot
only: no public renderer API, CLI option, package discovery, glTF/static model
parsing, asset registry/catalog, material/texture/descriptor/sampler policy,
shader change, staging/device-local upload, runtime/product/scene API, app shell
behavior, CLI/debugger output, or gameplay behavior change is included.

Native Floor/Wall Mesh Asset Binding is complete for no-Qt renderer prep:
`engine/apps/native_play/assets/floor.igmesh` and
`engine/apps/native_play/assets/wall.igmesh` add checked-in native text mesh
assets for the floor and wall model slots. `NativeVulkanRenderer.cpp` loads
both through `LoadNativeStaticMeshAssetFile(NativePlayAssetPath(...))` when
creating scene meshes. Successful loaded assets become `floorMesh_` and
`wallMesh_`; `NativeVulkanModelSlot::Floor` and `NativeVulkanModelSlot::Wall`
register to those loaded meshes only when `HasMesh(...)` succeeds. If either
asset fails to load or validates false, that slot reuses the existing
`cubeMesh_` fallback, with no duplicate cube GPU mesh upload for floor/wall
fallback. `native_static_mesh_asset_loader_tests` validates the checked-in floor
and wall fixture counts, and Player/NPC bindings remain unchanged. This binds
two loaded text mesh assets to renderer-private floor/wall slots only: no public
renderer API, CMake, app-shell/CLI option, package discovery, glTF/static model
parsing, asset registry/catalog, material/texture/descriptor/sampler policy,
shader change, staging/device-local upload, Linux/dGPU policy, backend
abstraction, runtime/product/scene/draw-list API, CLI/debugger output, or
gameplay/session/input/scripted-control change is included.

Native Static Model Slot Policy, .igmesh First is complete for no-Qt renderer
prep: `NativeStaticModelPolicy.hpp` adds an app-local value-only policy under
`engine/apps/native_play`. `NativeStaticModelSlot` lists `Floor`, `Wall`,
`NpcActor`, and `Player`; `NativeStaticModelAssetRef` carries a slot plus
`meshFilename`; `NativeStaticModelPolicy` stores a `models` vector;
`DefaultNativeStaticModelPolicy()` maps `Floor -> floor.igmesh`,
`Wall -> wall.igmesh`, `NpcActor -> npc.igmesh`, and `Player -> player.igmesh`;
and `FindNativeStaticModelAsset(...)` returns the first matching slot. The policy
has no filesystem, file loading, parsing, GPU, or Vulkan knowledge.
`NativeVulkanRenderer.cpp` consumes the policy only to choose filenames for the
existing loaded `.igmesh` path, preserving current load behavior and fallback
meshes. The source packet adds focused policy tests for stable default entries,
lookup, missing slots, duplicate first-match behavior, and value-only filenames.
This is not a glTF/glb parser, JSON/GLB dependency, shared render-server move,
public renderer API change, app shell change, runtime/product/scene/server/
draw-list API change, shader/material/texture/descriptor/sampler policy,
staging/device-local upload policy, Linux/dGPU policy, backend abstraction,
CLI/debugger output change, or gameplay/session/input/scripted-control change.
The future glTF subset remains separately gated: one mesh, one primitive,
triangles, required positions, optional vertex colors/default later, indexed
`uint16` first, and no materials, textures, normals, UVs, animation, skins,
scene graph, or transforms.

Native Static Model Slot Text Helper Extraction is complete as a behavior-
preserving slot text cleanup. The central inline helper
`NativeStaticModelSlotText(...)` now lives beside `NativeStaticModelSlot` in
`NativeStaticModelPolicy.hpp`. The static model load report `slot=...` rendering
uses the central helper after removing the CLI-local
`NativeStaticModelSlotName(...)` switch from `IggyNativePlay.cpp`. Stable slot
strings are `Floor`, `Wall`, `NpcActor`, `Player`, and fallback `Unknown`.
Direct static model policy tests cover all four slots plus `Unknown` fallback.
`NativeStaticModelLoadStatusName(...)` and
`NativeStaticModelFallbackKindName(...)` remain local and unchanged for possible
later packets. Source verification passed `native_static_model_policy_tests`,
`iggy_native_play`, exact CLI smoke for `--dump-static-model-load-report`, and
source `git diff --check`; `NativeVulkanRenderer.cpp` rebuilt because it includes
the edited policy header, but no renderer source changed. Successful
`--dump-static-model-load-report` output is preserved byte-for-byte for checked-
in assets: summary row, fixed row order, slot names, filenames, statuses,
fallbacks, counts, issue counts, and trailing newlines. This packet does not
change static model policy defaults or lookup, static model load report output,
row order, load status behavior, fallback behavior, CLI parser/help/dispatch/
conflict/exit behavior, renderer/model-slot behavior, static mesh export/report/
manifest/package/verification/package-directory behavior, CMake, fixtures,
assets, package loading/discovery, write policy, schema, or glTF/glb/JSON parser
work.

Native Static Model Load Report, .igmesh Policy Path is complete for no-Qt
renderer prep: `NativeStaticModelLoadReport.hpp` adds a backend-free app-local
report builder over the existing value-only `NativeStaticModelPolicy` and
existing `.igmesh` loader. `BuildNativeStaticModelLoadReport(...)` iterates the
fixed slot order `Floor`, `Wall`, `NpcActor`, `Player`; uses
`FindNativeStaticModelAsset(policy, slot)` and
`LoadNativeStaticMeshAssetFile(assetRoot / meshFilename)` only for explicit
policy refs; and records per-slot `slot`, `meshFilename`, `status`, `fallback`,
`issueCount`, `vertexCount`, and `indexCount`. Status values are
`MissingPolicyRef`, `Loaded`, and `LoadFailed`; aggregate counts are loaded,
failed, and missing. Report-only fallback mapping is `Floor -> Cube`,
`Wall -> Cube`, `NpcActor -> ProceduralNpcMarker`, and
`Player -> ProceduralBean`. Tests cover default checked-in assets and counts
(`floor` 4/6, `wall` 8/36, `npc` 7/30, `player` 6/24), missing policy refs, bad
filename load failure, and no inference of unlisted assets.
`NativeVulkanRenderer.cpp` is untouched, so there is no renderer mutation or
GPU/Vulkan/SDL behavior. This is not a glTF/glb parser, GLB binary parser, JSON
parser, custom glTF subset parser, dependency fetch, package install, file
discovery, package discovery, registry/catalog, model authoring policy, asset
manifest, shared render-server move, public renderer API change, app shell
change, runtime/product/scene/server/draw-list API change, shader/material/
texture/descriptor/sampler policy, normals/UVs/animation/skins/scene graph/
transforms work, staging/device-local upload policy, Linux/dGPU policy, backend
abstraction, CLI/debugger output change, or gameplay/input/scripted-control
change.

Native Static Model Load Status Text Helper Extraction is complete as a
behavior-preserving load status text cleanup. The enum-owned inline helper
`NativeStaticModelLoadStatusText(...)` now lives beside
`NativeStaticModelLoadStatus` in `NativeStaticModelLoadReport.hpp`. Static model
load report `status=...` rendering uses the central helper after removing the
CLI-local status switch from `IggyNativePlay.cpp`. Stable status strings are
`MissingPolicyRef`, `Loaded`, and `LoadFailed`, with fallback `Unknown`.
`NativeStaticModelFallbackKindName(...)` remains local and unchanged for a
possible later packet. Direct static model load report tests cover
`MissingPolicyRef`, `Loaded`, `LoadFailed`, and out-of-range `Unknown` fallback.
Source verification passed `native_static_model_load_report_tests`,
`iggy_native_play`, CLI smoke for `--dump-static-model-load-report`, `rg` checks
for the summary row and all four `status=Loaded` slot rows, and source
`git diff --check`. Successful `--dump-static-model-load-report` output is
preserved byte-for-byte for checked-in assets: summary row, fixed row order,
slot names, filenames, statuses, fallbacks, counts, issue counts, and trailing
newlines. This packet does not change static model policy defaults or lookup,
static model load report output, row order, load status assignment, fallback
behavior, CLI parser/help/dispatch/conflict/exit behavior, renderer/model-slot
behavior, static mesh export/report/manifest/package/verification/package-
directory behavior, CMake, fixtures, assets, package loading/discovery, write
policy, schema, or glTF/glb/JSON parser work.

Native Static Model Fallback Kind Text Helper Extraction is complete as a
behavior-preserving fallback text cleanup. The enum-owned inline helper
`NativeStaticModelFallbackKindText(...)` now lives beside
`NativeStaticModelFallbackKind` in `NativeStaticModelLoadReport.hpp`. Static
model load report `fallback=...` rendering uses the central helper after
removing the CLI-local fallback-kind switch from `IggyNativePlay.cpp`. Stable
fallback strings are `Cube`, `ProceduralBean`, and `ProceduralNpcMarker`, with
fallback `Unknown`. Direct static model load report tests cover `Cube`,
`ProceduralBean`, `ProceduralNpcMarker`, and out-of-range `Unknown` fallback.
Source verification passed `native_static_model_load_report_tests`,
`iggy_native_play`, CLI smoke for `--dump-static-model-load-report`, `rg` checks
for the summary row and all four slot rows with unchanged fallback names/count
fields, source `git diff --check`, and source `git diff --cached --check`.
Successful `--dump-static-model-load-report` output is preserved byte-for-byte
for checked-in assets: summary row, fixed row order, slot names, filenames,
statuses, fallback names, counts, issue counts, and trailing newlines. This
packet does not change `NativeStaticModelPolicy.hpp`, fallback assignment
behavior, static model policy defaults or lookup, static model load report
output, row order, CLI parser/help/dispatch/conflict/exit behavior, renderer/
model-slot behavior, `NativeVulkanRenderer.cpp`, static mesh export/report/
manifest/package/verification/package-directory behavior, CMake, fixtures,
assets, package loading/discovery/acceptance, write policy, schema, or glTF/glb/
JSON parser work.

Native Static Model Load Report Text Renderer Extraction is complete as a
behavior-preserving report serialization cleanup. `NativeStaticModelLoadReport.hpp`
now exposes pure header-only `BuildNativeStaticModelLoadReportText(const
NativeStaticModelLoadReport &report)`. `PrintNativeStaticModelLoadReport(...)`
delegates to the renderer helper, and CLI output/exit behavior remains
unchanged. The renderer preserves the existing static model load report text
format exactly: summary row, entry row order, `slot=`,
`filename=<missing>` handling, `status=`, `fallback=`, vertex/index/issue
counts, and trailing newlines. Source cleanup removed no-longer-needed app-shell
direct using declarations for load-report entry/status/fallback/slot text
helpers. Exact text tests cover the default checked-in asset report and a
missing-policy-ref report branch. Source verification passed
`native_static_model_load_report_tests`, `iggy_native_play`, CLI smoke for
`--dump-static-model-load-report`, `rg` checks for the exact summary row and all
four default slot rows, source `git diff --check`, and source
`git diff --cached --check`. This packet does not change
`NativeStaticModelPolicy.hpp`, static model policy defaults or lookup, fallback
assignment behavior, report data-building semantics, renderer/model-slot
behavior, `NativeVulkanRenderer.cpp`, CLI parser/help/dispatch/conflict/exit
behavior, checked-in assets or fixtures, `.igmesh` schema/loading, static mesh
export/package/verification/package-directory behavior, exact verification
behavior, generated sidecar/export write policy, package loading/discovery/
acceptance, or glTF/glb/JSON parser work.

Native Static Model Load Report CLI Dump is complete for no-Qt asset
diagnostics: `iggy_native_play` accepts `--dump-static-model-load-report` through
`LaunchOptions::dumpStaticModelLoadReport`, argument parsing, and help text in
`IggyNativePlay.cpp`. Main dispatch builds
`BuildNativeStaticModelLoadReport(DefaultNativeStaticModelPolicy(),
IGGY_NATIVE_PLAY_ASSET_DIR)`, prints a compact stdout report, and exits before
`NativeVulkanApp` construction/run. Exit code is 0 only when all fixed slots are
loaded and nonzero if any fixed slot is missing or failed. The dump does not
require `--play`, scripted controls, SDL display availability, or Vulkan
renderer initialization beyond normal binary linkage. Output starts with
`static-model-load-report loaded=N failed=N missing=N`, followed by one row per
slot with slot, filename or `<missing>`, status, fallback, vertices, indices,
and issues; current checked-in assets report Floor 4/6, Wall 8/36, NpcActor
7/30, and Player 6/24. Existing play/scripted/debug/final-state behavior and
output are preserved except for the added help option. This is not a glTF/glb
parser, GLB binary parser, JSON parser, custom glTF subset parser, dependency
fetch, package install, web lookup, `.igmesh` schema change, file discovery,
directory scanning, package discovery, asset registry/catalog, manifest
expansion, model authoring policy, renderer API change, `NativeVulkanRenderer`
change, `NativeSceneDrawList.hpp` change, runtime/product/scene/server API
change, gameplay/input/scripted-control semantic change, shader/material/
texture/descriptor/sampler policy, normals/UVs/animation/skins/scene graph/
transforms work, staging/device-local upload policy, Linux/dGPU policy, or
backend abstraction.

Native Static Mesh Text Writer / Roundtrip is complete for no-Qt asset
groundwork: `NativeStaticMeshAssetWriter.hpp` adds pure app-local
`WriteNativeStaticMeshAssetText(const NativeStaticMeshAsset &)`, serializing the
currently loaded `.igmesh` text format. Successful output is deterministic and
newline-terminated: fixed header `# Native static mesh asset`, one
`v x y z r g b` row per vertex in order, and one `tri a b c` row per three
indices in order. `NativeStaticMeshAssetWriteIssueCode` contains `InvalidMesh`
and `NonTriangleIndexCount`; `NativeStaticMeshAssetWriteResult` carries `text`,
`issues`, and `written()`. The writer refuses non-serializable input without
mutation or repair: invalid/empty mesh reports `InvalidMesh` with no text, and
valid indices whose count is not a multiple of three report
`NonTriangleIndexCount` with no text/tri rows. `IsNativeStaticMeshAssetValid(...)`
semantics are unchanged. Tests cover deterministic triangle text plus reload,
cube roundtrip representative data, procedural bean counts, empty invalid mesh,
non-triangle index count, and deterministic repeated calls. This is not a
glTF/glb/JSON parser, GLB binary parser, custom glTF subset parser, dependency
fetch, package install, vendoring, web lookup, `.igmesh` schema expansion beyond
serializing the current format, normals/UVs/materials/textures/descriptors/
samplers/skins/animation/scene graph/transforms/metadata fields, file writing,
file discovery, directory scanning, package discovery, registry/catalog,
manifest expansion, authoring policy, renderer behavior change,
`NativeVulkanRenderer.cpp` change, public renderer API change, draw-list/
runtime/product/scene/server API change, app-shell/CLI change, gameplay/input/
scripted-control change, or docs mixed into source.

Native Static Mesh Fixture Writer Roundtrip is complete as a test-only extension
of `native_static_mesh_asset_writer_tests.cpp`: checked-in renderer-bound
`.igmesh` fixtures now roundtrip through `LoadNativeStaticMeshAssetFile`,
`WriteNativeStaticMeshAssetText`, `LoadNativeStaticMeshAssetText`, and a second
canonical write/idempotence assertion. Covered fixture counts are
`floor.igmesh` 4 vertices / 6 indices, `wall.igmesh` 8 / 36, `npc.igmesh`
7 / 30, and `player.igmesh` 6 / 24. The packet also adds procedural NPC marker
write/reload count coverage to pair with existing procedural bean coverage.
CMake only adds `IGGY_NATIVE_PLAY_TEST_ASSET_DIR` to
`native_static_mesh_asset_writer_tests`. No production source, renderer, app
shell, asset fixture, CLI, shader, runtime/product/scene, or docs changes were
part of the source packet. This is not a glTF/glb/JSON parser, GLB/custom
parser, dependency fetch, package install, vendoring, web lookup, `.igmesh`
schema expansion or fixture rewrite, file writing/export CLI, normals/UVs/
materials/textures/descriptors/samplers/skins/animation/scene graph/transforms/
metadata fields, file discovery beyond explicit checked-in test filenames,
directory scanning, package discovery, registry/catalog, manifest expansion,
authoring package policy, renderer behavior change, `NativeVulkanRenderer.cpp`
change, public renderer API change, draw-list/runtime/product/scene/server API
change, app-shell/CLI change, gameplay/input/scripted-control change, or docs
mixed into source.

Native Static Mesh Built-In Export CLI is complete as an app-shell diagnostic:
`iggy_native_play --dump-static-mesh-asset NAME` supports exactly `cube`,
`bean`, and `npc-marker`. The command selects the existing built-in procedural
mesh, serializes it through `WriteNativeStaticMeshAssetText(...)`, prints the
writer's raw deterministic `.igmesh` text to stdout, and exits 0 before
`NativeVulkanApp` construction, SDL initialization, or Vulkan launch. Unknown
names fail nonzero with a compact error such as
`iggy_native_play: unknown static mesh asset: nope`; combining
`--dump-static-model-load-report` with `--dump-static-mesh-asset` also fails
nonzero to avoid ambiguous stdout formats. Sample output for `cube`, `bean`,
and `npc-marker` starts with `# Native static mesh asset`; cube output includes
cube vertex rows and a later `tri 0 1 2`, while bean and NPC marker output emit
their existing procedural vertex rows. Existing help/report/scripted/final-state/
product/session/render behavior is preserved except for the added help option.
This is not file writing, fixture rewriting, arbitrary asset path input,
checked-in asset normalization, glTF/glb/JSON parsing, dependency work, schema
expansion, materials/textures/descriptors/samplers/normals/UVs/animation/scene
graph fields, renderer behavior/API change, runtime/product/scene/server/
draw-list API change, gameplay/scripted/final-state semantic change, or docs
mixed into source.

Native Static Mesh Export Policy is complete as value-only native app metadata:
`NativeStaticMeshExportPolicy.hpp` defines `NativeStaticMeshBuiltInExportId`
with `Cube`, `Bean`, and `NpcMarker`; `NativeStaticMeshExportAssetRef` with
`id`, `name`, and `defaultFilename`; and `NativeStaticMeshExportPolicy` with
`assets`. `DefaultNativeStaticMeshExportPolicy()` returns stable refs in order:
`Cube` / `cube` / `cube.igmesh`, `Bean` / `bean` / `bean.igmesh`, and
`NpcMarker` / `npc-marker` / `npc-marker.igmesh`.
`FindNativeStaticMeshExportAsset(policy, name)` performs first-match lookup, and
`BuiltInNativeStaticMeshExportAsset(id)` maps ids to the existing built-in CPU
mesh assets. Existing `--dump-static-mesh-asset NAME` now resolves through the
policy before writing raw `.igmesh`, preserving accepted names, unknown-name
error behavior, conflict behavior with `--dump-static-model-load-report`, and
raw deterministic stdout. Tests cover stable default order, exact names and
filenames, basename-only filenames, lookup, missing-name null, duplicate
first-match, and writer-valid built-in meshes. This is not file writing,
`--output`, fixture rewrite/canonicalization, arbitrary asset path input,
directory scanning, package discovery, registry/catalog/manifest expansion,
source mutation, glTF/glb/JSON parser/dependency work, `.igmesh` schema
expansion, material/texture/descriptor/sampler/normals/UV/animation/scene graph
metadata, renderer behavior, `NativeVulkanRenderer.cpp`, public renderer API,
runtime/product/scene/server/draw-list API change, gameplay/scripted/final-state
semantic change, or docs mixed into source.

Native Static Mesh Output Directory Export CLI is complete as a constrained
file-export path for built-in `.igmesh` dumps: header-only
`NativeStaticMeshFileExport.hpp` adds
`ExportNativeStaticMeshAssetToDirectory(...)`, which validates policy lookup,
existing output directory, directory type, target nonexistence, writer success,
file open, and write success. Expected validation failures return
status/result data and do not print or throw. `iggy_native_play
--dump-static-mesh-asset NAME` now accepts optional `--output-dir DIR`; without
it, raw `.igmesh` stdout is unchanged. With it, the CLI writes to
`DIR/defaultFilename` from `NativeStaticMeshExportPolicy` and prints compact
status such as
`static-mesh-export name=cube output=/tmp/iggy-native-export-smoke/cube.igmesh bytes=523`.
`--output-dir` requires `--dump-static-mesh-asset`, the static model load report
conflict remains, unknown assets still report
`iggy_native_play: unknown static mesh asset: nope`, and existing targets fail
with `TargetAlreadyExists`. Tests cover export to a temp directory plus reload,
default policy exports, unknown assets, missing output directory, file-not-
directory output path, target already exists, and no parent directory creation.
This is not arbitrary `--output PATH`, overwrite/force/delete/rename/temp-file
replacement, fixture rewrite/canonicalization, checked-in fixture writes by
default, production directory creation, package discovery, scanning, registry/
catalog/manifest expansion, source mutation, glTF/glb/JSON parser/dependency
work, `.igmesh` schema expansion, materials/textures/descriptors/samplers/
normals/UVs/skins/animation/transforms/scene graph/metadata fields, renderer
behavior, `NativeVulkanRenderer.cpp`, public renderer API, runtime/product/
scene/server/draw-list API change, or gameplay/input/scripted-control/
final-state semantic change.

Native Static Mesh File Export Status Text Helper Extraction is complete as a
behavior-preserving status text cleanup. The central inline helper
`NativeStaticMeshFileExportStatusText(...)` now lives beside
`NativeStaticMeshFileExportStatus` in `NativeStaticMeshFileExport.hpp`. The
CLI-local `NativeStaticMeshFileExportStatusName(...)` switch was removed, and
the single-export and batch-export compact CLI failure paths now use the central
helper. Stable strings are `Exported`, `InvalidPolicy`, `UnknownAsset`,
`MissingOutputDirectory`, `OutputDirectoryNotDirectory`, `TargetAlreadyExists`,
`WriterFailed`, `FileOpenFailed`, `WriteFailed`, and fallback `Unknown`. Direct
file-export status tests cover every current status plus `Unknown`. Source
verification passed `native_static_mesh_file_export_tests`, `iggy_native_play`,
single-export collision smoke preserving
`static mesh export failed: TargetAlreadyExists`, batch-export collision smoke
preserving `static mesh batch export failed: TargetAlreadyExists`, and source
`git diff --check`. This packet does not change single/batch success output,
compact failure prefixes or status strings, export status assignment,
preflight/write order, issue counts, output path selection, sidecar writes,
no-overwrite/no-create-directory behavior, CLI parser/dispatch/conflicts,
verifier/package-directory/report/generated text behavior, CMake, fixtures,
assets, renderer/model-slot behavior, package loading/discovery, schema, or
glTF/glb/JSON parser work.

Native Static Mesh Single File Export Success Text Renderer Extraction is
complete as a behavior-preserving single-export stdout cleanup.
`NativeStaticMeshFileExport.hpp` now exposes pure
`BuildNativeStaticMeshFileExportSuccessText(std::string_view name, const
NativeStaticMeshFileExportResult &result)`, which serializes only the successful
single-file export compact stdout line:
`static-mesh-export name=<name> output=<path> bytes=<N>\n`.
`PrintNativeStaticMeshAssetFileExport(...)` delegates to the helper only after
the existing `Exported` status check. Unknown-asset failure text, non-`Exported`
failure text, and batch export success/failure text are preserved exactly. Exact
text coverage uses a real successful cube single export result. Source
verification passed `native_static_mesh_file_export_tests`, `iggy_native_play`,
a single-export smoke preserving
`static-mesh-export name=cube output=/tmp/iggy-native-single-export-renderer-123/cube.igmesh bytes=523`,
target-exists collision smoke preserving `static mesh export failed:
TargetAlreadyExists`, source `git diff --check`, and source
`git diff --cached --check`. This packet does not change export policy defaults,
export status assignment, write/preflight order, issue or byte count semantics,
output path selection, no-overwrite/no-create-directory behavior, CLI parser/
help/dispatch/conflict/exit behavior, unknown-asset failure text, non-`Exported`
failure text, batch export success/failure text, static model behavior,
manifest/package/verification/package-directory behavior, exact verification
behavior, generated sidecars, export write policy, package loading/discovery/
acceptance, `NativeVulkanRenderer.cpp`, renderer/model-slot behavior, checked-in
assets or fixtures, `.igmesh` schema/loading, or glTF/glb/JSON parser work.

Native Static Mesh Single File Export Failure Text Renderer Extraction is
complete as a behavior-preserving single-export stderr body cleanup.
`NativeStaticMeshFileExport.hpp` now exposes pure
`BuildNativeStaticMeshFileExportFailureText(const
NativeStaticMeshFileExportResult &result)`, which serializes only the
non-`Exported` single-file export failure message body:
`static mesh export failed: <Status> output=<path> issues=<N>`. The helper
excludes the `iggy_native_play:` prefix and trailing newline because the
existing exception/catch path still supplies both. `PrintNativeStaticMeshAssetFileExport(...)`
delegates the non-`Exported` branch through
`std::runtime_error(BuildNativeStaticMeshFileExportFailureText(result))` after
the unknown-asset special case, preserving `iggy_native_play: unknown static
mesh asset: nope`. Single-export success text remains routed through
`BuildNativeStaticMeshFileExportSuccessText(...)`, and batch export
success/failure text is unchanged. Exact helper coverage includes
target-exists and invalid-policy failure text. Source verification passed
`native_static_mesh_file_export_tests`, `iggy_native_play`, success smoke
preserving
`static-mesh-export name=cube output=/tmp/iggy-native-single-export-failure-renderer-124/cube.igmesh bytes=523`,
collision smoke preserving
`iggy_native_play: static mesh export failed: TargetAlreadyExists output=/tmp/iggy-native-single-export-failure-renderer-124/cube.igmesh issues=0`,
unknown-asset smoke preserving `iggy_native_play: unknown static mesh asset:
nope`, source `git diff --check`, and source `git diff --cached --check`. This
packet does not change unknown-asset failure text, single-export success text,
batch export success/failure text, export status assignment, write/preflight
order, issue or byte count semantics, output path selection, no-overwrite/
no-create-directory behavior, CLI parser/help/dispatch/conflict/exit behavior,
static model behavior, manifest/package/verification/package-directory
behavior, exact verification behavior, generated sidecars, export write policy,
package loading/discovery/acceptance, `NativeVulkanRenderer.cpp`, renderer/
model-slot behavior, checked-in assets or fixtures, `.igmesh` schema/loading,
or glTF/glb/JSON parser work.

Native Static Mesh Batch Export Success Text Renderer Extraction is complete as
a behavior-preserving batch stdout cleanup. `NativeStaticMeshFileExport.hpp` now
exposes pure `BuildNativeStaticMeshFileExportBatchSuccessText(const
NativeStaticMeshFileExportBatchResult &result)`, which serializes only the
successful batch export stdout line:
`static-mesh-export-batch output=<dir> exported=<N> bytes=<N> manifest=<path> manifestBytes=<N> packageManifest=<path> packageManifestBytes=<N>\n`.
`PrintNativeStaticMeshAssetBatchExport(...)` delegates to the helper only after
the existing `Exported` status check. Batch failure path/output selection and
compact failure text remain unchanged. Exact helper coverage uses a real
successful default batch export result. Source verification passed
`native_static_mesh_file_export_tests`, `iggy_native_play`, success smoke
matching `exported=3`, `bytes=33879`, `manifestBytes=272`, and
`packageManifestBytes=250`, file existence checks for `cube.igmesh`,
`bean.igmesh`, `npc-marker.igmesh`, `static-mesh-export-manifest.txt`, and
`static-mesh-export-package-manifest.txt`, collision smoke preserving
`iggy_native_play: static mesh batch export failed: TargetAlreadyExists output=/tmp/iggy-native-batch-success-renderer-125/static-mesh-export-manifest.txt issues=0`,
source `git diff --check`, and source `git diff --cached --check`. This packet
does not change batch failure text extraction, batch failure output or issue
selection, export status assignment, preflight/write order,
no-overwrite/no-create-directory behavior, sidecar filenames or content, byte
count semantics, CLI parser/help/dispatch/conflict/exit behavior, static model
behavior, package directory report behavior, exact verification behavior,
generated sidecars beyond existing export behavior, package loading/discovery/
acceptance, `NativeVulkanRenderer.cpp`, renderer/model-slot behavior, checked-in
assets or fixtures, `.igmesh` schema/loading, or glTF/glb/JSON parser work.

Native Static Mesh Batch Export Failure Text Renderer Extraction is complete as
a behavior-preserving batch stderr body cleanup. `NativeStaticMeshFileExport.hpp`
now exposes pure
`BuildNativeStaticMeshFileExportBatchFailureText(const
NativeStaticMeshFileExportBatchResult &result)`, which serializes only the batch
non-`Exported` failure body:
`static mesh batch export failed: <Status> output=<path> issues=<N>`. The
helper excludes the `iggy_native_play:` prefix and trailing newline because the
existing exception/catch path still supplies both. Output-path and issue-count
selection are preserved exactly: default output is `result.outputDirectory`,
manifest sidecar collision selects `manifestOutputPath`, package manifest
collision selects `packageManifestOutputPath`, and the first matching
non-`Exported` entry selects that entry output path when non-empty and adds that
entry issue count. `PrintNativeStaticMeshAssetBatchExport(...)` delegates the
non-`Exported` branch through
`std::runtime_error(BuildNativeStaticMeshFileExportBatchFailureText(result))`.
Batch success helper/output, single-export success/failure helpers, export
behavior, and app-level catch prefix are unchanged. Exact helper coverage
includes manifest sidecar collision, package sidecar collision, and asset target
collision. Source verification passed `native_static_mesh_file_export_tests`,
`iggy_native_play`, manifest collision smoke preserving exact failure text for
`static-mesh-export-manifest.txt`, package manifest collision smoke preserving
exact failure text for `static-mesh-export-package-manifest.txt`, asset
collision smoke preserving exact failure text for `bean.igmesh`, success smoke
preserving `exported=3`, `bytes=33879`, `manifestBytes=272`, and
`packageManifestBytes=250`, source `git diff --check`, and source
`git diff --cached --check`. This packet does not change batch success text,
single-export text, export data shape, export status assignment,
preflight/write order, no-overwrite/no-create-directory behavior, sidecar
filenames or content, byte count semantics, CLI parser/help/dispatch/conflict/
exit behavior, static model behavior, package directory diagnostics, exact
verification behavior, generated sidecars beyond existing export behavior,
package loading/discovery/acceptance, `NativeVulkanRenderer.cpp`, renderer/
model-slot behavior, checked-in assets or fixtures, `.igmesh` schema/loading,
or glTF/glb/JSON parser work.

Native Static Mesh Built-In Batch Export CLI is complete as a constrained
multi-file export path for the default built-in mesh policy:
`ExportNativeStaticMeshPolicyToDirectory(...)` exports every
`DefaultNativeStaticMeshExportPolicy()` asset to `DIR/defaultFilename`. The
batch helper preflights output directory existence/type and all target filenames
before writing, so common validation failures write no files. It returns batch
result/entry structs with aggregate status, output directory, exported count,
total byte count, issue count, and per-asset file export results.
`iggy_native_play --export-static-mesh-assets --output-dir DIR` writes
`cube.igmesh`, `bean.igmesh`, and `npc-marker.igmesh` and prints compact success
only, such as
`static-mesh-export-batch output=/tmp/iggy-native-export-batch-smoke exported=3 bytes=33879`.
Existing single-asset stdout and single-asset output-dir behavior are unchanged.
Parser conflicts are explicit: batch requires `--output-dir`, cannot combine
with `--dump-static-mesh-asset`, and cannot combine with
`--dump-static-model-load-report`. Re-running into existing output reports
`TargetAlreadyExists` for the first blocked target. This is not arbitrary
`--output PATH`, overwrite/force/delete/rename/temp-file replacement, in-place
canonicalization, checked-in fixture rewrite, production directory creation,
package discovery/scanning, registry/catalog/manifest expansion, source
mutation, glTF/glb/JSON parser/dependency work, `.igmesh` schema expansion,
material/texture/descriptor/sampler/normals/UV/animation/scene graph/metadata
fields, renderer behavior, `NativeVulkanRenderer.cpp`, public renderer API,
runtime/product/scene/server/draw-list API change, or gameplay/scripted/
final-state semantic change.

Native Static Mesh Export Report CLI is complete as a no-write diagnostics
surface: header-only app-local `NativeStaticMeshExportReport.hpp` adds
`BuildNativeStaticMeshExportReport(...)` over `DefaultNativeStaticMeshExportPolicy()`
or supplied policies. Report entries include export name, default filename,
built-in id, writable status, issue count, vertex count, index count, and writer
byte count. Aggregates include asset count, writable count, total bytes, and
total issues. `iggy_native_play --dump-static-mesh-export-report` prints the
report and exits before `NativeVulkanApp` construction or SDL/Vulkan startup.
Sample output is:
`static-mesh-export-report assets=3 writable=3 bytes=33879 issues=0`,
`asset=cube filename=cube.igmesh status=Writable vertices=8 indices=36 bytes=523 issues=0`,
`asset=bean filename=bean.igmesh status=Writable vertices=234 indices=1296 bytes=23882 issues=0`,
and
`asset=npc-marker filename=npc-marker.igmesh status=Writable vertices=98 indices=504 bytes=9474 issues=0`.
The report mode conflicts with `--output-dir`, `--export-static-mesh-assets`,
`--dump-static-mesh-asset`, and `--dump-static-model-load-report`. The report
path does not write files, validate output dirs/paths, touch renderer behavior,
inspect the filesystem, or change gameplay/scripted/final-state semantics. It
does not add arbitrary output paths, overwrite/force/create-directory policy,
checked-in fixture rewrites/canonicalization, package discovery/scanning,
registry/catalog/manifest expansion, source mutation, glTF/glb/JSON parser/
dependency work, `.igmesh` schema expansion, material/texture/descriptor/
sampler/normals/UV/animation/scene graph/metadata fields,
`NativeVulkanRenderer.cpp`, public renderer API, or runtime/product/scene/
server/draw-list API changes.

Native Static Mesh Export Report Status Text Helper Extraction is complete as a
behavior-preserving status text cleanup. The central inline helper
`NativeStaticMeshExportReportStatusText(...)` now lives beside
`NativeStaticMeshExportReportStatus` in `NativeStaticMeshExportReport.hpp`, and
export report row rendering uses the central helper. Stable strings are
`Writable`, `WriterFailed`, and fallback `Unknown`. Direct export report status
tests cover `Writable`, `WriterFailed`, and `Unknown` fallback. Source
verification passed `native_static_mesh_export_report_tests`,
`iggy_native_play`, exact CLI smoke for `--dump-static-mesh-export-report`, and
source `git diff --check`. The default export report output is preserved byte-
for-byte: summary row, cube/bean/npc-marker asset rows, row order,
`status=Writable`, vertices, indices, bytes, issues, and trailing newlines.
This packet does not change export report output, row order, counts, status
assignment, CLI parser/help/dispatch/conflicts, report construction,
writer/policy behavior, filesystem/write behavior, verifier/package-directory/
file export/manifest/package manifest behavior, CMake, fixtures, assets,
renderer/model-slot behavior, package loading/discovery, schema, or
glTF/glb/JSON parser work.

Native Static Mesh Export Report Text Renderer Extraction is complete as a
behavior-preserving report serialization cleanup. `NativeStaticMeshExportReport.hpp`
now exposes pure header-only `BuildNativeStaticMeshExportReportText(const
NativeStaticMeshExportReport &report)`. `PrintNativeStaticMeshExportReport(...)`
delegates to the renderer helper, and CLI output/exit behavior remains
unchanged. The renderer preserves the existing static mesh export report text
format exactly: summary row, entry row order, `asset=`, `filename=`, `status=`,
vertex/index/byte/issue counts, and trailing newlines. Source cleanup removed
no-longer-needed app-shell direct using declarations for export report
entry/status text rendering. Exact text tests cover the default three-row report
and a custom duplicate two-entry policy report. Source verification passed
`native_static_mesh_export_report_tests`, `iggy_native_play`, CLI smoke for
`--dump-static-mesh-export-report`, `rg` checks for the exact summary row and
all three default asset rows, source `git diff --check`, and source
`git diff --cached --check`. This packet does not change export policy defaults,
report data-building semantics, writer behavior, byte count semantics, CLI
parser/help/dispatch/conflict/exit behavior, static model behavior, manifest/
package/verification/package-directory behavior, exact verification behavior,
generated sidecars, export write policy, package loading/discovery/acceptance,
`NativeVulkanRenderer.cpp`, renderer/model-slot behavior, checked-in assets or
fixtures, `.igmesh` schema/loading, or glTF/glb/JSON parser work.

Native Static Mesh Export Policy Validation is complete as backend-free policy
guarding: `ValidateNativeStaticMeshExportPolicy(...)` now lives in
`NativeStaticMeshExportPolicy.hpp` with structured
`NativeStaticMeshExportPolicyValidationIssueCode` values for `EmptyName`,
`DuplicateName`, `EmptyDefaultFilename`, `DefaultFilenameContainsSeparator`, and
`DuplicateDefaultFilename`. `NativeStaticMeshExportPolicyValidationIssue` records
issue details, and `NativeStaticMeshExportPolicyValidationResult::valid()`
reports whether a policy is clean. Validation is filesystem-free and GPU-free.
The default export policy is unchanged: `cube` / `cube.igmesh`, `bean` /
`bean.igmesh`, and `npc-marker` / `npc-marker.igmesh`. `NativeStaticMeshFileExport.hpp`
now validates supplied policies before single or batch export performs asset
lookup, directory checks, target preflight, writer work, or writes.
`NativeStaticMeshFileExportStatus::InvalidPolicy` and native CLI status text are
available for invalid policy rejection; invalid single and batch exports return
`InvalidPolicy` with issue counts and write no files. Valid default export
report and valid default batch export output remain unchanged. This is not a
source/test/CMake/asset/shader/runtime change in the docs packet and does not
add package/export manifests, sidecar output, package discovery, registry/
catalog/manifest expansion, source mutation, authoring package policy, checked-
in fixture canonicalization, overwrite/force/create-directory/temp replacement
policy, renderer loading cleanup, `NativeVulkanRenderer.cpp` changes,
glTF/glb/JSON parser/dependency work, `.igmesh` schema/writer/loader/report
byte math changes, valid default CLI output changes, gameplay/scripted/
final-state behavior changes, or Linux/dGPU validation.

Native Static Mesh Export Manifest Text Builder is complete as a deterministic
no-write manifest surface: header-only app-local
`NativeStaticMeshExportManifest.hpp` adds
`BuildNativeStaticMeshExportManifestText(...)`. The builder validates the
supplied `NativeStaticMeshExportPolicy`, then uses
`BuildNativeStaticMeshExportReport(...)` for stable asset counts and writer byte
counts. `NativeStaticMeshExportManifestResult` reports `Built`,
`InvalidPolicy`, or `WriterFailed`. `iggy_native_play
--dump-static-mesh-export-manifest` prints deterministic manifest text and exits
before `NativeVulkanApp` construction, SDL startup, or Vulkan startup. Sample
output is:
`static-mesh-export-manifest version=1 assets=3 bytes=33879`,
`asset=cube filename=cube.igmesh vertices=8 indices=36 bytes=523`,
`asset=bean filename=bean.igmesh vertices=234 indices=1296 bytes=23882`, and
`asset=npc-marker filename=npc-marker.igmesh vertices=98 indices=504 bytes=9474`.
The manifest mode conflicts with `--output-dir`,
`--export-static-mesh-assets`, `--dump-static-mesh-export-report`,
`--dump-static-mesh-asset`, and `--dump-static-model-load-report`. Existing
valid export report and batch export output remain unchanged. This is not a
source/test/CMake/asset/shader/runtime change in the docs packet and does not
add sidecar file writes, package/export manifest files on disk, overwrite/
create-directory/temp-file policy, arbitrary output paths, checked-in fixture
canonicalization or rewrite, package discovery, directory scanning, registry/
catalog expansion, source mutation, material/texture/schema changes, JSON/
glTF/glb parser/dependency work, renderer behavior changes,
`NativeVulkanRenderer.cpp` changes, native app CMake source registration
changes, gameplay/scripted/final-state changes, or docs mixed into source.

Native Static Mesh Export Manifest Status Text Helper Extraction is complete as
a behavior-preserving status text cleanup. The central inline helper
`NativeStaticMeshExportManifestStatusText(...)` now lives beside
`NativeStaticMeshExportManifestStatus` in `NativeStaticMeshExportManifest.hpp`,
and `PrintNativeStaticMeshExportManifest()` compact failure text uses the
central helper. Stable strings are `Built`, `InvalidPolicy`, `WriterFailed`, and
fallback `Unknown`. Direct mesh export manifest status tests cover `Built`,
`InvalidPolicy`, `WriterFailed`, and `Unknown` fallback. Source verification
passed `native_static_mesh_export_manifest_tests`, `iggy_native_play`, exact CLI
smoke for `--dump-static-mesh-export-manifest`, and source `git diff --check`.
Successful `--dump-static-mesh-export-manifest` output is preserved byte-for-
byte for default built-ins: header, cube/bean/npc-marker rows, order, counts,
byte totals, and trailing newlines. This packet does not change compact failure
string shape or status text, builder validation/write semantics, `written()`
behavior, reader/file-reader behavior, generated sidecar content, CLI
parser/help/dispatch/conflicts, package manifest status/helper behavior,
package-directory/exact verification/file export/export report/export policy/
asset writer behavior, CMake, fixtures, assets, renderer/model-slot behavior,
package loading/discovery, schema, or glTF/glb/JSON parser work.

Native Static Mesh Export Manifest Failure Text Renderer Extraction is complete
as a behavior-preserving manifest stderr body cleanup.
`NativeStaticMeshExportManifest.hpp` now exposes pure
`BuildNativeStaticMeshExportManifestFailureText(const
NativeStaticMeshExportManifestResult &result)` beside the mesh export manifest
result/status boundary. The helper serializes only the failure message body:
`static mesh export manifest failed: <Status> issues=<N>`, excluding the
app-level `iggy_native_play:` prefix and trailing newline because the existing
exception/catch path still supplies both. `PrintNativeStaticMeshExportManifest()`
delegates the non-written failure body through the helper after the existing
`!result.written()` check. Successful manifest dumping still prints
`result.text` unchanged, and exact invalid-policy helper coverage was added.
Source verification passed `native_static_mesh_export_manifest_tests`,
`iggy_native_play`, successful manifest smoke matching the exact header and
cube/bean/npc-marker asset rows, source `git diff --check`, and source
`git diff --cached --check`. This packet does not change successful
`--dump-static-mesh-export-manifest` output, generated manifest text, manifest
build semantics, `written()` semantics, `NativeStaticMeshExportManifestResult`
data shape, status strings, issue count semantics, CLI parser/help/dispatch/
conflict/exit behavior, app-level error prefix/newline behavior, package
manifest failure extraction, package directory report behavior, exact
verification/report behavior, file export helpers, static model surfaces,
`NativeVulkanRenderer.cpp`, renderer/model-slot behavior, checked-in assets or
fixtures, `.igmesh` schema/loading, or glTF/glb/JSON parser work.

Native Static Mesh Batch Manifest Sidecar Export is complete for batch export
only: `ExportNativeStaticMeshPolicyToDirectory(...)` now writes policy mesh
files plus one manifest sidecar named exactly `static-mesh-export-manifest.txt`.
The sidecar content is exactly
`BuildNativeStaticMeshExportManifestText(policy).text`. Batch export preflights
the sidecar target before mesh writes and applies the same no-overwrite
`TargetAlreadyExists` policy; batch results now report `manifestOutputPath` and
`manifestByteCount`. CLI batch success output includes stable `manifest=...`
and `manifestBytes=...` fields, for example:
`static-mesh-export-batch output=/tmp/iggy-native-sidecar-smoke-78 exported=3 bytes=33879 manifest=/tmp/iggy-native-sidecar-smoke-78/static-mesh-export-manifest.txt manifestBytes=272`.
The sidecar content shape is:
`static-mesh-export-manifest version=1 assets=3 bytes=33879`,
`asset=cube filename=cube.igmesh vertices=8 indices=36 bytes=523`,
`asset=bean filename=bean.igmesh vertices=234 indices=1296 bytes=23882`, and
`asset=npc-marker filename=npc-marker.igmesh vertices=98 indices=504 bytes=9474`.
An existing sidecar target fails before mesh writes with a compact error such as
`iggy_native_play: static mesh batch export failed: TargetAlreadyExists output=/tmp/iggy-native-sidecar-existing-78/static-mesh-export-manifest.txt issues=0`.
Single-asset stdout and single-asset output-dir export remain unchanged; single
output-dir export writes no sidecar. This docs packet does not change source,
tests, CMake, assets, shaders, runtime, package discovery/scanning, registry/
catalog expansion, package semantics, source mutation, overwrite/force/create-
directory/temp replacement, checked-in fixture rewrites/canonicalization,
renderer behavior, `NativeVulkanRenderer.cpp`, JSON/glTF/glb parser/dependency
work, `.igmesh` schema expansion, materials/textures/normals/UVs/animation/
schema work, gameplay/scripted/final-state behavior, or next research/scout
implementation.

Native Static Mesh Export Directory Verification CLI is complete as a read-only
verification path: `VerifyNativeStaticMeshExportDirectory(policy, directory)`
validates the supplied export policy, requires an existing output directory,
compares `static-mesh-export-manifest.txt` exactly to
`BuildNativeStaticMeshExportManifestText(policy).text`, then loads only expected
policy files and checks vertex/index counts against the export report. Extra
unrelated files are ignored; no directory scanning/discovery semantics are
introduced. `iggy_native_play --verify-static-mesh-export --output-dir DIR`
prints `static-mesh-export-verify output=/tmp/iggy-native-verify-smoke-79 verified=3 manifest=ok`
on success and exits before `NativeVulkanApp` construction, SDL startup, or
Vulkan startup. Missing manifest failures report the manifest path, for example
`iggy_native_play: static mesh export verification failed: MissingManifest output=/tmp/iggy-native-verify-missing-79/static-mesh-export-manifest.txt issues=0`;
single-export directories fail the same way because they intentionally have no
sidecar. The verify mode conflicts with `--export-static-mesh-assets`,
`--dump-static-mesh-asset`, `--dump-static-mesh-export-report`,
`--dump-static-mesh-export-manifest`, and `--dump-static-model-load-report`.
This docs packet does not change source, tests, CMake, assets, shaders, runtime,
directory scanning/discovery beyond expected policy files, package discovery,
registry/catalog/package semantics, source mutation, overwrite/force/create-
directory/temp replacement policy, arbitrary output paths beyond existing
`--output-dir`, fixture rewrites/canonicalization, renderer behavior,
`NativeVulkanRenderer.cpp`, JSON/glTF/glb parser/dependency work, `.igmesh`
schema/material/texture/normal/UV/animation behavior, gameplay/scripted/
final-state behavior, or next research/scout implementation.

Native Static Mesh Export Directory Verification Status Text Helper Extraction
is complete as a behavior-preserving status text cleanup. The central inline
helper `NativeStaticMeshExportDirectoryVerificationStatusText(...)` now lives
beside `NativeStaticMeshExportDirectoryVerificationStatus` in
`NativeStaticMeshExportDirectoryVerification.hpp`. Verification report summary
and per-asset status rendering plus the two compact CLI failure paths now use
that helper after removing the report-local and CLI-local duplicate switches.
Stable strings are `Verified`, `InvalidPolicy`, `MissingOutputDirectory`,
`OutputDirectoryNotDirectory`, `MissingManifest`, `ManifestMismatch`,
`MissingAsset`, `AssetLoadFailed`, `GeometryMismatch`, `ManifestBuildFailed`,
`MissingPackageManifest`, `PackageManifestReadFailed`,
`PackageManifestMismatch`, `PackageManifestBuildFailed`, and fallback `Unknown`.
Direct verification tests cover every current status plus `Unknown`. Source
verification passed verification tests, verification report tests,
`iggy_native_play`, valid export verification smoke, missing-manifest
verification failure smoke, missing-manifest verification report failure smoke,
and source `git diff --check`. This packet does not change report text,
per-asset status text, compact CLI failure strings, CLI parser/dispatch/help/
success output, verifier logic, status ordering, issue counts, problem paths,
verified flags, entry data, sidecar matching, generated sidecar/export behavior,
package acceptance semantics, package-directory diagnostics, CMake, fixtures,
assets, renderer/model-slot behavior, package loading/discovery, schema, or
glTF/glb/JSON parser work.

Native Static Mesh Export Verification Success Text Renderer Extraction is
complete as a behavior-preserving verify stdout cleanup.
`NativeStaticMeshExportDirectoryVerification.hpp` now exposes pure
`BuildNativeStaticMeshExportDirectoryVerificationSuccessText(const NativeStaticMeshExportDirectoryVerificationResult &result)`,
which serializes only successful `--verify-static-mesh-export` stdout:
`static-mesh-export-verify output=<dir> verified=<N> manifest=ok packageManifest=ok\n`.
`PrintNativeStaticMeshExportDirectoryVerification(...)` delegates only the
existing success `std::cout` block to the helper after `result.verified()` is
known true. Verification failure rendering, problem-path selection, issue-count
semantics, status ordering, exact sidecar matching, package manifest read
diagnostics, CLI exit behavior, `VerifyNativeStaticMeshExportDirectory(...)`,
result data shape, `verified()` semantics, sidecar state semantics, and
app-level error prefix/newline behavior are unchanged. Exact helper coverage
uses a real verified default batch export result. Source verification passed
`native_static_mesh_export_directory_verification_tests`, `iggy_native_play`,
success smoke preserving
`static-mesh-export-verify output=/tmp/iggy-native-verify-success-renderer-129 verified=3 manifest=ok packageManifest=ok`,
failure smoke preserving
`iggy_native_play: static mesh export verification failed: MissingManifest output=/tmp/iggy-native-verify-failure-renderer-129/static-mesh-export-manifest.txt issues=0`,
source `git diff --check`, and source `git diff --cached --check`. This packet
does not change verification report/package-directory report behavior, built-in
asset dump behavior, file export behavior, manifest/package-manifest behavior,
CMake/docs mixing, ledger state, sidecar generation, export write policy,
package loading/discovery/acceptance, `NativeVulkanRenderer.cpp`, renderer/
model-slot behavior, checked-in assets or fixtures, `.igmesh` schema/loading, or
glTF/glb/JSON parser work.

Native Static Mesh Export Verification Failure Text Renderer Extraction is
complete as a behavior-preserving verify failure-body cleanup.
`NativeStaticMeshExportDirectoryVerification.hpp` now exposes pure
`BuildNativeStaticMeshExportDirectoryVerificationFailureText(const NativeStaticMeshExportDirectoryVerificationResult &result)`,
which serializes only the non-verified `--verify-static-mesh-export` failure
body:
`static mesh export verification failed: <Status> output=<path> issues=<N>`.
The helper excludes the app-level `iggy_native_play:` prefix and embedded
trailing newline, preserving the existing catch path ownership. Output path
selection remains `problemPath` when present and `outputDirectory` otherwise.
`PrintNativeStaticMeshExportDirectoryVerification(...)` throws using the helper
after `!result.verified()`, while success text remains routed through the
existing success helper. Source verification passed
`native_static_mesh_export_directory_verification_tests`, `iggy_native_play`,
missing manifest failure smoke, missing package manifest failure smoke,
unchanged verification success smoke, source `git diff --check`, and source
`git diff --cached --check`. This packet does not change verification report/
package-directory report behavior, verifier behavior/status ordering/result
shape, `VerifyNativeStaticMeshExportDirectory(...)`, `verified()` semantics,
status strings, problem-path setting, issue-count semantics, package manifest
read diagnostics, exact sidecar matching, CLI parser/help/dispatch/conflict/
exit behavior, app-level error prefix/newline behavior, built-in asset dumps,
file export, manifest/package-manifest behavior, CMake/docs mixing, ledger
state, sidecar generation, export write policy, package loading/discovery/
acceptance, `NativeVulkanRenderer.cpp`, renderer/model-slot behavior,
checked-in assets or fixtures, `.igmesh` schema/loading, or glTF/glb/JSON parser
work.

Native Static Mesh Export Verification Report CLI is complete as a read-only
report surface around `VerifyNativeStaticMeshExportDirectory(policy,
directory)`. `iggy_native_play
--dump-static-mesh-export-verification-report --output-dir DIR` exits before
`NativeVulkanApp` construction, SDL startup, or Vulkan startup. Success prints
a summary plus one row per asset in the default export policy, for example:
`static-mesh-export-verification-report status=Verified output=/tmp/iggy-native-verify-report-smoke-80.PBsYB2 verified=3 issues=0`,
`asset=cube filename=cube.igmesh status=Verified vertices=8 expectedVertices=8 indices=36 expectedIndices=36 issues=0`,
`asset=bean filename=bean.igmesh status=Verified vertices=234 expectedVertices=234 indices=1296 expectedIndices=1296 issues=0`, and
`asset=npc-marker filename=npc-marker.igmesh status=Verified vertices=98 expectedVertices=98 indices=504 expectedIndices=504 issues=0`.
Failed verification prints the report first, then exits nonzero through the
existing compact `iggy_native_play:` error style, for example:
`static-mesh-export-verification-report status=MissingManifest output=/tmp/iggy-native-verify-report-missing-80.EG7aZh verified=0 issues=0 problem=/tmp/iggy-native-verify-report-missing-80.EG7aZh/static-mesh-export-manifest.txt`
then
`iggy_native_play: static mesh export verification report failed: MissingManifest output=/tmp/iggy-native-verify-report-missing-80.EG7aZh/static-mesh-export-manifest.txt issues=0`.
Existing `--verify-static-mesh-export` output and behavior are preserved. The
report mode conflicts with `--verify-static-mesh-export`,
`--export-static-mesh-assets`, `--dump-static-mesh-asset`,
`--dump-static-mesh-export-report`, `--dump-static-mesh-export-manifest`, and
`--dump-static-model-load-report`, and requires `--output-dir`. This docs packet
does not change source, tests, CMake, assets, shaders, runtime,
`NativeVulkanRenderer.cpp`, renderer behavior, shader behavior, asset fixtures,
runtime/product/scene/server APIs, docs-in-source, native app CMake source
registration, writes/repair/scanning/package/catalog/parser/schema/material/
texture behavior, gameplay/scripted/final-state behavior, or next research/
scout implementation.

Native Static Mesh Export Package Policy is complete as a header-only,
app-local, value-only policy surface. `NativeStaticMeshExportPackagePolicy`
defines stable metadata for native static mesh export packages:
`NativeStaticMeshExportPackageFormatId =
"iggy:native-static-mesh-export-package"`,
`NativeStaticMeshExportPackageFormatVersion = 1`, and
`NativeStaticMeshExportPackageManifestFilename =
"static-mesh-export-manifest.txt"`. `DefaultNativeStaticMeshExportPackagePolicy()`
wraps `DefaultNativeStaticMeshExportPolicy()`. `ValidateNativeStaticMeshExportPackagePolicy(...)`
performs deterministic metadata validation only and does not access the
filesystem, mutate/export package directories, or verify package directories.
Validation issues cover `EmptyFormatId`, `UnsupportedFormatId`,
`UnsupportedVersion`, `EmptyManifestFilename`,
`ManifestFilenameContainsSeparator`,
`ManifestFilenameCollidesWithAssetFilename`, and `InvalidMeshExportPolicy` with
nested issue count surfaced. This docs packet does not change source, tests,
CMake, assets, shaders, runtime, `IggyNativePlay.cpp`,
`NativeVulkanRenderer.cpp`, renderer behavior, shader behavior, fixtures,
runtime/product/scene/server APIs, native app CMake source registration, CLI,
parser behavior, package discovery/scanning, package IO, overwrite/create-dir
policy, `.igmesh` schema, material/texture/normal/UV/animation behavior,
gameplay behavior, or next research/scout implementation.

Native Static Mesh Export Package Manifest Text Builder is complete as a
header-only, app-local, deterministic no-write manifest builder.
`NativeStaticMeshExportPackageManifest.hpp` adds
`NativeStaticMeshExportPackageManifestStatus { Built, InvalidPolicy }`,
`NativeStaticMeshExportPackageManifestResult { status, text, issueCount,
written() }`, and `BuildNativeStaticMeshExportPackageManifestText(const
NativeStaticMeshExportPackagePolicy &)`. The builder validates package policy
first with `ValidateNativeStaticMeshExportPackagePolicy(...)`; invalid policy
returns `InvalidPolicy`, issue count, and no text. It does not access the
filesystem or write files. `iggy_native_play
--dump-static-mesh-export-package-manifest` exits before `NativeVulkanApp`
construction, SDL startup, or Vulkan startup. Sample output is:
`static-mesh-export-package-manifest format=iggy:native-static-mesh-export-package version=1 manifest=static-mesh-export-manifest.txt assets=3`,
`asset=cube filename=cube.igmesh`, `asset=bean filename=bean.igmesh`, and
`asset=npc-marker filename=npc-marker.igmesh`. The CLI conflicts with
`--output-dir`, `--export-static-mesh-assets`, `--verify-static-mesh-export`,
`--dump-static-mesh-export-verification-report`,
`--dump-static-mesh-export-manifest`, `--dump-static-mesh-export-report`,
`--dump-static-mesh-asset`, and `--dump-static-model-load-report`. This docs
packet does not change source, tests, CMake, assets, shaders, runtime, package
file IO, reader/parser syntax, package verification integration, package
discovery/scanning/catalog/registry, write/repair behavior,
overwrite/create-dir policy, renderer behavior, model-slot binding, glTF/JSON
dependencies, `.igmesh` schema, fixtures, docs-in-source, gameplay/scripted/
final-state behavior, `NativeVulkanRenderer.cpp`, shader behavior,
runtime/product/scene/server APIs, renderer loading, native app CMake source
registration, or next research/scout implementation.

Native Static Mesh Export Package Manifest Status Text Helper Extraction is
complete as a behavior-preserving status text cleanup. The central inline helper
`NativeStaticMeshExportPackageManifestStatusText(...)` now lives beside
`NativeStaticMeshExportPackageManifestStatus` in
`NativeStaticMeshExportPackageManifest.hpp`, and
`PrintNativeStaticMeshExportPackageManifest()` compact failure text uses the
central helper. Stable strings are `Built`, `InvalidPolicy`, and fallback
`Unknown`. Direct package manifest status tests cover `Built`, `InvalidPolicy`,
and `Unknown` fallback. Source verification passed
`native_static_mesh_export_package_manifest_tests`, `iggy_native_play`, exact
CLI smoke for `--dump-static-mesh-export-package-manifest`, and source
`git diff --check`. Successful `--dump-static-mesh-export-package-manifest`
output is preserved byte-for-byte for default built-ins: header,
cube/bean/npc-marker rows, row order, format id, version, nested manifest
filename, asset count, and trailing newlines. This packet does not change
compact failure string shape or status text, builder validation/write semantics,
`written()` behavior, reader/file-reader behavior, generated package sidecar
content, CLI parser/help/dispatch/conflicts, mesh manifest helper behavior,
package-directory/exact verification/file export/export report/export policy/
asset writer behavior, CMake, fixtures, assets, renderer/model-slot behavior,
package loading/discovery, schema, or glTF/glb/JSON parser work.

Native Static Mesh Export Package Manifest Failure Text Renderer Extraction is
complete as a behavior-preserving package manifest stderr body cleanup.
`NativeStaticMeshExportPackageManifest.hpp` now exposes pure
`BuildNativeStaticMeshExportPackageManifestFailureText(const
NativeStaticMeshExportPackageManifestResult &result)` beside the package
manifest result/status boundary. The helper serializes only the package
manifest failure message body:
`static mesh export package manifest failed: <Status> issues=<N>`, excluding
the app-level `iggy_native_play:` prefix and trailing newline because the
existing exception/catch path still supplies both.
`PrintNativeStaticMeshExportPackageManifest()` delegates the non-written failure
body through the helper after the existing `!result.written()` check. Successful
package manifest dumping still prints `result.text` unchanged, and exact
invalid-policy helper coverage uses
`BuildNativeStaticMeshExportPackageManifestText(...)`. Source verification
passed `native_static_mesh_export_package_manifest_tests`, `iggy_native_play`,
successful package manifest smoke matching the exact summary row and
cube/bean/npc-marker asset rows, source `git diff --check`, and source
`git diff --cached --check`. This packet does not change successful
`--dump-static-mesh-export-package-manifest` output, generated package manifest
text, package manifest build semantics, `written()` semantics,
`NativeStaticMeshExportPackageManifestResult` data shape, status strings, issue
count semantics, CLI parser/help/dispatch/conflict/exit behavior, app-level
error prefix/newline behavior, mesh manifest failure text, file export helpers,
built-in asset dump writer failure behavior, verification/report/package-
directory code, static model surfaces, `NativeVulkanRenderer.cpp`, renderer/
model-slot behavior, checked-in assets or fixtures, `.igmesh` schema/loading,
or glTF/glb/JSON parser work.

Native Static Mesh Batch Package Manifest Sidecar Export is complete for batch
export only. `ExportNativeStaticMeshPolicyToDirectory(...)` now writes an
additional package sidecar named exactly
`static-mesh-export-package-manifest.txt`; the sidecar content is exactly
`BuildNativeStaticMeshExportPackageManifestText(packagePolicy).text`, with
`packagePolicy.meshPolicy` composed from the mesh policy passed to the batch
export. `NativeStaticMeshExportPackagePolicy::manifestFilename` remains
unchanged and continues to name the nested mesh export manifest
`static-mesh-export-manifest.txt`. Batch export preflights the package sidecar
target before asset and mesh-manifest writes. A pre-existing package sidecar
returns `TargetAlreadyExists`, preserves that file, and writes no meshes or mesh
manifest. Successful CLI output now includes `packageManifest=...` and
`packageManifestBytes=250` fields. Single export still writes only the selected
`.igmesh` and no sidecars; `--dump-static-mesh-export-package-manifest` remains
no-write and unchanged. The later package sidecar verification packet makes
verify and verification-report paths require the package sidecar. This docs packet does
not change source, tests, CMake, assets, shaders, runtime, package
parser/reader behavior, package discovery/scanning/catalog/registry, package
verification integration, exact-extra-file validation, overwrite/force/
create-dir/temp replacement/arbitrary output path behavior, single-export
sidecar behavior, fixture rewrites, renderer behavior, `NativeVulkanRenderer.cpp`,
model-slot binding, gameplay/scripted/final-state behavior, shader behavior,
native app source registration, `.igmesh` schema/material/texture/normal/UV/
animation behavior, glTF/glb/JSON parser dependencies, third-party dependencies,
docs-in-source, or next research/scout implementation.

Native Static Mesh Package Sidecar Verification is complete for the existing
read-only export-directory verifier. `VerifyNativeStaticMeshExportDirectory(...)`
now requires `static-mesh-export-package-manifest.txt` after checking the output
directory and exact mesh manifest text, and before asset geometry checks. The
expected package sidecar text is built read-only with
`BuildNativeStaticMeshExportPackageManifestText(...)` from a package policy
composed from the verification mesh policy. New package-specific statuses are
`MissingPackageManifest`, `PackageManifestMismatch`, and
`PackageManifestBuildFailed`, and status-to-text mappings are updated for both
the verification report helper and CLI error text. No new CLI flag was added:
existing `iggy_native_play --verify-static-mesh-export --output-dir DIR` and
`iggy_native_play --dump-static-mesh-export-verification-report --output-dir DIR`
now fail/report missing or mismatched package sidecars. Missing package sidecar
failure reports
`iggy_native_play: static mesh export verification failed: MissingPackageManifest output=/tmp/iggy-native-package-verify-missing-84.RIkcIc/static-mesh-export-package-manifest.txt issues=0`;
the report path prints
`static-mesh-export-verification-report status=MissingPackageManifest output=/tmp/iggy-native-package-verify-missing-84.RIkcIc verified=0 issues=0 problem=/tmp/iggy-native-package-verify-missing-84.RIkcIc/static-mesh-export-package-manifest.txt`
before the compact `iggy_native_play:` failure. Mismatched package sidecars
report `PackageManifestMismatch` with `issues=1`. Verification remains
read-only and explicit-directory only. This docs packet does not change source,
tests, CMake, assets, shaders, runtime, package parser/reader syntax, semantic
parsing of package manifest rows, discovery/scanning/catalog/registry,
exact-extra-file rejection, repair/loading, export write behavior, overwrite/
create-dir policy, CLI flags, single-export sidecars, fixture rewrites, renderer
behavior, `NativeVulkanRenderer.cpp`, `.igmesh` schema, glTF/glb/JSON
dependencies, docs-in-source, native app CMake source registration,
runtime/product/scene/server APIs, gameplay/scripted/final-state behavior, or
next research/scout implementation.

Native Static Mesh Verification Summary Sidecar Diagnostics is complete for the
existing read-only verification result and report surfaces.
`NativeStaticMeshExportDirectoryVerificationResult` now carries explicit sidecar
diagnostics: `manifestPath`, `packageManifestPath`, `manifestVerified`, and
`packageManifestVerified`. The verifier records the mesh manifest path before
mesh manifest checks and the package manifest path before package sidecar
checks. `manifestVerified` is set only after exact mesh manifest text match, and
`packageManifestVerified` is set only after exact package manifest sidecar text
match. Verification status ordering and failure behavior are unchanged. Report
summary rows now include compact sidecar states:
`manifest=... packageManifest=...`. `iggy_native_play
--verify-static-mesh-export` success output now includes `packageManifest=ok`
alongside existing `manifest=ok`, for example
`static-mesh-export-verify output=/tmp/iggy-native-verification-diag-ok-85.6tfWQS verified=3 manifest=ok packageManifest=ok`.
Successful report summaries include
`static-mesh-export-verification-report status=Verified output=/tmp/iggy-native-verification-diag-ok-85.6tfWQS verified=3 issues=0 manifest=ok packageManifest=ok`.
Missing package sidecar report summaries include
`manifest=ok packageManifest=missing`, while missing mesh manifest summaries
include `manifest=missing packageManifest=not-checked`. This docs packet does
not change source, tests, CMake, assets, shaders, runtime, package manifest
parser/reader semantics, discovery/scanning/catalog/registry behavior, export
write behavior, CLI flags, single-export sidecars, renderer behavior, shader
behavior, runtime/product/scene/server APIs, `.igmesh` schema, fixtures,
docs-in-source, or next research/scout implementation.

Native Static Mesh Package Manifest Text Reader is complete as a dependency-free,
filesystem-free in-memory reader for the current generated package manifest
text grammar. `NativeStaticMeshExportPackageManifest.hpp` now includes
`NativeStaticMeshExportPackageManifestDocument`,
`NativeStaticMeshExportPackageManifestAssetRow`,
`NativeStaticMeshExportPackageManifestReadIssueCode`,
`NativeStaticMeshExportPackageManifestReadIssue`,
`NativeStaticMeshExportPackageManifestReadResult`, and
`ReadNativeStaticMeshExportPackageManifestText(...)`. The parser accepts only
the generated header form
`static-mesh-export-package-manifest format=... version=... manifest=... assets=N`
and asset rows of `asset=NAME filename=FILENAME`. It validates format id,
version, nested manifest filename, asset count, basename-only filenames,
nonempty names/filenames, duplicate asset names, duplicate filenames,
unexpected lines, missing fields, extra tokens, and malformed rows. It does not
reconstruct `NativeStaticMeshExportPolicy` or built-in ids from manifest rows.
Tests roundtrip the generated default package manifest text through the reader
and cover empty input, bad header token, unsupported format/version, malformed
asset count, missing header/asset fields, extra header/asset tokens, malformed
asset rows, unexpected lines, asset count mismatch, duplicate names, duplicate
filenames, separator-containing asset filenames, and separator-containing nested
manifest filenames. This docs packet does not change source, tests, CMake,
assets, shaders, runtime, filesystem access, package sidecar file reading,
verification/report/export/CLI integration, package directory readers,
discovery/scanning/catalog/registry behavior, repair/loading, exact-extra-file
rejection, policy reconstruction from rows, export write behavior, CLI flags,
renderer behavior, schema behavior, fixture rewrites, docs-in-source, or next
research/scout implementation.

Native Static Mesh Package Manifest File Reader is complete as an explicit-file
wrapper over the package manifest text reader.
`ReadNativeStaticMeshExportPackageManifestFile(const std::filesystem::path &path)`
opens exactly the supplied path in binary mode, reads the full file, and
delegates to `ReadNativeStaticMeshExportPackageManifestText(...)`.
`NativeStaticMeshExportPackageManifestReadIssueCode::FileOpenFailed` was added;
on file-open failure the wrapper returns one issue with line `0` and token set
to `path.string()`. Existing text-reader grammar and malformed-input behavior
are unchanged. Tests cover generated default package manifest text written to a
temp file reading successfully while preserving parsed format, version, nested
manifest filename, and asset rows; missing file reporting exactly one
`FileOpenFailed` issue with line `0` and supplied path token; and malformed
readable files propagating text-reader issue codes without reporting
file-open failure. This docs packet does not change source, tests, CMake,
assets, shaders, runtime, package directory readers, directory summaries/
reports, loading, discovery/scanning/catalog/registry behavior, exact-extra-file
rejection, verification/report/export/CLI integration, policy reconstruction or
built-in id reconstruction from manifest rows, export write behavior, CLI
flags, renderer behavior, schema behavior, fixture rewrites, docs-in-source, or
next research/scout implementation.

Native Static Mesh Package Manifest Verification Reader Diagnostics is complete
for explicit-directory verification. `ReadNativeStaticMeshExportPackageManifestFile(...)`
is now wired into `VerifyNativeStaticMeshExportDirectory(...)` after package
sidecar existence and before exact deterministic text comparison. The verifier
adds `PackageManifestReadFailed` and `packageManifestReadIssueCount` on
`NativeStaticMeshExportDirectoryVerificationResult`. Malformed or unreadable
package sidecars return `PackageManifestReadFailed`, set `problemPath` to the
package sidecar, keep `manifestVerified=true`, keep
`packageManifestVerified=false`, and do not scan asset geometry. Parse-valid but
non-exact package sidecars still return `PackageManifestMismatch`; missing
package sidecars still return `MissingPackageManifest`; package sidecars are
still not checked when mesh manifest verification fails first. Malformed package
sidecar verifier failures report
`iggy_native_play: static mesh export verification failed: PackageManifestReadFailed output=/tmp/iggy-native-verify-reader-88-bad.8iYAKS/static-mesh-export-package-manifest.txt issues=1`.
Report summaries for malformed sidecars include
`static-mesh-export-verification-report status=PackageManifestReadFailed output=/tmp/iggy-native-verify-reader-88-bad.8iYAKS verified=0 issues=1 manifest=ok packageManifest=invalid problem=/tmp/iggy-native-verify-reader-88-bad.8iYAKS/static-mesh-export-package-manifest.txt`.
This preserves exact deterministic package text verification: parse-valid exact
mismatches remain `PackageManifestMismatch` with `packageManifest=mismatch`,
while valid exports still report `manifest=ok packageManifest=ok`. This docs
packet does not change source, tests, CMake, assets, shaders, runtime, package
directory readers/reports/loading, discovery/scanning/catalog/registry behavior,
export behavior, overwrite/create-directory/temp replacement/arbitrary output
path behavior, exact-extra-file rejection, repair behavior, CLI flags, semantic
package acceptance replacing exact text verification, policy/built-in id
reconstruction from package rows, renderer behavior, shader behavior,
runtime/product/scene/server APIs, `.igmesh` schema, fixtures, gameplay,
docs-in-source, or next research/scout implementation.

Native Static Mesh Verification Package Read Issue Rows are complete for
read-only verification reporting. `NativeStaticMeshExportDirectoryVerificationResult`
now carries structured package manifest read issues in
`packageManifestReadIssues` for `PackageManifestReadFailed` results.
`VerifyNativeStaticMeshExportDirectory(...)` copies
`readPackageManifest.issues`, sets `packageManifestReadIssueCount` from the
vector size, and keeps `issueCount` equal to that count. Existing
`PackageManifestReadFailed`, `packageManifest=invalid`,
`manifestVerified=true`, `packageManifestVerified=false`, empty asset entries,
and verification ordering are preserved. Parse-valid but exact-mismatched
package sidecars remain `PackageManifestMismatch` with no read issue rows;
missing package sidecars remain `MissingPackageManifest` with no read issue
rows; mesh manifest failures still prevent package sidecar read rows.
`BuildNativeStaticMeshExportDirectoryVerificationReport(...)` now emits
deterministic package manifest reader issue rows after the summary and before
asset rows when `packageManifestReadIssues` is non-empty, for example
`packageManifestReadIssue code=MalformedHeader line=1 token=static-mesh-export-package`.
Fresh valid export verification reports no read issue rows, malformed package
sidecar reports keep the compact verifier failure
`PackageManifestReadFailed ... issues=1`, and parse-valid exact mismatch
reports remain `PackageManifestMismatch` / `packageManifest=mismatch` without
read issue rows. This docs packet does not change source, tests, CMake, assets,
shaders, runtime, CLI flags, ParseArgs, usage text, package directory readers/
reports/loading, discovery/scanning/catalog/registry/source mutation/repair/
exact-extra-file rejection, export behavior, overwrite/create-directory/temp
replacement/arbitrary output path policy, semantic package acceptance replacing
exact deterministic text verification, policy/built-in id reconstruction from
package rows, renderer behavior, runtime/product/scene/server APIs, gameplay,
`.igmesh` schema, fixtures, dependencies, docs-in-source, or next research/
scout implementation.

Native Static Mesh Package Manifest Read Issue Text Helper Extraction is
complete as a behavior-preserving mapping cleanup. The new central inline helper
`NativeStaticMeshExportPackageManifestReadIssueCodeText(...)` lives in
`NativeStaticMeshExportPackageManifest.hpp`. Verification report and package-
directory report package manifest read issue rows now route through that helper
instead of duplicated local switches. The nested mesh manifest issue-code mapper
in `NativeStaticMeshExportPackageDirectoryReport.hpp` remains separate and
unchanged because it maps the separate
`NativeStaticMeshExportManifestReadIssueCode` enum. Direct package manifest tests
cover every current package manifest read issue enum string plus the `Unknown`
fallback. Source verification passed package manifest tests, verification report
tests, package-directory report tests, `iggy_native_play`, valid export smoke
with no package read issue rows in valid reports, and source `git diff --check`.
This docs packet does not change report text, row order/counts, reader behavior,
verifier behavior, package-directory read behavior, CLI behavior, exact sidecar
matching, generated sidecar/export behavior, package acceptance semantics,
package-directory reader/status behavior, verifier internals, package
loading/discovery, `.igmesh` loading beyond existing verifier behavior,
renderer/model-slot behavior, CMake, assets, fixtures, glTF/glb/JSON parser
work, or next source packet scope.

Native Static Mesh Export Verification Report Text Renderer Extraction is
complete as a behavior-preserving serialization split. The new pure helper
`BuildNativeStaticMeshExportDirectoryVerificationReportText(const NativeStaticMeshExportDirectoryVerificationReport &report)`
owns existing verification report text serialization. The full
`BuildNativeStaticMeshExportDirectoryVerificationReport(policy, directory)`
still calls `VerifyNativeStaticMeshExportDirectory(policy, directory)`, assigns
`report.text = BuildNativeStaticMeshExportDirectoryVerificationReportText(report)`,
and returns the same report surface. Report text is preserved byte-for-byte:
summary row, `status=`, `output=`, `verified=`, `issues=`, `manifest=`,
`packageManifest=`, optional `problem=`, package manifest read issue rows, asset
rows, row order, paths, tokens, counts, and trailing newlines. Focused parity
coverage compares renderer output to `report.text` for valid export, missing
mesh manifest, missing package manifest, malformed package sidecar, missing
asset, and geometry mismatch. Valid CLI smoke still reports `status=Verified`,
`manifest=ok packageManifest=ok`, and verified cube/bean/npc-marker rows.
Missing package sidecar and malformed package sidecar smokes keep existing
nonzero CLI behavior and report rows for `MissingPackageManifest` and
`PackageManifestReadFailed`. This docs packet does not change verification data,
status/order/issue counts, CLI exit behavior, exact sidecar matching,
generated sidecar/export behavior, package acceptance semantics, package
directory report behavior, package loading/discovery, `.igmesh` loading beyond
existing verifier behavior, renderer/model-slot behavior, CMake, assets,
fixtures, glTF/glb/JSON parser work, or next source packet scope.

Native Static Mesh Export Verification Report Data Builder Extraction is
complete as a behavior-preserving structured-data split. The new no-text helper
`BuildNativeStaticMeshExportDirectoryVerificationReportData(const NativeStaticMeshExportPolicy &policy, const std::filesystem::path &directory)`
constructs `NativeStaticMeshExportDirectoryVerificationReport`, assigns
`report.verification = VerifyNativeStaticMeshExportDirectory(policy, directory)`,
and returns with `report.text` empty. The full
`BuildNativeStaticMeshExportDirectoryVerificationReport(policy, directory)` now
calls the data builder, assigns
`report.text = BuildNativeStaticMeshExportDirectoryVerificationReportText(report)`,
and returns the same full report behavior. Report text and verification
semantics are preserved: status, verified count, issue count, problem path,
sidecar paths/states, package read issue count/rows, entries, entry
statuses/counts, CLI behavior, and failure behavior. Focused tests compare
data-builder structured fields against the full builder and assert empty data
builder text for valid export, missing mesh manifest, missing package manifest,
malformed package sidecar, missing asset, and geometry mismatch; existing
text-renderer parity checks remain intact. This docs packet does not change
verifier internals, verification status/order/issue counts, report text, CLI
behavior, exact sidecar matching, generated sidecar/export behavior, package
acceptance semantics, package directory report/reader behavior, package
loading/discovery, `.igmesh` loading beyond existing verifier behavior,
renderer/model-slot behavior, CMake, assets, fixtures, glTF/glb/JSON parser
work, or next source packet scope.

Native Static Mesh Verification Report Builder Parity Coverage is complete as a
test-only coverage packet. Existing data-builder/full-builder parity and
text-renderer parity assertions now cover missing output directory, file path
instead of directory, manifest mismatch, package manifest mismatch, corrupt
asset/load failure, and extra unrelated file ignored branches. The tests use the
existing `BuildNativeStaticMeshExportDirectoryVerificationReportData(...)`,
`ExpectDataBuilderMatchesFullReport(...)`, and `ExpectTextRendererMatches(...)`
helpers. Source verification passed focused verification report tests,
`iggy_native_play`, CLI smokes for valid export, missing package sidecar, and
malformed package sidecar, and source `git diff --check`. This packet changed no
production source, report text, CLI behavior, verifier behavior/status/counts/
rows, package read issue semantics, exact sidecar matching, generated sidecar/
export behavior, package acceptance semantics, package directory report/reader
behavior, package loading/discovery, `.igmesh` loading beyond existing verifier
behavior, renderer/model-slot behavior, CMake, assets, fixtures, glTF/glb/JSON
parser work, or next source packet scope.

Native Static Mesh Package Directory Reader is complete as a header-only,
explicit-directory package read helper. `NativeStaticMeshExportPackageDirectoryReader.hpp`
adds `NativeStaticMeshExportPackageDirectoryReadStatus`,
`NativeStaticMeshExportPackageDirectoryAsset`,
`NativeStaticMeshExportPackageDirectoryReadResult`, and
`ReadNativeStaticMeshExportPackageDirectory(const std::filesystem::path &directory)`.
`NativeStaticMeshExportPackageDirectoryReadStatusText(...)` also lives at this
reader/result boundary and returns stable strings `Read`, `MissingDirectory`,
`DirectoryNotDirectory`, `PackageManifestReadFailed`, and fallback `Unknown`.
The reader records the supplied directory, requires it to exist and be a
directory, composes `directory / static-mesh-export-package-manifest.txt`, and
delegates to `ReadNativeStaticMeshExportPackageManifestFile(...)`. On package
manifest read failure it returns `PackageManifestReadFailed`, copies package
manifest read issues, sets `issueCount`, and does not inspect the nested
manifest or asset files. On success it copies the parsed package manifest
document, projects `manifestPath = directory / document.manifestFilename`, and
projects each parsed asset row to `{ name, filename, directory / filename }` in
row order. `read()` returns true only for `Read`. Tests cover batch-exported
temp directory reads, missing directory, file path as `DirectoryNotDirectory`,
missing and malformed package sidecars as `PackageManifestReadFailed`, removed
nested mesh manifest not failing, removed declared mesh asset not failing, and
extra unrelated files being ignored. Direct status-text tests cover `Read`,
`MissingDirectory`, `DirectoryNotDirectory`, `PackageManifestReadFailed`, and
`Unknown` fallback. This does not compare package sidecar text
to generated default text, verify nested mesh manifest text, parse mesh export
manifests, load `.igmesh` assets, check geometry, check existence of nested
manifest or asset files, reconstruct export policy or built-in ids from package
rows, scan directories, reject extra files, integrate CLI/export/verification/
renderer/package loading, mutate source, change export behavior, change
overwrite/create-directory/temp replacement/arbitrary output path policy,
change runtime/product/scene/server APIs, change gameplay, add glTF/JSON
dependencies, change schema/fixtures/CMake/docs-in-source, or open the next
source packet.

Native Static Mesh Package Directory Report Builder is complete as a header-only
no-write report surface over `ReadNativeStaticMeshExportPackageDirectory(...)`.
`NativeStaticMeshExportPackageDirectoryReport.hpp` adds
`NativeStaticMeshExportPackageDirectoryReport`,
the reader-boundary `NativeStaticMeshExportPackageDirectoryReadStatusText(...)`
for summary/failure status text, central package manifest issue code text mapping
for report rows, and
`BuildNativeStaticMeshExportPackageDirectoryReport(const std::filesystem::path &directory)`.
The report summary includes status, explicit directory, asset count, issue
count, package manifest path when available, and nested mesh manifest path when
available. Successful reports emit one asset row per parsed package asset in row
order: `asset=<name> filename=<filename> path=<directory/filename>`. Package
manifest read failures emit deterministic
`packageManifestReadIssue code=... line=... token=...` rows. The report uses
only the package directory reader result and does not verify nested files or
inspect beyond the reader. Tests cover valid exported directory summaries and
three asset rows, missing directory summary-only output, file-not-directory
summary-only output, missing package sidecar `PackageManifestReadFailed` with
`FileOpenFailed`, malformed package sidecar parser issue rows, removed nested
mesh manifest still reporting `Read` with projected manifest path, removed
declared mesh asset still reporting `Read` with projected asset path, and extra
unrelated files not appearing in output. This does not change CLI flags,
ParseArgs, usage text, native app behavior, verification/export/package-loading
integration, package loading, renderer integration, model-slot expansion,
discovery/scanning/catalog/registry/source mutation/repair/exact-extra-file
rejection, export behavior, overwrite/create-directory/temp replacement/
arbitrary output path policy, semantic package acceptance, policy
reconstruction, mesh manifest parsing, `.igmesh` loading, geometry checks,
schema/material/texture/normal/UV/animation expansion, glTF/JSON dependencies,
fixture rewrites, runtime/product/scene/server APIs, gameplay, source/test/
CMake/assets/shader/runtime files in this docs packet, or next source packet
scope.

Native Static Mesh Package Directory Report CLI is complete for the native
no-Qt app shell. `iggy_native_play` now accepts
`--dump-static-mesh-export-package-directory-report`; the flag requires
`--output-dir DIR` and is included in the output-dir allow-list. It conflicts
with `--dump-static-model-load-report`,
`--dump-static-mesh-export-package-manifest`,
`--dump-static-mesh-export-verification-report`, `--verify-static-mesh-export`,
`--export-static-mesh-assets`, `--dump-static-mesh-asset`,
`--dump-static-mesh-export-report`, and
`--dump-static-mesh-export-manifest`. Help text includes the new flag. Early
dispatch calls `BuildNativeStaticMeshExportPackageDirectoryReport(outputDir)`,
writes `report.text` to stdout, and returns success only when `report.readOk()`
is true. On non-`Read`, it prints report text first, then exits nonzero through
the existing `iggy_native_play:` error path with compact status/output/issues.
Dispatch happens before `NativeVulkanApp` construction and before SDL/Vulkan
startup. Valid export/report smoke output includes `status=Read`, `assets=3`,
`issues=0`, package manifest path, nested manifest path, and three asset rows.
Missing and malformed package sidecar smokes fail nonzero after report text with
`PackageManifestReadFailed` and package read issue rows. Missing nested
`static-mesh-export-manifest.txt` still exits zero with `status=Read` and
projected paths, so the CLI report remains non-verifying. This does not change
package loading, verification semantics, scanning/discovery, export mutation,
renderer behavior, source parser/dependency work, package report builder,
`NativeVulkanRenderer.cpp`, model-slot expansion, package discovery/scanning/
catalog/registry, source mutation, repair, exact-extra-file rejection, export
behavior, write policy, deterministic verification replacement,
export-policy/built-in id reconstruction, `.igmesh` schema/material/texture/
normal/UV/animation expansion, gameplay/scripted/final-state behavior, fixture
rewrites, docs-in-source, CMake, source/test/assets/shader/runtime files in this
docs packet, or next source packet scope.

Native Static Mesh Package Directory Presence Diagnostics are complete for the
package directory report. The report now emits read-only presence facts for
paths already projected by the package directory reader: summary rows append
`manifestExists=1|0` when the nested mesh manifest path is available, and asset
rows append `exists=1|0` for each declared package asset path. `readOk()` and
CLI exit semantics are unchanged: missing nested mesh manifest or declared mesh
asset remains `status=Read` / exit 0, while a missing package sidecar remains
`PackageManifestReadFailed` / nonzero. Valid package directory smoke prints
`manifestExists=1` plus `exists=1` for `cube`, `bean`, and `npc-marker`; missing
nested `static-mesh-export-manifest.txt` prints `status=Read` and
`manifestExists=0`; missing declared `cube.igmesh` prints `exists=0` for cube
and `exists=1` for remaining assets; missing package sidecar still reports
`PackageManifestReadFailed` and a `FileOpenFailed` issue row. This does not add
nested mesh manifest parsing, `.igmesh` loading, generated-text comparison,
directory scanning, policy reconstruction, CLI parser changes, export behavior
changes, renderer changes, docs-in-source, CMake changes, package discovery/
scanning/catalog/registry, exact-extra-file rejection, repair, source mutation,
arbitrary package loading, geometry checks, built-in id reconstruction, export
write behavior, overwrite/create-dir/temp replacement/arbitrary output path
policy, fixture/generated asset changes, gameplay/scripted/final-state changes,
`NativeVulkanRenderer.cpp`, glTF/glb/JSON parser/dependency work, source/test/
assets/shader/runtime files in this docs packet, or next source packet scope.

Native Static Mesh Package Directory File Fact Diagnostics are complete for the
package directory report. The report now adds read-only filesystem facts for
already-known package paths: summary rows include package sidecar facts
`packageManifestExists=1|0 packageManifestRegularFile=1|0 packageManifestBytes=N`;
summary rows keep nested mesh manifest presence and add
`manifestRegularFile=1|0 manifestBytes=N`; asset rows keep `exists=1|0` and add
`regularFile=1|0 bytes=N`. Facts use non-throwing `std::filesystem` status and
file-size calls; missing paths, directories, and size failures report
`regularFile=0 bytes=0`. `readOk()` and CLI exit semantics are unchanged:
missing nested mesh manifest, missing declared asset, and directory-at-asset
remain `status=Read` / exit 0; missing package sidecar remains
`PackageManifestReadFailed` / nonzero after report text. Valid package directory
smoke prints regular-file byte counts for package manifest `250`, nested
manifest `272`, cube `523`, bean `23882`, and npc-marker `9474`. Missing nested
mesh manifest prints `manifestExists=0 manifestRegularFile=0 manifestBytes=0`;
missing declared `cube.igmesh` prints `exists=0 regularFile=0 bytes=0`;
directory-at-asset prints `exists=1 regularFile=0 bytes=0`; missing package
sidecar prints
`packageManifestExists=0 packageManifestRegularFile=0 packageManifestBytes=0`
with the existing `FileOpenFailed` issue row. This does not add CLI changes,
renderer changes, verification/export behavior changes, docs-in-source, CMake
changes, runtime/product changes, shader changes, glTF/JSON/parser changes,
schema-scope changes, package discovery/scanning/catalog/registry,
exact-extra-file rejection, repair, source mutation, arbitrary package loading,
nested mesh manifest parsing, `.igmesh` loading, geometry checks,
generated-text comparison, policy/built-in id reconstruction, export write
behavior, overwrite/create-dir/temp replacement/arbitrary output path policy,
fixture/generated asset changes, gameplay/scripted/final-state behavior,
`NativeVulkanRenderer.cpp`, source/test/assets/shader/runtime files in this
docs packet, or next source packet scope.

Native Static Mesh Export Manifest Text Reader is complete as an in-memory,
dependency-free reader for the current generated native static mesh export
manifest grammar only. `NativeStaticMeshExportManifest.hpp` now includes
`NativeStaticMeshExportManifestAssetRow`,
`NativeStaticMeshExportManifestDocument`,
`NativeStaticMeshExportManifestReadIssueCode`,
`NativeStaticMeshExportManifestReadIssue`,
`NativeStaticMeshExportManifestReadResult`, and
`ReadNativeStaticMeshExportManifestText(std::string_view)`. The accepted grammar
is exactly header `static-mesh-export-manifest version=1 assets=N bytes=N` plus
rows `asset=NAME filename=FILENAME vertices=V indices=I bytes=B`. The reader
validates empty input, malformed header, unsupported version, malformed asset
and byte counts, missing fields, extra tokens, malformed or unexpected rows,
basename-only filenames, unsigned numeric row facts, duplicate asset names and
filenames, asset count mismatch, and total byte count mismatch. Generated
default manifest readback was tested against builder output and report facts,
preserving version, asset count, byte count, row order, and row facts. Existing
`iggy_native_play --dump-static-mesh-export-manifest` output remains
`static-mesh-export-manifest version=1 assets=3 bytes=33879` with cube
8/36/523, bean 234/1296/23882, and npc-marker 98/504/9474 rows. This reader is
separate from package-directory/report/verification integration and does not
reconstruct `NativeStaticMeshExportPolicy` or built-in ids from mesh manifest
rows, load `.igmesh` assets, validate geometry, or change CLI/export behavior.

Native Static Mesh Export Manifest File Reader is complete as an explicit-file
wrapper over the mesh export manifest text reader. `NativeStaticMeshExportManifest.hpp`
now adds `NativeStaticMeshExportManifestReadIssueCode::FileOpenFailed` and
`ReadNativeStaticMeshExportManifestFile(const std::filesystem::path &path)`.
The wrapper opens exactly the supplied path in binary mode; on open failure it
returns one `FileOpenFailed` issue with `line=0` and `token=path.string()`. On
open success it reads the full file into memory and delegates unchanged to
`ReadNativeStaticMeshExportManifestText(...)`, so existing text-reader behavior
and generated manifest output are unchanged. Tests cover generated-file read
success, missing explicit file `FileOpenFailed`, and propagation of text-reader
issues from a readable malformed file. This docs packet does not add
package-directory reader/report integration, verification/report/export/CLI
behavior changes, package loading/discovery/scanning/catalog/registry, semantic
package acceptance, exact-extra-file rejection, repair behavior, default path
composition, directory traversal, package-directory reads, policy/built-in id
reconstruction from mesh manifest rows, nested mesh manifest report rows,
`.igmesh` loading, geometry validation, schema/material/texture/normal/UV/
animation expansion, renderer behavior, `NativeVulkanRenderer.cpp`, model-slot
expansion, runtime/product/scene/server APIs, gameplay/scripted/final-state
behavior, fixture/generated asset changes, CMake changes, glTF/glb/JSON
dependencies/parsers, write-policy changes, docs-in-source, or next source
packet scope.

Native Static Mesh Export Manifest Read Issue Text Helper Extraction is complete
as the mesh export manifest counterpart to the package manifest helper
extraction. The new central inline helper
`NativeStaticMeshExportManifestReadIssueCodeText(...)` lives beside the mesh
export manifest read enum and reader in `NativeStaticMeshExportManifest.hpp`.
Package-directory report nested mesh `manifestReadIssue` rows now route through
that central mesh manifest helper after the redundant local mapper was removed
from `NativeStaticMeshExportPackageDirectoryReport.hpp`. This stays separate
from packet 110's package manifest helper because it maps
`NativeStaticMeshExportManifestReadIssueCode`, not the package manifest read
enum. Direct mesh export manifest tests cover every current read issue enum
string plus the `Unknown` fallback. Source verification passed mesh export
manifest tests, package-directory report tests, `iggy_native_play`, a missing
nested mesh manifest smoke that still reports
`manifestReadIssue code=FileOpenFailed`, and source `git diff --check`. This
packet does not change package-directory report text, summary fields,
`manifestReadIssue` rows, tokens, counts, row order, trailing newlines,
`readOk()`, CLI exit behavior, reader behavior, exact verification, generated
sidecar/export behavior, package acceptance semantics, package manifest helper
behavior, package-directory reader/status behavior, verifier internals, package
loading/discovery, `.igmesh` loading beyond existing verifier behavior,
renderer/model-slot behavior, CMake, assets, fixtures, glTF/glb/JSON parser
work, or next source packet scope.

Native Static Mesh Package Directory Manifest Read Diagnostics are complete for
the package directory report. The report now parses the already-projected nested
mesh manifest path with `ReadNativeStaticMeshExportManifestFile(...)`, but only
when package directory read succeeds. Summary rows append nested manifest read
diagnostics when `manifestPath` is available: valid/readable manifests print
`manifestRead=ok manifestReadIssues=0`, while missing, unreadable, or malformed
manifests print `manifestRead=invalid manifestReadIssues=N`. The report emits
deterministic nested manifest read issue rows after existing
`packageManifestReadIssue` rows and before asset rows, using
`manifestReadIssue code=<CodeText> line=<line> token=<token>`.
`NativeStaticMeshExportPackageDirectoryReport::readOk()` is unchanged and still
wraps package directory read status only. Missing or malformed nested mesh
manifests remain diagnostic-only: package directory report output remains
`status=Read`, CLI exits 0, and package asset rows remain present. Valid smoke
printed `manifestRead=ok manifestReadIssues=0` plus existing package, manifest,
and asset file facts. Missing nested manifest smoke exited 0 and printed
`manifestRead=invalid manifestReadIssues=1` plus
`manifestReadIssue code=FileOpenFailed line=0 token=<manifest path>`.
Malformed nested manifest smoke exited 0 and printed
`manifestRead=invalid manifestReadIssues=1` plus
`manifestReadIssue code=UnsupportedVersion line=1 token=2`. This docs packet
does not add package directory reader status/data changes, verification/export
behavior changes, CLI changes, exact deterministic verification replacement,
semantic package acceptance, generated-text comparison, package acceptance
validation, package-vs-mesh row comparisons, policy/built-in id reconstruction,
`.igmesh` loading, geometry validation, package loading,
discovery/scanning/catalog/registry, exact-extra-file rejection, repair, source
mutation, write behavior, renderer behavior,
`NativeVulkanRenderer.cpp`, model-slot expansion, runtime/product/scene/server
APIs, gameplay/scripted/final-state behavior, fixture/generated asset changes,
CMake changes, glTF/glb/JSON dependencies/parsers, schema/material/texture/
normal/UV/animation expansion, docs-in-source, or next source packet scope.

Native Static Mesh Package Directory Manifest Asset Rows are complete as
diagnostic row projection in the package directory report. When the projected
nested mesh manifest reads successfully, the report emits parsed nested mesh
manifest asset rows after any `manifestReadIssue` block and before existing
package-declared `asset=` rows. The row shape is
`manifestAsset=<name> filename=<filename> vertices=<vertexCount> indices=<indexCount> bytes=<byteCount>`.
Valid batch export reports three rows:
`manifestAsset=cube filename=cube.igmesh vertices=8 indices=36 bytes=523`,
`manifestAsset=bean filename=bean.igmesh vertices=234 indices=1296 bytes=23882`,
and
`manifestAsset=npc-marker filename=npc-marker.igmesh vertices=98 indices=504 bytes=9474`.
Missing or malformed nested mesh manifests remain diagnostic-only: no
`manifestAsset=` rows are emitted, package asset rows remain present,
`status=Read` and CLI exit 0 are preserved. This diagnostics surface does not
load `.igmesh`, validate geometry, traverse asset paths from mesh-manifest rows,
or add file facts for mesh-manifest-declared filenames.

Native Static Mesh Package Directory Manifest Row Comparison Diagnostics are
complete as diagnostic-only comparison output in the package directory report.
When the nested mesh manifest reads successfully, the report compares package
sidecar asset rows to parsed nested mesh manifest rows by asset name and
filename only. Summary rows append
`manifestMatches=N manifestMismatches=N manifestComparisonIssues=N`; valid
default batch export reports
`manifestMatches=3 manifestMismatches=0 manifestComparisonIssues=0` and no
`manifestComparison` rows. Deterministic mismatch rows are emitted after
`manifestAsset=` rows and before package `asset=` rows:
`manifestComparison code=MissingFromManifest asset=<packageName> packageFilename=<packageFilename>`,
`manifestComparison code=MissingFromPackage manifestAsset=<manifestName> manifestFilename=<manifestFilename>`,
and
`manifestComparison code=FilenameMismatch asset=<name> packageFilename=<packageFilename> manifestFilename=<manifestFilename>`.
Missing-from-manifest smoke exited 0 and printed
`manifestComparison code=MissingFromManifest asset=npc-marker packageFilename=npc-marker.igmesh`.
Missing-from-package smoke exited 0 and printed
`manifestComparison code=MissingFromPackage manifestAsset=extra manifestFilename=extra.igmesh`.
Filename-mismatch smoke exited 0 and printed
`manifestComparison code=FilenameMismatch asset=cube packageFilename=cube.igmesh manifestFilename=cube-renamed.igmesh`.
Missing or malformed nested manifests emit no comparison summary or rows and
preserve existing `manifestReadIssue` diagnostics. This docs packet does not add
package acceptance, verification, nonzero CLI behavior, issue-count/status
semantics, exact deterministic verification replacement, generated-text
comparison, semantic package acceptance, package acceptance validation, package
directory reader status/data changes, `readOk()` changes, CLI exit behavior
changes, policy/built-in id reconstruction, `.igmesh` loading, geometry
validation, asset path traversal from mesh-manifest rows, file facts for
mesh-manifest-declared filenames, package loading, discovery/scanning/catalog/registry,
exact-extra-file rejection, repair, source mutation, write behavior, renderer
behavior, `NativeVulkanRenderer.cpp`, model-slot expansion,
runtime/product/scene/server APIs, gameplay/scripted/final-state behavior,
fixture/generated asset changes, CMake changes, glTF/glb/JSON
dependencies/parsers, schema/material/texture/normal/UV/animation expansion,
docs-in-source, or next source packet scope.

Native Static Mesh Package Directory Comparison Issue Count Diagnostics are
complete as summary text in the package directory report. When nested manifest
comparison runs (`manifestRead=ok`), summary rows now include
`manifestComparisonIssues=N`; the value equals the number of emitted
`manifestComparison` rows and matches `manifestComparisons.size()`. Valid
default export prints
`manifestMatches=3 manifestMismatches=0 manifestComparisonIssues=0`, emits no
comparison rows, and keeps existing `manifestAsset=` and package `asset=` rows.
One-row mismatch cases print `manifestComparisonIssues=1` while preserving
existing row text and order for `MissingFromManifest`, `MissingFromPackage`, and
`FilenameMismatch`. Missing or malformed nested manifests still emit no
comparison summary or rows and preserve existing
`manifestRead=invalid manifestReadIssues=N` diagnostics. This docs packet does
not change core `issues=`, `report.read.issueCount`, package directory reader
status/data, `readOk()`, CLI exit behavior, verification behavior, package
acceptance, generated sidecar text, export behavior, renderer behavior,
docs-in-source, CMake, assets, fixtures, `.igmesh` loading, geometry validation,
discovery/scanning, write policy, or next source packet scope.

Native Static Mesh Package Directory Manifest Comparison Helper is complete as a
behavior-preserving extraction. Package-vs-nested-mesh-manifest row comparison
now lives in `NativeStaticMeshExportPackageDirectoryManifestComparisonResult` and
`CompareNativeStaticMeshExportPackageDirectoryManifestRows(...)`. The helper
preserves package-order `MissingFromManifest` and `FilenameMismatch` rows first,
then manifest-order `MissingFromPackage` rows. Report output text/order,
`manifestMatches`, `manifestMismatches`, `manifestComparisonIssues`, `readOk()`,
CLI exit behavior, package directory reader status/data, and verification/export
behavior are unchanged. Valid CLI smoke still prints
`manifestMatches=3 manifestMismatches=0 manifestComparisonIssues=0`, no
`manifestComparison` rows, and existing `manifestAsset=` plus package `asset=`
rows. Combined mismatch coverage verifies `manifestMatches=1
manifestMismatches=3 manifestComparisonIssues=3` with row order
`FilenameMismatch`, `MissingFromManifest`, `MissingFromPackage`. Missing or
malformed nested manifest smokes still print
`manifestRead=invalid manifestReadIssues=1`, no `manifestAsset=`, no
`manifestComparison`, and no comparison summary fields. This docs packet does
not change docs-in-source, `IggyNativePlay.cpp`, CMake, package directory reader
status/data, manifest reader behavior, verification/export behavior, `readOk()`,
CLI exit behavior, core `issues=`, package acceptance/semantic verification,
scanning/discovery, `.igmesh` loading, geometry validation, policy/built-in
reconstruction, renderer behavior, fixtures/generated assets, glTF/glb/JSON
parser work, or next source packet scope.

Native Static Mesh Package Directory Structured Manifest Diagnostics are
complete as a structured-data exposure with unchanged report text. The
`NativeStaticMeshExportPackageDirectoryReport` now carries
`manifestReadAttempted`, `manifestRead`, and `manifestComparison`, and the
builder populates those fields instead of keeping nested manifest read and
comparison diagnostics as local-only variables. `manifestReadAttempted` is true
only when package directory read succeeds and a projected nested manifest path is
available. `manifestRead` stores
`ReadNativeStaticMeshExportManifestFile(report.read.manifestPath)` when
attempted. `manifestComparison` is populated only when
`manifestReadAttempted && manifestRead.read()`; otherwise it remains
default/empty. Focused assertions cover valid export, combined mismatch,
missing/malformed nested manifest, and missing/malformed package sidecar paths.
Valid CLI smoke still has unchanged `manifestMatches=3 manifestMismatches=0
manifestComparisonIssues=0`, no `manifestComparison` rows, and existing
`manifestAsset=` plus package `asset=` rows. Combined mismatch output remains
`manifestMatches=1 manifestMismatches=3 manifestComparisonIssues=3` with row
order `FilenameMismatch`, `MissingFromManifest`, `MissingFromPackage`. Missing
or malformed nested manifests still print
`manifestRead=invalid manifestReadIssues=1`, no `manifestAsset=`, no
`manifestComparison`, and no comparison summary fields. This docs packet does
not change report text, rows, summary fields, statuses, `readOk()`, CLI exit
behavior, core `issues=`, package directory reader/status data, manifest reader
behavior, verification/export behavior, package acceptance/semantic
verification, scanning/discovery, `.igmesh` loading, geometry validation,
policy/built-in reconstruction, renderer behavior, fixtures/generated assets,
glTF/glb/JSON parser work, or next source packet scope.

Native Static Mesh Package Directory Structured File Facts are complete as a
structured-data exposure with unchanged report text. The report now has
`NativeStaticMeshExportPackageDirectoryPathFacts`,
`ReadNativeStaticMeshExportPackageDirectoryPathFacts(...)`, and
`NativeStaticMeshExportPackageDirectoryAssetFacts`, plus stored fields on
`NativeStaticMeshExportPackageDirectoryReport`: `packageManifestFactsRecorded`,
`packageManifestFacts`, `manifestFactsRecorded`, `manifestFacts`, and
`assetFacts`. The builder replaces its local path-facts lambda with the
structured helper and renders the same existing text from the stored facts.
`packageManifestFactsRecorded` / `packageManifestFacts` are populated when
`report.read.packageManifestPath` is available, including missing or malformed
package sidecar cases. `manifestFactsRecorded` / `manifestFacts` are populated
when `report.read.manifestPath` is available, including missing or malformed
nested manifest cases. `assetFacts` mirrors `report.read.assets` in
package-declared row order and stores facts only for package-declared asset
paths. Nested mesh-manifest-declared-only rows do not receive file facts.
Focused coverage includes valid export, missing package sidecar, malformed
package sidecar, missing nested mesh manifest, missing declared package asset,
and directory-at-declared-asset. Report text output is unchanged: no new rows,
summary fields, row-order changes, statuses, counts, tokens, or newline changes.
This docs packet does not change `readOk()`, CLI exit behavior, core `issues=`,
package directory reader status/data, exact verification/export/package
acceptance, generated sidecar text, write policy, nested mesh-manifest-only path
facts, `.igmesh` loading, geometry validation, policy/built-in reconstruction,
discovery/scanning/catalog/registry, exact-extra-file rejection, renderer/model
slot/schema/gameplay behavior, glTF/glb/JSON parser work, or next source packet
scope.

Native Static Mesh Package Directory Report Text Renderer Extraction is complete
as a behavior-preserving serialization split. The new header-only helper
`BuildNativeStaticMeshExportPackageDirectoryReportText(const NativeStaticMeshExportPackageDirectoryReport &report)`
owns existing package directory report text serialization, while
`BuildNativeStaticMeshExportPackageDirectoryReport(...)` still collects
structured report data and then assigns
`report.text = BuildNativeStaticMeshExportPackageDirectoryReportText(report)`.
The renderer consumes existing structured fields only: `read`,
`packageManifestFacts`, `manifestFacts`, package manifest read issues,
`manifestRead`, `manifestComparison`, and `assetFacts`. Current report text is
preserved byte-for-byte for summary rows, field order, issue rows, manifest
asset rows, comparison rows, package asset rows, counts, tokens, paths, and
newlines. Focused tests compare renderer output to `report.text` for valid
export, missing package sidecar, malformed package sidecar, missing nested
manifest, combined comparison mismatch, and missing declared asset. This docs
packet does not change report text, rows, summary fields, row order, statuses,
counts, tokens, newlines, `readOk()`, CLI exit behavior, core `issues=`,
package directory reader status/data, exact verification/export/package
acceptance, generated sidecar text, write policy, nested mesh-manifest-only path
facts, `.igmesh` loading, geometry validation, policy/built-in reconstruction,
discovery/scanning/catalog/registry, exact-extra-file rejection, renderer/model
slot/schema/gameplay behavior, glTF/glb/JSON parser work, or next source packet
scope.

Native Static Mesh Package Directory Report Data Builder Extraction is complete
as a behavior-preserving structured-data split. The new no-text helper
`BuildNativeStaticMeshExportPackageDirectoryReportData(const std::filesystem::path &directory)`
performs structured package report data collection and leaves `report.text`
empty. Its responsibilities are the package directory read, nested mesh manifest
read attempt/result, package-vs-nested-manifest comparison, package sidecar
facts, nested manifest facts, and package-declared asset facts.
`BuildNativeStaticMeshExportPackageDirectoryReport(...)` remains the full report
builder by calling the data builder and then assigning
`report.text = BuildNativeStaticMeshExportPackageDirectoryReportText(report)`.
Full report text and behavior are preserved exactly: no report text, row,
summary field, row-order, status, count, token, or newline changes. Focused
tests compare no-text data-builder structured fields against the full builder
for valid export, missing package sidecar, missing nested manifest, combined
comparison mismatch, and missing declared asset; full builder text is still
checked against the text renderer helper. This docs packet does not change
`readOk()`, CLI exit behavior, core `issues=`, package directory reader
status/data, exact verification/export/package acceptance, generated sidecar
text, write policy, nested mesh-manifest-only path facts, `.igmesh` loading,
geometry validation, policy/built-in reconstruction, discovery/scanning/catalog/
registry, exact-extra-file rejection, renderer/model slot/schema/gameplay
behavior, glTF/glb/JSON parser work, or next source packet scope.

Native Static Mesh Package Directory Report Builder Parity Coverage is complete
as a test-only coverage packet. Existing data-builder/full-builder parity and
text-renderer parity assertions now cover missing-from-manifest comparison,
missing-from-package comparison, filename mismatch comparison, missing directory,
file path instead of directory, malformed package sidecar data-builder parity,
malformed nested manifest, directory at declared asset path, and extra unrelated
file ignored branches. The tests use existing
`BuildNativeStaticMeshExportPackageDirectoryReportData(...)`,
`ExpectDataBuilderMatchesFullReport(...)`, and `ExpectTextRendererMatches(...)`
helpers. Source verification passed focused package-directory report tests,
`iggy_native_play`, valid package-directory report smoke, missing package
sidecar smoke, missing nested mesh manifest smoke, and source `git diff --check`.
This packet changed no production source, report text, CLI behavior, package-
directory read behavior/status/issue counts/rows, manifest comparison row/count
semantics, file facts, verifier behavior, exact sidecar matching, generated
sidecar/export behavior, package acceptance semantics, package directory report/
reader internals, package loading/discovery, `.igmesh` loading beyond existing
verifier behavior, renderer/model-slot behavior, CMake, assets, fixtures,
glTF/glb/JSON parser work, or next source packet scope.

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
- Other model-slot file binding, glTF/glb parsing under the constrained subset, asset
  registry/catalog, materials/textures/descriptors/samplers, non-cube model slot
  expansion, package/authoring asset policy, renderer expansion, and any canvas
  polish.
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
