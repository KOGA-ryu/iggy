# E136: ActiveCreative G1 - Source Predicate Plumbing

## Objective

Gate 1 of the `window.activeCreative` mirror-delete migration. Add the
identity/source routing path and thread `creativeApp` pointers to hot callers,
but do not switch routing behavior yet.

Design authority:
- `docs/activecreative_mirror_delete_preflight_v0_2.md`
- Parent decomposed card:
  `docs/creative_mode/builder_tasks/done/E135-activecreative-mirror-delete-decomposed.md`

## Scope

This is additive plumbing only. Keep `window.activeCreative` alive and keep the
existing mirror-backed routing predicates as the behavior source.

## Required Work

1. Move the dual-source predicate into FrontendRouter:
   - Declare/define
     `productCreativeDocumentEditorActiveForSource(const ProductAppWindowState&, const creative::CreativeAppState*)`
     in `src/app/iggy3d/menu/FrontendRouter.hpp/.cpp`.
   - Move the current body from `src/app/iggy3d/save/Flow.cpp`.
   - Keep the exact fallback behavior:
     - non-null `creativeApp`: Creative mode plus
       `productCreativeWorldActiveForIdentity(creativeApp->identity)`;
     - null `creativeApp`: fall back to
       `productCreativeDocumentEditorActiveForWindow(window)`.

2. Add defaulted `creativeApp`/`creative` pointer fields, but do not consume
   them for routing yet:
   - `ProductWindowFramePresenterRequest` in
     `src/app/iggy3d/window/FramePresenter.hpp`.
   - `ProductGameplayProjectionRefreshRequest` and
     `ProductGameplayProjectionFrameRequest` in
     `src/app/iggy3d/gameplay/ProjectionRefresh.hpp`.
   - `ProductGameplayMapMakerToggleActionContext` in
     `src/app/iggy3d/menu/ActionHandlers.hpp`.
   - `ProductWindowEditorMousePickPreviewContext` in
     `src/app/iggy3d/window/InputFrame.hpp`.

3. Populate the new fields only at build sites that already have the live
   `creativeApp`:
   - `src/app/iggy3d/window/Loop.cpp`
   - `src/app/iggy3d/AppKernel.cpp`
   - `src/app/iggy3d/window/InputFrame.cpp`
   - the map-maker toggle dispatch path in `ActionHandlers`/caller context.

4. Use forward declarations for `creative::CreativeAppState` in touched headers
   where a pointer is enough.

## Do Not

- Do not switch any call from
  `productCreativeDocumentEditorActiveForWindow(...)` or
  `productCreativeWorldActiveForWindow(...)` to the source predicate.
- Do not touch receipt construction.
- Do not change `Flow.cpp:71-79`.
- Do not delete `ProductActiveCreativeState`, `window.activeCreative`,
  `mirrorProductActiveCreativeIdentity`, or the ownership TSV row.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Also report:

```sh
rg -n "productCreativeDocumentEditorActiveForWindow|productCreativeWorldActiveForWindow" /Users/kogaryu/iggy3d/src/app/iggy3d
```

The report should confirm this gate added plumbing but did not migrate routing
reads.

## Acceptance

- Full CTest suite remains green.
- Receipt golden is unchanged.
- `activeCreative` grep count may remain unchanged except for no new field
  users.
- Diff shape is one moved predicate plus new defaulted pointer fields and
  population lines.

## Completion Brief

Append:

- Files changed:
- Predicate moved:
- Request/context fields added:
- Population sites:
- Proof no routing reads switched:
- Suite:
- Concerns/deferred:

## Completion Brief - Done

- Files changed:
  - `src/app/iggy3d/menu/FrontendRouter.hpp`
  - `src/app/iggy3d/menu/FrontendRouter.cpp`
  - `src/app/iggy3d/save/Flow.cpp`
  - `src/app/iggy3d/window/FramePresenter.hpp`
  - `src/app/iggy3d/gameplay/ProjectionRefresh.hpp`
  - `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
  - `src/app/iggy3d/menu/ActionHandlers.hpp`
  - `src/app/iggy3d/window/InputFrame.hpp`
  - `src/app/iggy3d/window/InputFrame.cpp`
  - `src/app/iggy3d/window/Loop.cpp`
  - `src/app/iggy3d/AppKernel.cpp`
- Predicate moved:
  - Added `productCreativeDocumentEditorActiveForSource(const ProductAppWindowState&, const creative::CreativeAppState*)` to `FrontendRouter.hpp/.cpp`.
  - Removed the duplicate local helper body from `Flow.cpp`.
  - Preserved the exact behavior: non-null `creativeApp` uses Creative mode plus `productCreativeWorldActiveForIdentity(creativeApp->identity)`; null `creativeApp` falls back to `productCreativeDocumentEditorActiveForWindow(window)`.
  - `Flow.cpp:71-79` was left unchanged.
- Request/context fields added:
  - `ProductWindowFramePresenterRequest::creativeApp`
  - `ProductGameplayProjectionRefreshRequest::creativeApp`
  - `ProductGameplayProjectionFrameRequest::creativeApp`
  - `ProductGameplayMapMakerToggleActionContext::creativeApp`
  - `ProductWindowEditorMousePickPreviewContext::creativeApp`
  - `dispatchProductWindowTopLevelToggleAction(...)` now accepts a defaulted `creative::CreativeAppState* creativeApp = nullptr` so existing call sites can stay source-compatible.
- Population sites:
  - `AppKernel.cpp` passes `&creativeApp` into `ProductGameplayProjectionRefreshRequest`.
  - `Loop.cpp` passes `request.creativeApp` into projection-frame and frame-presenter requests.
  - `ProjectionRefresh.cpp` threads `request.creativeApp` into the projection-frame request.
  - `InputFrame.cpp` passes `context.creativeApp` through top-level toggle dispatch and into map-maker toggle context, and adds it to editor mouse-pick preview context construction.
- Proof no routing reads switched:
  - No existing `productCreativeDocumentEditorActiveForWindow(...)` or `productCreativeWorldActiveForWindow(...)` routing read was migrated to the source predicate in this gate.
  - Required grep still reports mirror-backed reads in `FrontendRouter.cpp`, creative bridge wrappers, `ActionHandlers.cpp`, `Transitions.cpp`, `ProjectionRefresh.cpp`, `Loop.cpp`, `InputFrame.cpp`, and `FramePresenter.cpp`.
  - The only source-predicate consumer remains the existing save-flow call; this slice only moved that predicate implementation out of `Flow.cpp`.
  - Receipt golden unchanged: `git diff -- tests/golden/product_receipt_key_order.golden` produced no diff.
- Suite:
  - `cmake --build /Users/kogaryu/iggy3d/build -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure` passed: 260/260 tests, total real time 45.91s.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files passed.
  - Required grep command was run and included in the planner-facing brief.
- Concerns/deferred:
  - `Testing/Temporary/LastTest.log` remains dirty from CTest output and was not touched intentionally.
  - Gate 2 should consume these pointer fields; this slice intentionally leaves `window.activeCreative` and mirror-backed routing behavior in place.
