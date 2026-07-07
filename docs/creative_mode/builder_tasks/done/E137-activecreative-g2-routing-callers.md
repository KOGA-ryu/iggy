# E137: ActiveCreative G2 - Migrate Routing Callers

## Objective

Gate 2 of the `window.activeCreative` mirror-delete migration. Switch routing
callers that now have `creativeApp` access to
`productCreativeDocumentEditorActiveForSource(...)`, while keeping the
mirror-backed leaf predicates available for fallback and later deletion.

## Prerequisite

E136 must be completed first. If
`productCreativeDocumentEditorActiveForSource(...)` is not declared in
`FrontendRouter.hpp` or the new request/context fields do not exist, move this
card to `blocked/` with the missing prerequisite.

## Required Work

1. Switch direct Group A callers to the source predicate where `creativeApp` is
   already in scope:
   - `src/app/iggy3d/save/Flow.cpp`
   - `src/app/iggy3d/window/InputFrame.cpp`
   - `src/app/iggy3d/creative/bridge/InputFrame.cpp`
   - `src/app/iggy3d/creative/bridge/WireframeFrame.cpp`
   - `src/app/iggy3d/creative/bridge/ViewportPickFrame.cpp`
   - `src/app/iggy3d/creative/ui/UiFrame.cpp`

2. Switch A-hop callers using fields added in E136:
   - `productWindowEditorMousePickSurfaceReady(...)`
   - `updateProductWindowMouseCapture(...)`

3. Switch Group B request-based callers using E136 fields:
   - `FramePresenter.cpp`
   - `ProjectionRefresh.cpp`
   - `Loop.cpp` title and no-window mouse-capture policy helpers.

4. Switch Group C only where the context has been threaded:
   - map-maker toggle context in `ActionHandlers`.
   - pause/transitions path only if a source parameter is already available from
     E136; otherwise leave it mirror-backed and report why.

5. Co-migrate tests that directly assert the source-vs-mirror routing split.
   Do not delete the mirror yet; tests may still assert fallback behavior.

## Do Not

- Do not touch receipt construction.
- Do not change `Flow.cpp:71-79`.
- Do not delete `ProductActiveCreativeState`, `window.activeCreative`,
  mirror funnel functions, or the TSV row.
- Do not make `productCreativeDocumentEditorActiveForWindow(...)`
  identity-only. It stays mirror-backed until G4.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Also report current routing grep output:

```sh
rg -n "productCreativeDocumentEditorActiveForWindow\\(|productCreativeWorldActiveForWindow\\(" /Users/kogaryu/iggy3d/src/app/iggy3d
```

## Acceptance

- Full CTest suite remains green.
- Routing callers with `creativeApp` availability use the source predicate.
- The mirror-backed window predicates still exist and still compile.
- Receipt golden is unchanged.

## Completion Brief

Append:

- Files changed:
- Group A callers migrated:
- A-hop callers migrated:
- Group B/C callers migrated or intentionally left:
- Remaining mirror-backed predicate readers:
- Suite:
- Concerns/deferred:

## Completion Brief - Done

- Files changed:
  - `src/app/iggy3d/creative/bridge/InputFrame.cpp`
  - `src/app/iggy3d/creative/bridge/ViewportPickFrame.cpp`
  - `src/app/iggy3d/creative/bridge/WireframeFrame.cpp`
  - `src/app/iggy3d/creative/ui/UiFrame.cpp`
  - `src/app/iggy3d/window/InputFrame.cpp`
  - `src/app/iggy3d/window/Loop.cpp`
  - `src/app/iggy3d/window/FramePresenter.cpp`
  - `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
  - `src/app/iggy3d/menu/ActionHandlers.cpp`
  - `src/app/iggy3d/menu/FrontendRouter.hpp`
  - `src/app/iggy3d/menu/FrontendRouter.cpp`
  - Source-vs-mirror fixture updates in focused product creative UI/input/pick/wireframe tests.
  - `tests/unit/product_frontend_router_tests.cpp`
- Group A callers migrated:
  - `Flow.cpp` was already using `productCreativeDocumentEditorActiveForSource(...)` from E136 and remains unchanged in this gate.
  - `ProductCreativeInputFrameRequest` and `ProductCreativeInputActionsRequest` paths now compute `receipt.active` with `productCreativeDocumentEditorActiveForSource(*request.window, request.creative)`.
  - `ProductCreativeWireframeFrameRequest`, `ProductCreativeViewportPickFrameRequest`, and `ProductCreativeUiFrameRequest` paths now use the source predicate from their request `creative` pointer.
  - Exported `*ActiveForWindow(...)` helpers remain mirror-backed for fallback/direct helper tests.
- A-hop callers migrated:
  - `productWindowEditorMousePickSurfaceReady(...)` now accepts `const creative::CreativeAppState*` and uses the source-backed world-active predicate, preserving its old world-active-only behavior.
  - `updateProductWindowMouseCapture(...)` now accepts `const creative::CreativeAppState*` and feeds the source predicate into `buildProductMouseCapturePolicy(...)`.
  - `processProductWindowInputFrame(...)` passes `context.creativeApp` into both A-hop helpers.
- Group B/C callers migrated or intentionally left:
  - `FramePresenter.cpp` creative overlay gating now uses `request.creativeApp`.
  - `ProjectionRefresh.cpp` HUD suppression, top-down map creative-active flag, creative-stage grid, and creative navigate override now use `request.creativeApp`; top-down map keeps world-active-only semantics.
  - `Loop.cpp` title and no-window mouse capture helpers now accept/pass `request.creativeApp`.
  - `ActionHandlers.cpp` map-maker toggle now uses `context.creativeApp` through the source-backed world-active predicate.
  - Pause/transitions remain mirror-backed because E136 did not thread a source pointer there.
  - `ProductControllerSampleInputContext` suppression remains mirror-backed because that context still has no `creativeApp` pointer.
- Remaining mirror-backed predicate readers:
  - Required grep reports remaining reads in exported fallback helpers, `FrontendRouter` internals, `Transitions.cpp`, and the controller-sample suppression path in `InputFrame.cpp`.
  - `productCreativeDocumentEditorActiveForWindow(...)` and `productCreativeWorldActiveForWindow(...)` still exist and still compile.
  - Receipt golden unchanged: `git diff -- tests/golden/product_receipt_key_order.golden` produced no diff.
- Suite:
  - `cmake --build /Users/kogaryu/iggy3d/build -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure` passed: 260/260 tests, total real time 45.49s.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files passed.
  - Targeted source-routed creative UI/input/pick/wireframe test cluster passed before the full suite.
- Concerns/deferred:
  - `Testing/Temporary/LastTest.log` remains dirty from CTest output and was not touched intentionally.
  - Gate 3 still needs receipt identity threading. Gate 4 still owns deletion of the mirror-backed predicates/state/funnel; this gate intentionally left them available.

## Review Repair - Codex

- Added `productCreativeWorldActiveForSource(...)` in `FrontendRouter` after
  review found several old world-active-only call sites had been changed to the
  document-editor predicate.
- Routed the world-active-only call sites through the new source-backed world
  predicate:
  - room-editor mouse-pick surface blocking
  - top-down map `creativeWorldActive`
  - map-maker toggle blocking
- Added a frontend-router guard proving `productCreativeWorldActiveForSource`
  ignores interaction mode while `productCreativeDocumentEditorActiveForSource`
  still requires Creative mode.
