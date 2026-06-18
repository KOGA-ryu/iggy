# 39 Qt Product Frame Pump Status Sync

Status: complete.

Goal: update planning and API docs after Qt Product Frame Pump Toggle
integration.

Integrated behavior recorded:
- Qt shell View menu exposes a checkable `Product Frame Pump` action for ready
  `--play` sessions.
- The pump is Qt/app-shell-owned replaceable timing only.
- The pump uses a window-owned `QTimer` at 250 ms / 4 Hz.
- Manual `Product Step` and timer ticks share the same one-frame helper/path.
- The shared path builds accumulator output plus projected input context, stores
  returned accumulator state so held movement survives and one-shots drain,
  calls `RuntimeGameplayProductFrameRequest {}.run(input)` exactly once per
  request/tick, updates current `productPlayState_`, latest frame member/context
  pointer, previous presentation camera, product play state context pointer, and
  refreshes the product play panel.
- Pump availability is ready product play only: product play exists and
  play-mode build is ready.
- Pump is independent of `Product Input Focus`; focus remains only the input
  gate.
- Timer stops if product play becomes unavailable or not ready.
- Timer does not auto-stop on `NoFrameAvailable`; no completion/end-state UX
  semantics were added.

Allowed transient Qt-owned state:
- Timer/action checked state.
- Existing product play state lane used by play mode.
- Product input accumulator state.
- Latest frame for panel projection.
- Previous presentation camera.

Boundaries preserved:
- No runtime/product API changes, scene/UI model changes, CMake/test changes, or
  product loop/frame request/play mode/input accumulator semantic changes.
- No player sprite or modern NPC actor render projection.
- No mouse screen-to-world/tile mapping.
- No `PrimaryPoint` or `PrimaryTile` synthesis.
- No selected/hovered target discovery, interaction target search, or reach
  lookup.
- No pause/retry/reset/completion/failure/save-load productization.
- No package scanning/watching/discovery or source mutation.
- No settings persistence for pump enabled state, interval, or keybindings.
- No raw Qt event persistence.
- No persistence of input accumulator, pump state, camera, latest frame, or
  render-frame data in runtime/session/gameplay/product-loop/play-mode
  snapshots, saves, settings, or scene/UI model truth.

Verification:
- `git diff --check`
- `git status --short --branch`
