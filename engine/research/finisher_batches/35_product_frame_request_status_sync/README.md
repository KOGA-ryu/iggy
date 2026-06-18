# 35 Product Frame Request Status Sync

Status: complete.

Goal: update planning and API docs after
`RuntimeGameplayProductFrameRequest` integration.

Integrated behavior recorded:
- `RuntimeGameplayProductFrameRequest` is an app-neutral runtime/product manual
  frame request wrapper.
- It composes `RuntimeGameplayProductPresentationCamera {}.build(...)` first.
- It calls `RuntimeGameplayProductPlayMode {}.frame(...)` exactly once with the
  selected transient camera/render config.
- It maps play-mode frame status directly to request status.
- It returns the carried next `RuntimeGameplayProductPlayModeState` from the
  nested frame result.
- It projects `inputEventCount` from the supplied transient input frame and
  `ignoredInputEventCount` from the nested play-surface result.
- It preserves nested camera and play-mode frame results without flattening
  invented fields.
- Caller ownership remains explicit for input-frame clearing/draining,
  previous-camera storage, latest-frame storage, and presentation state
  ownership.
- `RuntimeGameplayProductPlayMode::frame(...)` semantics are unchanged.

Boundaries preserved:
- No Qt/UI/CLI behavior.
- No Qt manual Step button.
- No automatic tick loop or frame pump.
- No mouse screen-to-world/tile mapping.
- No raw input persistence in runtime/session/gameplay/product-loop/play-mode,
  snapshots, saves, or UI models.
- No camera, presentation, or render-frame persistence as
  gameplay/session/product-loop/play-mode/save truth.
- No input-frame draining/clearing policy inside runtime.
- No previous-camera/latest-frame storage inside runtime/product state.
- No context enrichment from game state.
- No post-step camera follow.
- No player sprite or modern NPC render projection expansion.
- No pause/retry/reset, completion/failure, save/load semantics, package
  scanning/watching/discovery, source mutation, or gameplay semantics.
- No product loader/loop/play-mode semantic changes beyond narrow composition.

Verification:
- `git diff --check`
- `git status --short --branch`
