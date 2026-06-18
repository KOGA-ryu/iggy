# 36 Qt Product Manual Step Status Sync

Status: complete.

Goal: update planning and API docs after Qt Product Manual Step consumer
integration.

Integrated behavior recorded:
- Qt shell View menu exposes `Product Step` for ready `--play` sessions.
- The action is enabled only when product play mode exists and build status is
  ready.
- The action is independent of `Product Input Focus`; if focus is false,
  existing play-surface behavior ignores input but still consumes one available
  frame with empty intents/context.
- Execution builds `RuntimeGameplayProductFrameRequestInput` from current
  `productPlayState_`, transient `productInputFrame_`, and shell-owned
  presentation camera config.
- It calls `RuntimeGameplayProductFrameRequest {}.run(input)` exactly once.
- It updates only replaceable transient app-shell state:
  `productPlayState_`, `latestProductPlayModeFrame_` plus the stable product
  play panel context pointer, and `productPresentationCamera_` for the next
  request.
- It clears `productInputFrame_` after every executed request, regardless of
  `Stepped`, `NoFrameAvailable`, or `NotLoaded`.
- If no request executes because Step is unavailable, input is not silently
  cleared.
- It refreshes the existing product play panel after the request.
- Camera/config defaults are shell presentation defaults only: explicit
  fallback/view defaults, NPC commands enabled, and tile chunk cache disabled.
  They are not settings/save truth.

Boundaries preserved:
- No automatic app/tick loop or frame pump.
- No mouse screen-to-world/tile mapping.
- No player or modern NPC render projection expansion.
- No pause/retry/reset/completion/failure/save-load productization.
- No package scanning/watching/discovery or source mutation.
- No raw input persistence in runtime/session/gameplay/product-loop/play-mode
  state, snapshots, saves, settings, or scene/UI models.
- No camera, presentation, or render-frame data persistence as
  gameplay/session/product-loop/play-mode/save truth.
- Scene UI models do not execute product frames.
- No changes to `RuntimeGameplayProductFrameRequest`, play mode, product loop,
  input adapter, camera policy, or UI panel model semantics.
- No held-key/repeat/cadence behavior; `productInputFrame_` remains
  latest-event only.
- No settings persistence or keyboard shortcuts for Step.

Verification:
- `git diff --check`
- `git status --short --branch`
