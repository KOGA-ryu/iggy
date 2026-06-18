# UI Integration Profiles

This note records the intended high-level UI shape for later integration. It is
not an implementation order and does not open UI/Edi mutation work.

## Principle

The UI should display the game first, not the engine. Authoring and debugging
surfaces should be available as separate workspace profiles so normal play,
scenario construction, validation, and engine inspection do not compete for the
same screen.

The existing UI direction can support multiple far-left profile tabs. Those tabs
should represent workspace modes.

## Far-Left Workspace Profiles

### Play

Purpose: play the scenario.

Primary content:
- World/level view.
- Player character, NPCs, doors, items, and interaction targets.
- Minimal HUD.
- Current objective or local context.
- Inventory quick view.
- Interaction prompts.
- Pause/menu/save controls later.

Hidden by default:
- TOML source-plan details.
- Validator diagnostics.
- Runtime frame internals.

### Build

Purpose: arrange scenario content.

Primary content:
- Level/scenario canvas.
- Terrain, wall, and floor placement.
- Player start placement.
- NPC placement.
- Item/drop placement.
- Interaction target placement.
- Region placement.

This should be spatial and visual. It should not expose raw engine registries as
the primary interaction model.

### Script

Purpose: author scenario sequence facts without becoming a generic scripting
language.

Primary content:
- Authored frame flow.
- Player commands: move, interact, pickup.
- NPC controls: wait, seek, movement mode, target.
- Interaction/pickup timing.
- Frame ids and ordering.
- Expectation authoring entry points.

Non-goal:
- Arbitrary expressions, loops, condition language, or event scripting.

### Check

Purpose: prove the authored scenario works.

Primary content:
- Lint/check/run status.
- Expected vs actual results.
- Final rows.
- Trace frame playback.
- Diagnostics with source locations.
- Summary counts.

This profile should be backed by `RuntimeGameplayAuthoringPreviewModel` and the
existing authoring facade/projection APIs.

### Actors

Purpose: inspect and author NPC/player-facing actor facts.

Primary content:
- NPC actor list.
- Profiles and trait sets.
- Actor positions.
- Actor control preview.
- Movement/debug overlays where useful.

### Items

Purpose: inspect and author inventory/drop facts.

Primary content:
- Item drops.
- Pickup targets.
- Inventory expectations.
- Item ids/counts.
- Future item definitions if a catalog is approved.

### Interactions

Purpose: inspect and author interaction target/effect facts.

Primary content:
- Interaction targets.
- Toggle/required-item state.
- Enabled/disabled preview.
- Required item ids.
- Existing effect metadata.
- Future inspect/talk/event hooks only after gates approve them.

### Package

Purpose: inspect authored package metadata and boundaries.

Primary content:
- Package title.
- Description.
- Authoring version.
- Main scenario path.
- Package validation status.
- Package-level diagnostics.

Non-goal:
- Recursive package discovery.
- Dependency management.
- Asset catalogs.

### Debug

Purpose: inspect engine/runtime state for developers.

Primary content:
- Runtime state tree.
- Command queue.
- Player intents.
- NPC actor/control registries.
- Interaction target/effect state.
- Inventory/drop state.
- Frame summaries.
- Occupancy/collision overlays.
- AI map overlay.
- Navigation/path overlay.
- Save/load snapshot status later.
- Performance counters later.

This profile is for engine inspection, not normal authoring.

### Docs / Examples

Purpose: provide example-driven authoring guidance.

Primary content:
- Canonical fixtures.
- Regression examples.
- Package examples.
- Supported TOML table/key shape.
- "Open as scenario" entry points.

## First UI Milestone

Status: complete for the existing Qt shell read-only preview consumer.

The first safe UI milestone is a read-only preview consumer:
- Accept one explicit TOML file path or package path through
  `iggy_qt_shell --preview PATH`.
- Select preview mode with `--preview-mode run|trace|check|lint`.
- Call the runtime read-only preview model.
- Project preview state through `UiAuthoringPreviewPanelModel`.
- Render the `panel:authoring_preview` panel in the existing Qt shell.
- Show package metadata when present.
- Show diagnostics.
- Show summary/final rows.
- Show trace frames.
- Show expectation result.

Hard stops for the first milestone:
- No editing.
- No save/load mutation.
- No file watching.
- No package scanning.
- No UI-owned parsing.
- No new scenario semantics.

## Product Play UI Projection

Status: complete for the read-only product play panel model, context seam, and
Qt `--play` launch/load/build consumer plus ready-state focus toggle and
ready/focused keyboard product input mapping; runtime/product presentation
camera policy, manual frame request wrapper, product input context projection,
product input accumulator, and Qt manual Step with accumulator output plus
transient current-player-tile context projection are complete; Qt product frame
pump toggle is complete as replaceable app-shell timing; runtime/product actor
debug/material quad projection is complete; runtime/product pointer projection
and explicit primary point/tile accumulator event preservation are complete.
Qt product viewport ownership is complete as an app-shell surface.
Thin Qt mouse `PrimaryTile` consumer is complete for focused ready left-clicks
on the viewport. Scene-only `InteractionTargetSpatialQuery2D` is complete as a
pure spatial lookup primitive, and runtime/product
`RuntimeGameplayProductInteractionTargetQuery` is complete as a read-only
point/tile-center target query plus reach report, and
`RuntimeGameplayProductInputTargetContext` is complete as transient
hovered-target binding context enrichment from that report. Runtime/product
`RuntimeGameplayProductInputFrameTargetContext` is complete as opt-in pre-frame
target-context enrichment over the latest eligible `PrimaryTile` pressed event.
Textured sprites/animation/material policy, Qt/manual Step/pump or frame
request/play-surface context wiring decisions, explicit interaction execution,
product UX/save semantics, and broader UI execution remain gated.

The current product play UI projection:
- Extends `UiFeatureContext` with direct product play build/state/latest-frame
  pointers and a presence helper.
- Registers `feature:product_play` and `panel:product_play` in the runtime
  workspace model.
- Projects supplied product play pointers through
  `UiProductPlayModePanelModel`.
- Populates the product play panel only when product play context exists.
- Emits no missing-context diagnostic when product play context is absent.
- Keeps the product play panel hidden by default through existing panel
  assignments/settings behavior.
- Displays rows/counts from existing runtime fields: build/loop status,
  identity paths, loaded/focus/frame index/scenario frame count, latest
  frame/surface status, ignored input count, adapter counts, binding counts,
  step status/frame/counts, and presentation/render counts.

Qt launch consumer:
- `iggy_qt_shell --play PATH` accepts one explicit TOML file, package directory,
  or `package.toml` path for product load/build/play-mode build.
- The shell stores load, loop, play-mode build results and play-mode state in
  `IggyQtShellWindow` for stable UI context pointers.
- The shell sets product play context pointers and reveals the existing
  read-only `panel:product_play`.
- Latest product play frame remains absent until a later frame-step/tick gate.
- `--play` and `--preview` are mutually exclusive and exit 2 when combined.
- Missing `--play` path exits 2.
- Bad filesystem paths still open the shell and display failed load/build state.

Qt focus toggle:
- The View menu exposes a checkable `Product Input Focus` action for ready
  `--play` sessions.
- The action is enabled/applicable only for ready product play state.
- Toggling updates only the durable current product play focus bit, keeps
  product play context pointers stable, clears latest product play frame to
  absent, and refreshes the read-only product play panel.
- Failed-load product play sessions keep the action disabled/non-applicable.
- Launch still starts focused by default through runtime play-mode build
  defaults.

Qt keyboard input mapping:
- Ready, focused `--play` sessions map supported Qt key press/release events
  into app-shell-owned transient product accumulator state.
- The shell stores product controls/events only, not raw `QKeyEvent` objects or
  pointers.
- Product input events are recorded only when product play mode exists,
  play-mode build status is ready, and product input focus is enabled.
- Disabled focus, failed-load, and not-ready play sessions record no event; when
  focus is disabled, accumulator state is cleared.
- Auto-repeat and unsupported keys are ignored. Arrow/WASD map to movement,
  `E`/Return/Enter to interact, `I` to inspect, Space to wait, and Escape to
  cancel.
- Binding context is supplied later when building request frame output; no
  selected target, hovered target, scene/UI model exposure, or settings exposure
  was added.

Runtime presentation camera policy:
- `RuntimeGameplayProductPresentationCamera` is an app-neutral runtime/product
  policy that chooses transient caller-owned `CameraState` plus
  `LevelRenderFrame2DConfig` from product play state and caller-owned config.
- It supports not-loaded previous/fallback camera selection, loaded player
  initialization, previous-camera player follow through existing `CameraRig`,
  follow-disabled previous/fallback behavior, clamp result flags, and render
  config forwarding.
- It does not execute frames, call play-surface build, call product input
  adapter/binding, persist presentation state, or add Qt/UI/CLI behavior.

Runtime manual frame request wrapper:
- `RuntimeGameplayProductFrameRequest` composes presentation camera policy first
  and then calls `RuntimeGameplayProductPlayMode::frame(...)` exactly once with
  selected transient camera/render config.
- It maps nested play-mode frame status to request status, returns carried next
  play-mode state, projects supplied input event count and nested ignored input
  count, and preserves nested camera/play-mode frame results.
- Caller code owns transient input-frame clearing/draining, previous-camera
  storage, latest-frame storage, and presentation state ownership.
- It does not add Qt/UI/CLI behavior, a manual Step button, automatic tick
  loop/frame pump, mouse screen-to-world/tile mapping, raw input persistence,
  save/load, UX semantics, or gameplay semantics.

Runtime input context projection:
- `RuntimeGameplayProductInputContext` projects transient
  `PlayerInputBindingContext2D` for product play input.
- It reports `NotLoaded`, `LoadedWithoutPlayer`, or `Projected`.
- It returns default binding gates and sets only current player tile from loaded
  product play state when a player exists.
- It does not set selected/hovered targets, inspect interaction state, search
  targets, map mouse input, synthesize `PrimaryPoint`/`PrimaryTile`, or add
  cadence/pump behavior.

Runtime input accumulator:
- `RuntimeGameplayProductInputAccumulator` stores product-level transient input
  state only: held movement controls and pending one-shot product events.
- Held movement controls are `MoveNorth`, `MoveSouth`, `MoveWest`, and
  `MoveEast`; one-shot controls are `Interact`, `Inspect`, `Wait`, and
  `Cancel`, plus explicit `PrimaryPoint` and `PrimaryTile`.
- Held movement presses are deduplicated, releases remove held movement, and
  one-shot and primary releases are no-ops.
- Frame output emits held movement `Pressed` events each requested frame before
  one-shot events, carries supplied binding context, preserves held controls,
  preserves explicit primary point/tile payloads, drains one-shots, and never
  synthesizes `PrimaryPoint`/`PrimaryTile`.
- `clear(...)` returns empty transient state.

Runtime pointer projection:
- `RuntimeGameplayProductPointerProjection` maps viewport-local points through
  existing `CameraView` normalization to world point plus `tileForPoint(...)`
  tile.
- It uses `CameraState`, `CameraView`, `CameraViewConfig`, and `TileCoord` only;
  no Qt types.
- It preserves `CameraView` behavior for negative viewport dimensions,
  non-positive zoom, and zero-axis degenerate viewports.
- It returns status/flags/world point/tile for a future consumer to choose
  `PrimaryTile`, `PrimaryPoint`, or both under a separate policy gate.
- Point-vs-tile / `PrimaryPoint` emission policy remains future work.

Qt product viewport owner:
- Product play sessions create `QFrame#productViewport` inside `QFrame#mainSlot`,
  including failed/not-ready play sessions.
- The viewport uses zero-margin/zero-spacing layout, expanding size policy, and
  local `QFrame#productViewport` styling only.
- `productViewport_` is reset to null on main-slot rebuilds where no viewport is
  created.
- The viewport is a stable event/render target boundary, not render semantics,
  runtime truth, or scene/UI model truth.

Qt product mouse primary-tile consumer:
- `productViewport_` installs a viewport-only event filter for product play
  sessions.
- Focused, ready left mouse press records exactly one transient
  `RuntimeGameplayProductInputEvent2D` with `PrimaryTile` pressed and
  `tile = projection.tile` into the product input accumulator.
- The handler accepts/returns true only when that event is recorded; otherwise it
  falls through.
- The handler normalizes `QMouseEvent::position()` by the viewport pixel size
  into the configured product camera-view span from
  `productPresentationCameraConfig().cameraView.viewportSize` before calling
  `RuntimeGameplayProductPointerProjection`.
- It builds `RuntimeGameplayProductPresentationCamera` read-only from
  `productPlayState_` and the product presentation camera config and stores no
  camera or pointer state.
- Product Step and Product Frame Pump consume the accumulator later.

Interaction target spatial query:
- `InteractionTargetSpatialQuery2D` is scene-only and performs pure spatial
  lookup over `InteractionTarget2DRegistry`.
- It scans enabled targets in registry order, compares Euclidean distance to
  `max(0, target.radius) + max(0, extraRadius)`, picks the nearest eligible
  target, preserves first registry entry for exact distance ties, and supports
  point or tile-center queries.
- It does not project selected/hovered context, execute interactions, check
  reach/LOS/occupancy/pathfinding, define kind-priority/z-order/layer policy,
  or wire Qt/product input.

Runtime product interaction target query:
- `RuntimeGameplayProductInteractionTargetQuery` is an app-neutral runtime/product
  read-only report over current product play state interaction targets.
- It accepts query kind `None`, `Point`, or `TileCenter`, delegates point and
  tile-center lookup to `InteractionTargetSpatialQuery2D`, copies found target
  id/payload into the result, and annotates reach with `InteractionReach2D` when
  the state has a player.
- `NotLoaded`, `MissingQuery`, `TargetNotFound`, and `TargetFound` remain report
  statuses only; there is no top-level `LoadedWithoutPlayer` status and found
  targets remain visible without a player.
- It does not mutate selected/hovered target context, convert `PrimaryTile` to
  `Interact`, execute click-to-interact, use `InteractionPlan2D`, change
  command/gate/effect semantics, persist query results, or add Qt behavior.

Runtime product input target context:
- `RuntimeGameplayProductInputTargetContext` copies a base
  `PlayerInputBindingContext2D` and enriches it from an already-computed product
  interaction target query report.
- Valid `TargetFound` reports with `hasTarget` and a non-empty target id set
  hovered target fields, copy a diagnostic hovered id, and return
  `TargetProjected`.
- Not-loaded, missing-query, target-not-found, invalid found reports, and empty
  target ids return `Unchanged` with the base context copied exactly.
- It preserves selected target fields, existing hover on unchanged paths, current
  player tile, and input gate flags; valid found reports replace only hover.
- Reachability is not required, and reach remains query/report annotation only.
- It does not own hover lifecycle, clear hover on misses, synthesize
  `Interact`/`Inspect` targets, mutate query/product/gameplay/input state, or
  wire frame request/play-surface behavior.

Runtime product input frame target context:
- `RuntimeGameplayProductInputFrameTargetContext` is an opt-in pre-frame helper
  between accumulator output and frame request.
- It copies the input frame, scans events in order, and uses the latest eligible
  `PrimaryTile` pressed event with a tile payload.
- If no eligible event exists, it returns `NoEligiblePrimaryTile` with the copied
  frame unchanged.
- Missing tile payloads and `PrimaryTile` releases are ineligible and preserved
  for existing adapter behavior.
- It queries `RuntimeGameplayProductInteractionTargetQuery` as `TileCenter`,
  forwards spatial/reach configs, applies `RuntimeGameplayProductInputTargetContext`
  to the input frame's base binding context, and replaces only the copied frame
  binding context when projection succeeds.
- It preserves all events unchanged and in order, treats reach as diagnostic
  only, and does not synthesize `Interact`/`Inspect`, inject target ids into
  events, mutate product/input state, or change frame request/play-surface/input
  adapter behavior.

Qt manual Step consumer:
- The View menu exposes `Product Step` for ready `--play` sessions only.
- The action is independent of `Product Input Focus`; when focus is false,
  existing play-surface behavior ignores input but still consumes one available
  frame with empty intents/context.
- Qt stores accumulator state instead of a latest-edge `productInputFrame_`.
- Key recording maps supported keys to product controls/events and records into
  accumulator state without clearing per key event.
- Focus disable clears accumulator state.
- Executing Step builds accumulator frame output with projected binding context,
  stores the returned accumulator state before the frame request, and builds
  `RuntimeGameplayProductFrameRequestInput` from current `productPlayState_`,
  accumulator output, and shell-owned presentation camera config, then calls
  `RuntimeGameplayProductFrameRequest {}.run(input)` exactly once.
- The shell updates only replaceable transient app-shell state:
  `productPlayState_`, latest product play frame/context pointer, and
  `productPresentationCamera_`.
- The shell preserves held movement across steps and drains one-shots after each
  executed Step.
- The projected context is not written back into accumulator state.
- If Step is unavailable, accumulator state is not silently cleared.
- The existing read-only product play panel is refreshed after the request.
- Camera/config defaults are shell presentation defaults only and are not
  settings/save truth.

Qt frame pump toggle:
- The View menu exposes a checkable `Product Frame Pump` action for ready
  `--play` sessions only.
- The pump is Qt/app-shell-owned replaceable timing through a window-owned
  `QTimer` at 250 ms / 4 Hz.
- Manual Step and timer ticks share the same one-frame helper: accumulator
  output plus projected input context, returned accumulator state storage, one
  `RuntimeGameplayProductFrameRequest` call, transient shell state updates, and
  product play panel refresh.
- The pump is independent of `Product Input Focus`; focus remains only the input
  gate.
- The timer stops when product play becomes unavailable or not ready, but does
  not auto-stop on `NoFrameAvailable`.
- Pump enabled state, interval, and keybindings are not persisted.

Product actor render projection:
- Runtime presentation can now include untextured debug/material quad commands
  for the current player and present modern NPC actors through
  `RuntimeGameplayProductActorRenderCommands`.
- The commands are appended after level render output by
  `RuntimeGameplayProductPresentationFrame` and can reach Qt product play
  latest-frame presentation data through the existing frame request path.
- Qt does not own actor render semantics, actor state, materials, assets, or
  render persistence.

Hard stops for product play UI projection:
- No product frame execution, `RuntimeGameplayProductPlayMode::frame(...)`,
  `RuntimeGameplayProductPlaySurfaceFrame::build(...)`, app tick loop, or frame
  pump from scene/UI projection code.
- No raw Qt/device event persistence; supported keyboard mapping may produce
  only transient product input events inside the Qt shell.
- No scene/UI projection-owned execution, frame stepping, app tick loop, or
  frame pump; the Qt shell pump is the separately documented replaceable
  app-shell timing path.
- No product loader/file/package/TOML APIs called from scene/UI model code.
- No calls to `RuntimeGameplayProductPlayMode::frame`,
  `RuntimeGameplayProductPlaySurfaceFrame::build`, loader APIs, or product
  step/run functions from scene/UI.
- No default camera/render config, product input events, presentation frame, or
  latest frame result synthesis in Qt launch.
- No raw Qt key/mouse/focus event routing into product input events from the
  focus toggle.
- No product input events, default camera/render config, presentation frames,
  latest-frame synthesis, frame stepping/manual stepping, app tick loop, or
  frame pump from the focus toggle.
- No `RuntimeGameplayProductInputAdapter::map(...)` or
  `PlayerInputBinding2D::bind(...)` calls from Qt input mapping.
- No Qt mouse behavior beyond viewport-only focused ready left-click
  `PrimaryTile`; no Qt types in runtime/scene/product APIs.
- No `PrimaryPoint`, world-point payload, right/middle/move/wheel/double-click/
  drag/hover behavior, or target lookup from Qt input mapping.
- No settings persistence or keyboard shortcut for the focus toggle.
- No product play mode mutation from scene/UI projection code; Qt launch/focus
  code may update only the durable current focus bit through
  `RuntimeGameplayProductPlayMode::withInputFocus(...)`. Runtime/gameplay state
  mutation remains prohibited.
- No raw Qt event or product input event persistence in
  gameplay/session/product-loop/play-mode/save/settings/scene-UI truth.
- No camera, presentation, render commands, actor render config, or render-frame
  persistence in gameplay/session/product-loop/play-mode/save truth.
- No presentation camera policy output stored in scene/UI models or settings.
- No product frame request invocation, input-frame clearing/draining, previous
  camera storage, latest-frame storage, or presentation state ownership inside
  scene/UI projection code.
- No projected binding context persistence in runtime/session/gameplay/
  product-loop/play-mode state, snapshots, saves, settings, scene/UI models, or
  accumulator state.
- No selected/hovered target discovery, interaction target search, reach lookup,
  mouse screen-to-world/tile mapping, `PrimaryPoint`, or `PrimaryTile` synthesis
  from input context projection. Scene-only spatial lookup is available as a
  primitive, product target query reports are available read-only, and product
  input target context enrichment plus opt-in frame enrichment exist, but they
  remain unwired to Qt/manual Step/pump and frame request/play-surface context
  mutation here.
- No accumulator state persistence in runtime/session/gameplay/product-loop/
  play-mode state, snapshots, saves, settings, or scene/UI models.
- No raw Qt key/event storage, cadence/rate policy, accumulator-owned frame
  pump, pointer synthesis, target search, or reach lookup in accumulator state.
- No runtime/product semantic ownership by Qt frame pump.
- No pump enabled state, interval, keybindings, input accumulator, pump state,
  camera, latest frame, or render-frame persistence in runtime/session/
  gameplay/product-loop/play-mode snapshots, saves, settings, or scene/UI model
  truth.
- No textured sprite/animation sampling, new art/assets/material registry,
  package discovery, additional Qt mouse behavior, target context wiring,
  target search, reach lookup beyond the read-only product query/enrichment
  helpers, automatic frame request/play-surface ownership of frame enrichment,
  pause/retry/reset/completion/failure/save-load productization, package
  scanning/watching/discovery, source mutation, raw Qt event persistence,
  projected pointer persistence, viewport geometry/state persistence, render
  command/config persistence, render command drawing, canvas polish, or
  runtime/product semantic changes from Qt frame pump or viewport ownership.
- No settings persistence or keyboard shortcut for Qt manual Step.
- No hidden default camera/render config inside the UI model.
- No pause/retry/reset, completion/failure, save/load productization, package
  scanning/watching/discovery, source mutation, or new gameplay semantics.

## Later Milestones

1. Source-linked diagnostics.
2. Visual trace playback.
3. Target discovery/search/reach and interaction execution.
4. Latest-frame presentation integration beyond the read-only panel, if needed.
5. Automatic app tick loop / frame pump.
7. Build canvas for placement.
8. Structured authoring controls for existing facts.
9. Source/TOML roundtrip only after an explicit gate.

## Open Decisions

- Should Play and Build be separate app modes or tabs in one shell?
- Should TOML source view be always visible, optional, or debug-only?
- What is the first editable fact: terrain, actors, interactions, or frame
  commands?
- When does source mutation become safe enough to support?
