# E160 — GameplayStore G4: Visibility/Diagnostics And Completion

## Status

Ready. E159 is committed; claim this card next.

## Objective

Move the final visibility/render diagnostic GameplayStore fields into the
existing `ProductAppWindowState::gameplay` member and mark the parent complete:

```text
playerVisible
roomVisible
objectiveVisible
rendererMutatedRuntime
scriptedGameplaySmoke
sceneItemCount
debugItemCount
```

## Required Work

1. Extend `src/app/iggy3d/gameplay/GameplayStore.hpp` with the seven fields
   above, preserving current types/defaults from `ProductAppWindowState`.
2. Delete the seven flat fields from `ProductAppWindowState`.
3. Fix compiler errors by repointing only this field group to
   `<window>.gameplay.<field>`.
4. Update `docs/god_struct_member_ownership.tsv` by deleting the seven moved
   field rows. Do not add another `gameplay` row.
5. Update `docs/god_struct_decomposition_target_map.md` for GameplayStore #8:
   mark the structural move complete and note that no freshness token was added.
6. Update `docs/creative_mode/builder_tasks/PRIORITY.md` to mark E157-E160
   complete and leave E155/E156 blocked until planner slices them.

## Do Not

- Do not touch foreign `playerVisible`/`roomVisible`/`objectiveVisible` members
  on draw-list/render/projection types unless the compiler proves they are the
  `ProductAppWindowState` field.
- Do not use broad `sed` or bare-token replacement.
- Do not change receipt keys or regenerate the golden.
- Do not release E155 or E156.
- Do not stage, commit, push, or launch a window.

## Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests
ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run focused trailing-whitespace scan over touched files.

## Completion Brief

- Files changed:
- Seven fields moved:
- Receipt/ownership result:
- Required grep result:
- Full suite result:
- Concerns/deferred:

## Completion Brief - Builder

- Files changed:
  - `src/app/iggy3d/gameplay/GameplayStore.hpp`
  - `src/app/iggy3d/ProductAppWindowState.hpp`
  - `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
  - `src/app/iggy3d/gameplay/ScriptedDriver.cpp`
  - `src/app/iggy3d/receipt/GameplayRuntimeMovementFields.cpp`
  - `tools/iggy3d_product_frame_metrics/main.cpp`
  - `docs/god_struct_member_ownership.tsv`
  - `docs/god_struct_decomposition_target_map.md`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
  - `docs/creative_mode/builder_tasks/claimed/E160-gameplaystore-g4-visibility-diagnostics.md`
- Seven fields moved:
  - Added to `ProductAppWindowState::gameplay` via `GameplayStore`: `playerVisible`, `roomVisible`, `objectiveVisible`, `rendererMutatedRuntime`, `scriptedGameplaySmoke`, `sceneItemCount`, `debugItemCount`.
  - Removed the seven flat `ProductAppWindowState` members and repointed ProductAppWindowState call sites to `window.gameplay.<field>`.
  - Kept foreign same-name fields on draw-list/render/projection/options types untouched.
  - Deleted the seven moved rows from `docs/god_struct_member_ownership.tsv`; no new `gameplay` row was added.
  - Updated `docs/god_struct_decomposition_target_map.md` to mark GameplayStore complete as E157-E160 with no freshness token added.
  - Updated `docs/creative_mode/builder_tasks/PRIORITY.md` to mark E157-E160 complete and leave E155/E156 blocked.
- Receipt/ownership result:
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests`: `receipt key-order oracle: 1032 fields match golden (order + values)`.
  - `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests`: passed with `assigned=147`; only the `gameplay` member remains assigned to GameplayStore in the flat ownership table.
  - `git diff -- tests/golden/product_receipt_key_order.golden`: no output.
- Required grep result:
  - Flat `ProductAppWindowState` declarations for the seven fields: no matches.
  - Old ownership rows for the seven fields: no matches.
  - Flat ProductAppWindowState member accesses for moved fields: no matches.
  - Remaining same-name hits are foreign fields on `Options`, `ProductGameplayProjectionFrame`, `PrimitiveDrawList`, `RenderBridge`, and related tests/render call sites; those were intentionally not moved.
- Full suite result:
  - `cmake --build /Users/kogaryu/iggy3d/build -j10` passed.
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests` passed.
  - `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure` passed, 260/260.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed with no output.
  - Focused trailing-whitespace scan over touched files had no hits.
- Concerns/deferred:
  - E155/E156 remain blocked and were not released.
  - No commits, staging, pushes, window launch, receipt-golden regeneration, or broad scope beyond E160.
