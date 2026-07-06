# E55: Active Creative State Accessor And Fixtures

## Objective

After E54 centralizes active Creative identity mirroring, stop treating flat
`ProductAppWindowState::activeCreative*` fields as the easiest way for production
routing and tests to decide whether Creative mode is live.

## Problem

The active Creative identity migration is incomplete in two ways:

- Production routing still reads the window mirror as logical truth:
  `src/app/iggy3d/menu/FrontendRouter.cpp:314` checks
  `window.activeCreativeSaveId`, `window.activeCreativeWorldId`, and
  `window.activeCreativeDocumentId` directly.
- Many tests seed those same flat fields by hand to make Creative surfaces look
  active, e.g. `product_creative_ui_input_frame_tests.cpp`,
  `product_window_input_frame_tests.cpp`,
  `product_creative_viewport_pick_frame_tests.cpp`,
  `product_creative_ui_frame_tests.cpp`, and
  `product_frontend_router_tests.cpp`.

That makes the mirror a de facto source of truth even after the real identity
owner exists. A test can pass by writing a stale/partial mirror state without
exercising `creative::CreativeAppState::identity`, launch/open identity setup, or
the projection boundary.

## Dependencies

- Do this after E54, or keep the diff explicitly compatible with the E54 helper
  boundary.

## Required Reads

- `src/app/iggy3d/creative/CreativeAppState.hpp`
- `src/app/iggy3d/menu/FrontendRouter.cpp`
- `src/app/iggy3d/menu/FrontendRouter.hpp`
- `src/app/iggy3d/creative/ui/UiProjection.cpp`
- `src/app/iggy3d/window/InputFrame.cpp`
- `tests/unit/product_frontend_router_tests.cpp`
- `tests/unit/product_creative_ui_input_frame_tests.cpp`
- `tests/unit/product_window_input_frame_tests.cpp`
- `tests/unit/product_creative_viewport_pick_frame_tests.cpp`
- `tests/unit/product_creative_ui_frame_tests.cpp`

## Scope

- Add a small accessor/fixture boundary for active Creative world state, rather
  than making tests and routing code write/read the flat mirror fields directly.
- Prefer reading `creative::CreativeAppState::identity.worldActive()` where a
  creative app/container is available.
- Keep a narrow window-mirror fallback only where a function truly cannot see
  creative app state yet.
- Replace repeated test setup assignments like:
  `window.activeCreativeSaveId = ...; window.activeCreativeWorldId = ...; ...`
  with a shared helper that also populates the source identity when available.
- Preserve externally emitted RenderReceipt fields and current behavior.

## Acceptance

- Product routing has a named active-Creative-state seam instead of open-coded
  checks against flat window mirror fields.
- Tests that need a live Creative world use a fixture/helper that can keep
  `CreativeAppState::identity` and the window mirror coherent.
- Direct ad hoc writes to `window.activeCreativeSaveId`,
  `window.activeCreativeWorldId`, and `window.activeCreativeDocumentId` are
  materially reduced in focused Creative tests.
- Frontend/router tests prove the accessor handles:
  live identity, mirror-only fallback, and inactive/no-identity states.
- No launch/open/save behavior or receipt key changes.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_frontend_router_tests product_creative_ui_input_frame_tests product_window_input_frame_tests product_creative_viewport_pick_frame_tests product_creative_ui_frame_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_frontend_router_tests|product_creative_ui_input_frame_tests|product_window_input_frame_tests|product_creative_viewport_pick_frame_tests|product_creative_ui_frame_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not remove the mirror fields from `ProductAppWindowState`; they are still
  receipt/projection output.
- Do not make `FrontendRouter` own or mutate `CreativeAppState`.
- Do not rewrite unrelated UI/frame tests just to chase every occurrence.
- Do not combine this with E47 command diagnostic restructuring.

## Completion Brief

Status: done.

Files modified:
- `src/app/iggy3d/menu/FrontendRouter.hpp`
- `src/app/iggy3d/menu/FrontendRouter.cpp`
- `tests/unit/product_frontend_router_tests.cpp`
- `tests/unit/product_creative_ui_input_frame_tests.cpp`
- `tests/unit/product_window_input_frame_tests.cpp`
- `tests/unit/product_creative_viewport_pick_frame_tests.cpp`
- `tests/unit/product_creative_ui_frame_tests.cpp`

Implementation:
- Added named active Creative state accessors:
  - `productCreativeWorldActiveForIdentity(...)`
  - `productCreativeWorldActiveForWindowMirror(...)`
- Kept `productCreativeWorldActiveForWindow(...)` as the compatibility fallback
  wrapper for code that only has a `ProductAppWindowState`.
- Updated focused test fixtures so Creative-document window setup builds a
  `CreativeActiveIdentity` and projects it with
  `mirrorProductActiveCreativeIdentity(...)` instead of hand-writing the flat
  mirror fields.
- Left one intentional direct partial mirror write in
  `product_frontend_router_tests.cpp` to prove a stale document id in Player
  mode does not classify as live CreativeDocument.

Tests:
- Router tests now prove:
  - inactive identity is not active,
  - live `CreativeActiveIdentity` is active,
  - window-mirror fallback remains active for window-only callers,
  - empty mirror fallback is inactive,
  - CreativeDocument still wins over stale legacy map-maker status.
- Focused UI/input/pick/frame fixtures now use the identity projection helper.

Verification:
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing-whitespace scan over touched files
- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_frontend_router_tests product_creative_ui_input_frame_tests product_window_input_frame_tests product_creative_viewport_pick_frame_tests product_creative_ui_frame_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_frontend_router_tests|product_creative_ui_input_frame_tests|product_window_input_frame_tests|product_creative_viewport_pick_frame_tests|product_creative_ui_frame_tests)$' --output-on-failure`

All verification passed.

Concerns:
- Most production callers still only have the window mirror; this slice made the
  fallback explicit and gave identity-aware callers/tests a named seam, but it
  did not thread `CreativeAppState` through broader routing surfaces.
