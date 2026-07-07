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
