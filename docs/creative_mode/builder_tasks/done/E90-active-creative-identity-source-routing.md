# E90: Active Creative Identity Source Routing

## Objective

Move the first production readers off flat `window.activeCreative*` mirrors and
onto `creative::CreativeAppState::identity`, while preserving public receipt
compatibility fields.

## Problem

E88 identified active Creative identity as the safest first
`ProductAppWindowState` ownership migration. The typed source already exists:
`CreativeActiveIdentity` on `CreativeAppState`. Some production routing still
uses flat window mirrors, which can go stale.

This card should make source truth explicit for the narrow call sites that can
already see `CreativeAppState`.

## Required Reads

- `docs/creative_mode/post_claude_architecture_review_tally.md`
- `docs/creative_mode/builder_tasks/done/E88-product-window-state-owner-classification.md`
- `src/app/iggy3d/creative/CreativeAppState.hpp`
- `src/app/iggy3d/menu/FrontendRouter.cpp`
- `src/app/iggy3d/save/Flow.cpp`
- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/ReceiptBuilder.hpp`
- `tests/unit/product_frontend_router_tests.cpp`
- `tests/unit/product_save_bridge_tests.cpp`
- `tests/unit/product_creative_world_launch_tests.cpp`

## Scope

Allowed:

- Add or update tests proving stale flat window mirrors do not make Creative
  look active when `CreativeAppState::identity` is inactive.
- Change production readers that already receive or can cheaply receive
  `CreativeAppState` to use `CreativeActiveIdentity` instead of
  `window.activeCreative*`.
- Keep `window.activeCreative*` fields as receipt/compatibility mirrors.
- Keep existing launch/open/save identity behavior unchanged.

Likely target seams from E88:

- `FrontendRouter.cpp` already has an identity-based predicate. Prefer that
  source over `productCreativeWorldActiveForWindowMirror(...)` where
  `CreativeAppState` is available.
- `save/Flow.cpp` save-failure diagnostics should not infer Creative identity
  from stale window mirrors when `CreativeAppState::identity` is available.

## Do Not

- Do not delete `window.activeCreative*` fields.
- Do not change public receipt keys.
- Do not change save ids, save paths, world ids, document ids, or dirty-drain
  behavior.
- Do not change active-room/collision ownership in this card.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Acceptance

- A stale-window-mirror test exists and fails without the source-routing change.
- Production call sites covered by the test read `CreativeActiveIdentity` as
  source truth.
- Public receipt compatibility remains unchanged.
- Focused frontend/save/creative launch tests pass.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target product_frontend_router_tests product_save_bridge_tests product_creative_world_launch_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_frontend_router_tests|product_save_bridge_tests|product_creative_world_launch_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Behavior changed:
- Window mirror readers migrated:
- Stale-mirror test proof:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief - 2026-07-06

- Files changed:
  - `src/app/iggy3d/save/Flow.cpp`
  - `tests/unit/product_creative_world_launch_tests.cpp`
  - builder queue bookkeeping files for claim/done movement.
- Behavior changed:
  - Pause save/save-and-exit flow now prefers
    `creative::CreativeAppState::identity` when a `CreativeAppState*` is
    available.
  - If no `CreativeAppState*` is available, the old
    `window.activeCreative*` mirror predicate remains the fallback for existing
    null-facade failure behavior.
  - Public receipt fields and `window.activeCreative*` compatibility mirrors
    are unchanged.
  - Save ids, save paths, world ids, document ids, dirty-drain behavior, and
    launch/open behavior are unchanged.
- Window mirror readers migrated:
  - `executeProductPauseSaveFlow(..., creative::CreativeAppState*)`
  - `executeProductPauseSaveFlow(..., FrontendSettings&, creative::CreativeAppState*)`
  - Both now route the Creative save branch through
    `productCreativeDocumentEditorActiveForSource(...)`, which checks
    `window.interactionMode == Creative` plus
    `productCreativeWorldActiveForIdentity(creativeApp->identity)` when the
    typed source is present.
- Stale-mirror test proof:
  - Added `pauseSaveUsesCreativeIdentityInsteadOfStaleWindowMirror()` in
    `product_creative_world_launch_tests.cpp`.
  - The test sets stale `window.activeCreativeSaveId`,
    `window.activeCreativeWorldId`, and `window.activeCreativeDocumentId` while
    leaving `CreativeAppState::identity` inactive.
  - It proves pause save does not request Creative save and falls back to the
    product-save path with `product_save_session_missing`.
  - This test would fail under the previous window-mirror branch decision.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target product_frontend_router_tests product_save_bridge_tests product_creative_world_launch_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_frontend_router_tests|product_save_bridge_tests|product_creative_world_launch_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Focused trailing-whitespace scan over `src/app/iggy3d/save/Flow.cpp`,
    `tests/unit/product_creative_world_launch_tests.cpp`, this card, and
    `PRIORITY.md`.
- Concerns/deferred:
  - `productCreativeDocumentEditorActiveForWindow(...)` still exists and still
    reads window mirrors. That is intentional compatibility/fallback for callers
    that do not have `CreativeAppState`.
  - Frontend surface classification still cannot use source identity until its
    call sites pass identity through the route context. That should be a
    separate focused card, not folded into this save-flow slice.
