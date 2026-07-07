# E158 — GameplayStore G2: Movement/Planner State

## Status

Ready. E157 is committed; claim this card next.

## Objective

Move the movement/planner GameplayStore fields into the existing
`ProductAppWindowState::gameplay` member:

```text
gameplayCommand
gameplayMovement
gameplayWallRun
gameplayJump
gameplayReset
gameplayTraversal
gameplayDash
gameplayCollision
gameplayTickReasonCode
physicsMovementPlanner
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

- Do not move action/outcome/tape fields; those belong to E159.
- Do not move visibility/render diagnostics; those belong to E160.
- Do not use broad `sed` or bare-token replacement.
- Do not change receipt keys or regenerate the golden.
- Do not stage, commit, push, launch a window, or start E159.

## Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_receipt_key_order_tests|product_god_struct_ownership_coverage_tests|product_gameplay_controller_tests|product_movement_debug_hud_tests|product_gameplay_tape_runner_tests|product_window_input_frame_tests)$' --output-on-failure
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

## Completion Brief

- Files changed:
  - `src/app/iggy3d/gameplay/GameplayStore.hpp`
  - `src/app/iggy3d/ProductAppWindowState.hpp`
  - `docs/god_struct_member_ownership.tsv`
  - Movement/planner production call sites in `automation/AutomationGameplay.cpp`, `gameplay/*`, `menu/*`, `receipt/*`, `window/*`
  - Focused unit tests that read or seed the moved movement/planner fields
  - `tools/iggy3d_product_frame_metrics/main.cpp`
- Ten fields moved:
  - `gameplayCommand`
  - `gameplayMovement`
  - `gameplayWallRun`
  - `gameplayJump`
  - `gameplayReset`
  - `gameplayTraversal`
  - `gameplayDash`
  - `gameplayCollision`
  - `gameplayTickReasonCode`
  - `physicsMovementPlanner`
  - These now live at `ProductAppWindowState::gameplay.<field>` in `GameplayStore`.
- Receipt/ownership result:
  - Receipt keys and golden output stayed unchanged.
  - `product_receipt_key_order_tests`: `1032 fields match golden`.
  - `product_god_struct_ownership_coverage_tests`: passed, with the ten individual top-level ownership rows removed and the existing `gameplay	GameplayStore` row retained.
- Required grep result:
  - `rg -n "window\\.(gameplayCommand|gameplayMovement|gameplayWallRun|gameplayJump|gameplayReset|gameplayTraversal|gameplayDash|gameplayCollision|gameplayTickReasonCode|physicsMovementPlanner)\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests /Users/kogaryu/iggy3d/tools /Users/kogaryu/iggy3d/apps --glob '*.cpp' --glob '*.hpp'`: no matches.
  - `rg -n "\\b(gameplayCommand|gameplayMovement|gameplayWallRun|gameplayJump|gameplayReset|gameplayTraversal|gameplayDash|gameplayCollision|gameplayTickReasonCode|physicsMovementPlanner)\\b" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp`: no matches.
  - `rg --pcre2 -n "(?<!gameplay)\\.(gameplayCommand|gameplayMovement|gameplayWallRun|gameplayJump|gameplayReset|gameplayTraversal|gameplayDash|gameplayCollision|gameplayTickReasonCode|physicsMovementPlanner)\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests /Users/kogaryu/iggy3d/tools /Users/kogaryu/iggy3d/apps --glob '*.cpp' --glob '*.hpp'`: no matches.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build -j10`: passed.
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests`: passed.
  - `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests`: passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_receipt_key_order_tests|product_god_struct_ownership_coverage_tests|product_gameplay_controller_tests|product_movement_debug_hud_tests|product_gameplay_tape_runner_tests|product_window_input_frame_tests)$' --output-on-failure`: passed 6/6.
  - `git -C /Users/kogaryu/iggy3d diff --check`: passed.
  - Focused trailing-whitespace scan over changed files: passed.
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden`: empty.
- Concerns/deferred:
  - E159/E160 remain deferred; action/outcome/tape and visibility/render diagnostics were intentionally not moved.
