# E147: CreativeFly G7 - Receipt And Legacy Field Delete

## Objective

Finish the creative-fly anchor ownership migration: remove the legacy raw fields and make receipts/audits report the store-owned state.

This is the delete-last gate.

## Prerequisite

E142-E146 complete.

## Required Work

1. Delete legacy fields from `src/app/iggy3d/view/ViewportState.hpp`:
   - `creativeFlyAnchorValid`
   - `creativeFlyPositionMeters`

2. Remove compatibility mirroring from the store helper.

3. Update receipt fields in `src/app/iggy3d/receipt/GameplaySceneStateFields.cpp`:
   - Keep `creative_fly_world_x/y/z`, now sourced from the store position.
   - Replace/drop `creative_fly_anchor_valid` according to the ratified policy:
     - remove the raw valid bool from the golden;
     - add `creative_fly_anchor_provenance`;
     - add `creative_fly_anchor_world_epoch`.

4. Regenerate receipt golden intentionally:

   ```sh
   RECEIPT_GOLDEN_REGEN=1 ./build/product_receipt_key_order_tests
   ```

5. Update `docs/god_struct_member_ownership.tsv`:
   - remove rows for legacy raw fields if present;
   - add/confirm owner rows for `creativeWorldEpoch` and the store field.

6. Update all remaining tests to use the store helpers/accessors.

7. Run negative greps:

   ```sh
   rg -n "creativeFlyAnchorValid|creativeFlyPositionMeters" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
   rg -n "creative_fly_anchor_valid" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests /Users/kogaryu/iggy3d/tests/golden/product_receipt_key_order.golden
   ```

   Expected: no hits, except historical builder task docs if included accidentally. Do not rewrite historical done cards.

## Do Not

- Do not move `creativeFlyActive`, `creativeFlyStatus`, `creativeFlyReasonCode`, or `creativeFlySpeedMetersPerSecond`.
- Do not alter standalone app fly state.
- Do not change camera yaw/pitch policy.
- Do not change map-maker no-player latch policy.
- Do not stage, commit, push, or launch a window.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
RECEIPT_GOLDEN_REGEN=1 ./build/product_receipt_key_order_tests
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_receipt_key_order_tests|product_god_struct_ownership_coverage_tests|product_creative_world_launch_tests|product_window_input_frame_tests|product_vulkan_room_frame_tests|product_creative_fly_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Legacy fields deleted:
- Receipt fields changed:
- Golden field count before/after:
- Ownership TSV update:
- Negative grep results:
- Tests/checks run:
- Concerns/deferred:

## Completed Brief

- Files changed:
  - Updated `src/app/iggy3d/view/ViewportState.hpp`.
  - Updated `src/app/iggy3d/view/CreativeFlyAnchorStore.cpp`.
  - Updated `src/app/iggy3d/receipt/GameplaySceneStateFields.cpp`.
  - Updated `tests/golden/product_receipt_key_order.golden`.
  - Updated focused fly/window/launch/vulkan tests to use the store.
- Legacy fields deleted:
  - Removed `ProductViewportState::creativeFlyAnchorValid`.
  - Removed `ProductViewportState::creativeFlyPositionMeters`.
  - Removed store-helper compatibility mirroring to those fields.
- Receipt fields changed:
  - Removed `creative_fly_anchor_valid`.
  - Added `creative_fly_anchor_provenance`.
  - Added `creative_fly_anchor_world_epoch`.
  - Kept `creative_fly_world_x/y/z`, now sourced from
    `window.viewport.creativeFlyAnchor.positionMeters`.
- Golden field count before/after:
  - Before regen: 1031 fields.
  - After `RECEIPT_GOLDEN_REGEN=1 ./build/product_receipt_key_order_tests`:
    1032 fields.
  - Golden diff replaces `creative_fly_anchor_valid false` with
    `creative_fly_anchor_provenance Unseeded` and
    `creative_fly_anchor_world_epoch 0`.
- Ownership TSV update:
  - `creativeWorldEpoch` remains owned by `ViewportStore`.
  - `viewport` remains owned by `ViewportStore`.
  - No legacy raw-field ownership rows remain.
- Negative grep results:
  - `rg -n "creativeFlyAnchorValid|creativeFlyPositionMeters" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv`
    returned no hits.
  - `rg -n "creative_fly_anchor_valid" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests /Users/kogaryu/iggy3d/tests/golden/product_receipt_key_order.golden`
    returned no hits.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build -j10` passed.
  - `RECEIPT_GOLDEN_REGEN=1 ./build/product_receipt_key_order_tests`
    passed and regenerated the golden.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_receipt_key_order_tests|product_god_struct_ownership_coverage_tests|product_creative_world_launch_tests|product_window_input_frame_tests|product_vulkan_room_frame_tests|product_creative_fly_tests)$' --output-on-failure`
    passed: 6/6.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over source/test/task touched files passed.
    The tab-delimited receipt golden intentionally contains blank-value rows of
    the form `key<TAB>` and was excluded from that whitespace scan.
- Concerns/deferred:
  - No standalone app fly state was changed.
  - No yaw/pitch, map-maker no-player latch, or sibling
    `creativeFlyActive/status/reason/speed` policy was changed.
  - `Testing/Temporary/LastTest.log` remains dirty from CTest output and was
    intentionally left untouched.
