# 48 Qt Product Input Frame Target Context Consumer Status Sync

Status: complete.

## Goal

Sync planning and API docs after Qt began consuming
`RuntimeGameplayProductInputFrameTargetContext` in the shared product one-frame
path.

## Integrated Surface

- `IggyQtShellWindow::runProductFrameRequestOnce()` is the thin Qt caller.
- Manual `Product Step` and Qt `Product Frame Pump` both use it because both
  already call `runProductFrameRequestOnce()`.
- Qt includes `runtime/RuntimeGameplayProductInputFrameTargetContext.hpp`.
- Qt calls `RuntimeGameplayProductInputFrameTargetContext {}.enrich(...)` after
  accumulator `buildFrame(...)` and after `productInputAccumulator_ =
  frame.state`, but before `RuntimeGameplayProductFrameRequest {}.run(input)`.
- The call uses `productPlayState_`, `frame.frame`, and default spatial/reach
  configs.
- Qt passes `enrichedFrame.frame` to
  `RuntimeGameplayProductFrameRequestInput::inputFrame`.

## Behavior And Policy

- Accumulator one-shot drain and held-control behavior are unchanged because
  `productInputAccumulator_ = frame.state` remains before the frame request.
- On `NoEligiblePrimaryTile` or `Unchanged`, the helper returns a copied
  unchanged frame and Qt still passes that frame without branching.
- Qt does not store `enrichedFrame` diagnostics, target-query result,
  target-context result, primary tile index/tile, or reach result.
- No diagnostics are exposed in product panel, scene/ui models, settings, logs,
  or status text.
- Product Input Focus gating, mouse event handling, keyboard mapping, pump
  timing, and manual Step availability remain unchanged.

## Boundaries Preserved

- Qt is a thin caller of the app-neutral helper; it does not own target lookup
  semantics.
- No Qt types in runtime/product APIs.
- No changes to `RuntimeGameplayProductFrameRequest`,
  `RuntimeGameplayProductPlaySurfaceFrame`, product loop, input adapter,
  accumulator, input context, target query, target-context helper, or
  input-frame target-context helper semantics.
- No new Qt mouse behavior beyond existing focused ready left-click that records
  transient `PrimaryTile`.
- No diagnostics storage or persistence in Qt state, scene/UI truth, settings,
  runtime/session/gameplay/product-loop/play-mode state, snapshots, or saves.
- No `PrimaryTile` to `Interact` conversion.
- No `Interact`/`Inspect` target-id injection.
- No selected target state or durable hover state.
- Reach remains diagnostic only inside helper result and is not a Qt gate.
- No render drawing, target highlighting, sprite/material policy,
  pause/retry/reset, completion/failure, save/load,
  package scanning/watching/discovery, or source mutation.

## Next Framing

- This is the first thin caller use of the opt-in pre-frame helper.
- Future gates may decide diagnostics display, frame-request/play-surface
  ownership, reach-gated behavior, click-to-interact, selected target state,
  durable hover lifecycle, target highlighting, render drawing/canvas
  interpretation, `PrimaryPoint`, sprites/assets/materials, and
  UX/save/completion/package/source.

## Verification

- `git diff --check`
- `git status --short --branch`
