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
