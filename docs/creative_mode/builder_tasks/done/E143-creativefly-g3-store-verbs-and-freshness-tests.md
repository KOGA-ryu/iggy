# E143: CreativeFly G3 - Store Verbs And Freshness Tests

## Objective

Add the store operations and direct freshness tests for creative-fly anchor reseeding. Do not migrate production call sites yet.

This gate proves the epoch model before it owns a live consumer.

## Prerequisite

E142 complete.

If the store type or `creativeWorldEpoch` is missing, move this card to `blocked/` with evidence.

## Required Work

1. Add window-scoped helper verbs around the E142 store:
   - `bumpCreativeWorldEpoch(ProductAppWindowState&)`
   - `seedCreativeFlyAnchorFromOrigin(ProductAppWindowState&)`
   - `ensureFreshCreativeFlyAnchor(ProductAppWindowState&, const Session*)`
   - `seedCreativeFlyAnchorFromScene(ProductAppWindowState&, Vec3)`
   - `recordCreativeFlyAnchorIntegrated(ProductAppWindowState&, Vec3)`

2. Preserve migration compatibility:
   - For now, helper writes may mirror to legacy `window.viewport.creativeFlyAnchorValid` and `window.viewport.creativeFlyPositionMeters`.
   - This lets receipt and older tests stay green until G7.

3. Helper semantics:
   - Origin seed uses `{0.0F, 6.0F, 10.0F}`, provenance `OriginFramed`, and current `creativeWorldEpoch`.
   - `ensureFreshCreativeFlyAnchor(...)` reseeds when store is unseeded or stale for `creativeWorldEpoch`.
   - Session seed uses active player position if available; otherwise origin `{}` fallback, preserving the old latch behavior.
   - Scene seed uses the supplied scene position and provenance `SceneSeeded`.
   - Integrated seed uses final fly position and provenance `FlyIntegrated`.

4. Add direct tests that prove:
   - two different epochs reseed even if session/runtime hash would be identical.
   - stale epoch is not considered fresh.
   - origin seed stamps the current epoch and provenance.
   - session ensure latches even with null session using the old origin fallback.
   - integration updates the position and provenance.

## Do Not

- Do not wire `frameCreativeStageCameraOnOrigin(...)`.
- Do not wire `ensureCreativeFlyAnchor(...)`.
- Do not wire `mapMakerAnchorFor(...)`.
- Do not change camera projection consumer.
- Do not remove legacy raw fields.
- Do not change receipt fields or regenerate receipt golden.
- Do not touch standalone app fly state.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_fly_anchor_store_tests product_creative_fly_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_fly_anchor_store_tests|product_creative_fly_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Helper verbs added:
- Freshness semantics:
- Compatibility mirror behavior:
- Tests/checks run:
- Concerns/deferred:

## Completed Brief

- Files changed:
  - Updated `src/app/iggy3d/view/CreativeFlyAnchorStore.hpp`.
  - Added `src/app/iggy3d/view/CreativeFlyAnchorStore.cpp`.
  - Updated `CMakeLists.txt` to compile the new store implementation.
  - Updated `tests/unit/product_creative_fly_tests.cpp`.
- Helper verbs added:
  - `bumpCreativeWorldEpoch(ProductAppWindowState&)`.
  - `seedCreativeFlyAnchorFromOrigin(ProductAppWindowState&)`.
  - `ensureFreshCreativeFlyAnchor(ProductAppWindowState&, const Session*)`.
  - `seedCreativeFlyAnchorFromScene(ProductAppWindowState&, Vec3)`.
  - `recordCreativeFlyAnchorIntegrated(ProductAppWindowState&, Vec3)`.
- Freshness semantics:
  - Fresh means non-`Unseeded` provenance and
    `seededFromWorldEpoch == window.creativeWorldEpoch`.
  - `bumpCreativeWorldEpoch(...)` increments the window-owned epoch.
  - Origin seed uses `{0, 6, 10}`, `OriginFramed`, and the current epoch.
  - `ensureFreshCreativeFlyAnchor(...)` preserves a fresh store and reseeds when
    unseeded/stale using the active player's position or `{}` when no session or
    player is available.
  - Scene and integrated writes stamp the supplied position, current epoch, and
    `SceneSeeded` / `FlyIntegrated` provenance respectively.
- Compatibility mirror behavior:
  - Every helper that writes the store also mirrors
    `window.viewport.creativeFlyAnchorValid = true` and
    `window.viewport.creativeFlyPositionMeters = positionMeters`.
  - No production call sites were migrated in this gate.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_fly_tests -j10`
    passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^product_creative_fly_tests$' --output-on-failure`
    passed.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched/new files passed.
- Concerns/deferred:
  - Production origin, lazy seeder, scene seeder, camera consumer, receipt, and
    legacy-field deletion work remains for G4-G7.
  - `Testing/Temporary/LastTest.log` remains dirty from CTest output and was
    intentionally left untouched.
