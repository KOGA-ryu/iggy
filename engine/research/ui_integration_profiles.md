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
ready/focused keyboard product input mapping; frame pumping, camera policy,
mouse/world/tile input mapping, and gameplay execution remain gated.

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
  into the app-shell-owned transient
  `RuntimeGameplayProductInputFrame2D productInputFrame_`.
- The shell stores product input events only, not raw `QKeyEvent` objects or
  pointers.
- Product input events are recorded only when product play mode exists,
  play-mode build status is ready, and product input focus is enabled.
- Disabled focus, failed-load, and not-ready play sessions record no event; when
  focus is disabled, the transient input frame is cleared.
- The frame is latest-event bounded: each accepted key clears the frame before
  appending one event.
- Auto-repeat and unsupported keys are ignored. Arrow/WASD map to movement,
  `E`/Return/Enter to interact, `I` to inspect, Space to wait, and Escape to
  cancel.
- Binding context remains default; no current-player tile, selected target,
  hovered target, scene/UI model exposure, or settings exposure was added.

Hard stops for product play UI projection:
- No product frame execution, `RuntimeGameplayProductPlayMode::frame(...)`,
  `RuntimeGameplayProductPlaySurfaceFrame::build(...)`, app tick loop, or frame
  pump.
- No raw Qt/device event persistence; supported keyboard mapping may produce
  only transient product input events inside the Qt shell.
- No UI execution, frame stepping, automatic app/tick loop, or frame pump.
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
- No mouse position to world/tile mapping, `PrimaryPoint`, or `PrimaryTile`
  mapping.
- No settings persistence or keyboard shortcut for the focus toggle.
- No product play mode mutation from scene/UI projection code; Qt launch/focus
  code may update only the durable current focus bit through
  `RuntimeGameplayProductPlayMode::withInputFocus(...)`. Runtime/gameplay state
  mutation remains prohibited.
- No raw Qt event or product input event persistence in
  gameplay/session/product-loop/play-mode/save/settings/scene-UI truth.
- No camera, presentation, or render-frame persistence in
  gameplay/session/product-loop/play-mode/save truth.
- No hidden default camera/render config inside the UI model.
- No pause/retry/reset, completion/failure, save/load productization, package
  scanning/watching/discovery, source mutation, or new gameplay semantics.

## Later Milestones

1. Source-linked diagnostics.
2. Visual trace playback.
3. Mouse/world/tile input mapping only after an explicit product shell gate.
4. Camera lifecycle/presentation policy.
5. Product frame stepping / automatic app tick loop / frame pump.
7. Build canvas for placement.
8. Structured authoring controls for existing facts.
9. Source/TOML roundtrip only after an explicit gate.

## Open Decisions

- Should Play and Build be separate app modes or tabs in one shell?
- Should TOML source view be always visible, optional, or debug-only?
- What is the first editable fact: terrain, actors, interactions, or frame
  commands?
- When does source mutation become safe enough to support?
