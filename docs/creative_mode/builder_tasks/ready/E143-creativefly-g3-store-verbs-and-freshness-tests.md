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
