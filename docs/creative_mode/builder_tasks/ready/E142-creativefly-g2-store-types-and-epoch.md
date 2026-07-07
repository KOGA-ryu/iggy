# E142: CreativeFly G2 - Store Types And World Epoch

## Objective

Gate 2 of the `creativeFly` anchor ownership fix. Add the window-owned epoch and the anchor-store types, but do not wire production consumers yet.

This is a type/coverage slice. Keep behavior identical.

## Context

Ratified preflight: `docs/creative_mode/builder_tasks/done/E141-creativefly-anchor-store-preflight.md`.

Current raw anchor fields live in:

- `src/app/iggy3d/view/ViewportState.hpp`
  - `creativeFlyAnchorValid`
  - `creativeFlyPositionMeters`

Those fields stay in this gate for compatibility. They are deleted only in G7.

## Required Work

1. Add a new window-owned epoch field:
   - `ProductAppWindowState::creativeWorldEpoch = 0`
   - Put it near the viewport/creative window state where ownership is clear.
   - Add/update `docs/god_struct_member_ownership.tsv`.

2. Add a small store type, preferably near the viewport domain:
   - suggested file: `src/app/iggy3d/view/CreativeFlyAnchorStore.hpp`
   - `.cpp` only if helper definitions require it.

3. Suggested API shape:

   ```cpp
   enum class ProductCreativeFlyAnchorProvenance {
     Unseeded,
     OriginFramed,
     PlayerSeeded,
     SceneSeeded,
     FlyIntegrated,
   };

   struct ProductCreativeFlyAnchorStore {
     Vec3 positionMeters;
     ProductCreativeFlyAnchorProvenance provenance =
         ProductCreativeFlyAnchorProvenance::Unseeded;
     std::uint64_t seededFromWorldEpoch = 0;
   };
   ```

4. Add pure helpers for names/queries only:
   - `productCreativeFlyAnchorProvenanceName(...)`
   - `productCreativeFlyAnchorAvailable(...)`
   - `productCreativeFlyAnchorFreshForEpoch(...)`

5. Add `ProductCreativeFlyAnchorStore creativeFlyAnchor;` to `ProductViewportState`.

6. Add narrow tests for:
   - default store is unseeded/unavailable.
   - each provenance string is stable.
   - matching epoch + non-unseeded provenance is fresh.
   - stale epoch is not fresh.

## Do Not

- Do not remove `creativeFlyAnchorValid`.
- Do not remove `creativeFlyPositionMeters`.
- Do not change launch/open/map-maker/navigation behavior.
- Do not change receipt fields or regenerate receipt golden.
- Do not touch standalone app fly state.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Suggested Verification

Run the narrowest target that covers the new tests. If a new test target is added:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_fly_anchor_store_tests product_god_struct_ownership_coverage_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_fly_anchor_store_tests|product_god_struct_ownership_coverage_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Epoch field added:
- Store/API shape:
- Legacy fields preserved:
- Tests/checks run:
- Concerns/deferred:
