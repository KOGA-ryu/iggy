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
