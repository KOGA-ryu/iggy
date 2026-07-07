# E157 — GameplayStore G1: Lifecycle/Input Flags

## Objective

Start the GameplayStore regroup with a small compiler-guided subset. Add the
`GameplayStore` top-level member and move only the lifecycle/input flags:

```text
runtimeSessionCreated
gameplayActive
gameplayInputUsed
gameplayInputSource
gameplayTickAdvanced
playerPositionChanged
```

This is structural only. Do not move the remaining GameplayStore fields in this
card.

## Prerequisite

E154 has been decomposed into E157-E160. E157 is the only ready slice. E158-E160
stay blocked until reviewer releases them.

## Required Work

1. Add `src/app/iggy3d/gameplay/GameplayStore.hpp`.
2. Define `struct GameplayStore` with exactly the six fields above, preserving
   current types/defaults from `ProductAppWindowState`.
3. Add `GameplayStore gameplay;` to `ProductAppWindowState`.
4. Delete the six flat fields from `ProductAppWindowState`.
5. Fix compiler errors by repointing only these six fields:
   - `<window>.runtimeSessionCreated` -> `<window>.gameplay.runtimeSessionCreated`
   - `<window>.gameplayActive` -> `<window>.gameplay.gameplayActive`
   - `<window>.gameplayInputUsed` -> `<window>.gameplay.gameplayInputUsed`
   - `<window>.gameplayInputSource` -> `<window>.gameplay.gameplayInputSource`
   - `<window>.gameplayTickAdvanced` -> `<window>.gameplay.gameplayTickAdvanced`
   - `<window>.playerPositionChanged` -> `<window>.gameplay.playerPositionChanged`
6. Update `docs/god_struct_member_ownership.tsv`:
   - delete rows for the six moved fields;
   - add one row `gameplay	GameplayStore`.

## Do Not

- Do not move movement/planner fields; those belong to E158.
- Do not move action/outcome/tape fields; those belong to E159.
- Do not move visibility/render diagnostics; those belong to E160.
- Do not use broad `sed` or bare-token replacement.
- Do not touch receipt key names or regenerate the golden.
- Do not stage, commit, push, launch a window, or start E158.

## Required Greps

Report these after the change:

```sh
rg -n "window\\.(runtimeSessionCreated|gameplayActive|gameplayInputUsed|gameplayInputSource|gameplayTickAdvanced|playerPositionChanged)\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'
rg -n "\\b(runtimeSessionCreated|gameplayActive|gameplayInputUsed|gameplayInputSource|gameplayTickAdvanced|playerPositionChanged)\\b" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp
```

Expected: no old top-level window refs and no old top-level declarations. Hits
inside `GameplayStore.hpp` are expected.

## Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_receipt_key_order_tests|product_god_struct_ownership_coverage_tests|product_gameplay_controller_tests|product_window_input_frame_tests|product_menu_transitions_tests|product_ascii_room_activation_tests|product_interaction_mode_state_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run focused trailing-whitespace scan over touched files.

## Completion Brief

Keep the brief small:

- Files changed:
- Six fields moved:
- Receipt/ownership result:
- Required grep result:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief

- Files changed:
  - `src/app/iggy3d/gameplay/GameplayStore.hpp`
  - `src/app/iggy3d/ProductAppWindowState.hpp`
  - `docs/god_struct_member_ownership.tsv`
  - Production call sites in `Operations.cpp`, `ascii_room/Activation.cpp`, `automation/*`, `gameplay/*`, `menu/*`, `receipt/*`, `window/*`
  - Focused unit tests that read or seed the moved lifecycle/input flags
- Six fields moved:
  - `runtimeSessionCreated`
  - `gameplayActive`
  - `gameplayInputUsed`
  - `gameplayInputSource`
  - `gameplayTickAdvanced`
  - `playerPositionChanged`
  - These now live at `ProductAppWindowState::gameplay.<field>` in `GameplayStore`.
- Receipt/ownership result:
  - Receipt keys and golden output stayed unchanged.
  - `product_receipt_key_order_tests`: `1032 fields match golden`.
  - `product_god_struct_ownership_coverage_tests`: passed, with `gameplay	GameplayStore` replacing the six individual top-level rows.
- Required grep result:
  - `rg -n "window\\.(runtimeSessionCreated|gameplayActive|gameplayInputUsed|gameplayInputSource|gameplayTickAdvanced|playerPositionChanged)\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'`: no matches.
  - `rg -n "\\b(runtimeSessionCreated|gameplayActive|gameplayInputUsed|gameplayInputSource|gameplayTickAdvanced|playerPositionChanged)\\b" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp`: no matches.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build -j10`: passed.
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests`: passed.
  - `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests`: passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_receipt_key_order_tests|product_god_struct_ownership_coverage_tests|product_gameplay_controller_tests|product_window_input_frame_tests|product_menu_transitions_tests|product_ascii_room_activation_tests|product_interaction_mode_state_tests)$' --output-on-failure`: passed 7/7.
  - `git -C /Users/kogaryu/iggy3d diff --check`: passed.
  - Focused trailing-whitespace scan over changed files: passed.
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden`: empty.
- Concerns/deferred:
  - E158-E160 remain deferred; movement/planner, action/outcome/tape, and visibility/render diagnostics were intentionally not moved.
