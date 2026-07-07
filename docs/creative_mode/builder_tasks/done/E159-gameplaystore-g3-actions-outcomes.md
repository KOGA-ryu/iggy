# E159 — GameplayStore G3: Actions/Outcomes/Tape

## Status

Ready. E158 is committed; claim this card next.

## Objective

Move the action/outcome/tape GameplayStore fields into the existing
`ProductAppWindowState::gameplay` member:

```text
targetDiscovered
gameplayTarget
gameplayOutcome
sessionOutcome
gameplayTape
interactionExecuted
attackExecuted
productTransition
gameplayReachGate
gameplayLastRejection
```

## Required Work

1. Extend `src/app/iggy3d/gameplay/GameplayStore.hpp` with the ten fields above,
   preserving current types/defaults from `ProductAppWindowState`.
2. Delete the ten flat fields from `ProductAppWindowState`.
3. Fix compiler errors by repointing only this field group to
   `<window>.gameplay.<field>`.
4. Update `docs/god_struct_member_ownership.tsv` by deleting the ten moved field
   rows. Do not add another `gameplay` row.

## Do Not

- Do not move visibility/render diagnostics; those belong to E160.
- Do not use broad `sed` or bare-token replacement.
- Do not change receipt keys or regenerate the golden.
- Do not stage, commit, push, launch a window, or start E160.

## Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_receipt_key_order_tests|product_god_struct_ownership_coverage_tests|product_gameplay_controller_tests|product_gameplay_feedback_tests|product_gameplay_tape_runner_tests|product_menu_transitions_tests|product_window_input_frame_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run focused trailing-whitespace scan over touched files.

## Completion Brief

- Files changed:
- Ten fields moved:
- Receipt/ownership result:
- Required grep result:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief - Builder

- Files changed:
  - `src/app/iggy3d/gameplay/GameplayStore.hpp`
  - `src/app/iggy3d/ProductAppWindowState.hpp`
  - `src/app/iggy3d/gameplay/Controller.cpp`
  - `src/app/iggy3d/gameplay/GameplayFeedback.cpp`
  - `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
  - `src/app/iggy3d/gameplay/ScriptedDriver.cpp`
  - `src/app/iggy3d/gameplay/TapeRunner.cpp`
  - `src/app/iggy3d/menu/Transitions.cpp`
  - `src/app/iggy3d/receipt/GameplaySceneStateFields.cpp`
  - `tests/unit/product_gameplay_feedback_tests.cpp`
  - `tests/unit/product_menu_transitions_tests.cpp`
  - `tests/unit/product_vulkan_room_frame_tests.cpp`
  - `docs/god_struct_member_ownership.tsv`
  - `docs/creative_mode/builder_tasks/claimed/E159-gameplaystore-g3-actions-outcomes.md`
- Ten fields moved:
  - Added to `ProductAppWindowState::gameplay` via `GameplayStore`: `targetDiscovered`, `gameplayTarget`, `gameplayOutcome`, `sessionOutcome`, `gameplayTape`, `interactionExecuted`, `attackExecuted`, `productTransition`, `gameplayReachGate`, `gameplayLastRejection`.
  - Removed the ten flat `ProductAppWindowState` members and repointed ProductAppWindowState call sites to `window.gameplay.<field>` / `request.window.gameplay.<field>`.
  - Left non-window tape runner result fields such as `ProductGameplayTapeRunResult::sessionOutcome` unchanged.
- Receipt/ownership result:
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests`: `receipt key-order oracle: 1032 fields match golden (order + values)`.
  - `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests`: passed with `assigned=154`; the ten moved rows were deleted from `docs/god_struct_member_ownership.tsv`; no new `gameplay` row was added.
  - `git diff -- tests/golden/product_receipt_key_order.golden`: no output.
- Required grep result:
  - Flat `ProductAppWindowState` declarations for the ten fields: no matches.
  - Old ownership rows for the ten fields: no matches.
  - Flat window/member accesses for moved fields: no matches. Remaining `.sessionOutcome` matches are the existing tape runner result/test structs, not `ProductAppWindowState`.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build -j10` passed.
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests` passed.
  - `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_receipt_key_order_tests|product_god_struct_ownership_coverage_tests|product_gameplay_controller_tests|product_gameplay_feedback_tests|product_gameplay_tape_runner_tests|product_menu_transitions_tests|product_window_input_frame_tests)$' --output-on-failure` passed, 7/7.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed with no output.
  - Focused trailing-whitespace scan over touched files had no hits.
- Concerns/deferred:
  - E160 visibility/render diagnostics were not moved.
  - No commits, staging, pushes, window launch, broad CTest, or receipt-golden regeneration.
