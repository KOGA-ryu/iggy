# File Spec

Files: `src/app/iggy3d/gameplay/Tape.hpp`, `src/app/iggy3d/gameplay/Tape.cpp`

Verified at: `dd59395f`

## Owns

- Product gameplay tape data model and parser/loader.
- Script action names, expected command-rejection names, expected movement-block names, parse failure status/reason fields, and file loading failure status.

## Does Not Own

- Tape execution, session ticking, target lookup, gameplay command submission, automation dispatch, receipt field writing, fixture contents, or smoke launch behavior.

## Reads

- Text tape content or filesystem path.
- Tape tokens for `move`, `interact`, `attack`, `wait`, `expect_reject`, and `expect_blocked`.
- Command rejection and movement blocked reason enums.

## Writes / Mutates

- Returns `ProductGameplayTapeParseResult` containing status, reason, line counts, failed line/token, and parsed tape steps.
- Does not mutate session, window state, files, runtime world, or receipts.

## Calls Out To / Wires Out To

- Uses filesystem existence check, file stream read, line/comment stripping, token splitting, and enum mapping helpers.
- Output is consumed by `TapeRunner.*`.

## Called By / Entry Points

- `parseProductGameplayTape(...)` and `loadProductGameplayTapeFile(...)`.
- Tape runner loads from product app options and unit tests parse inline tape text.
- Grep proof: `rg -n "parseProductGameplayTape|loadProductGameplayTapeFile|productGameplayTapeActionName|productGameplayTapeRejectionName|productGameplayTapeMovementBlockName" src/app/iggy3d tests/unit tests/smoke`.

## Invariants

- Blank/comment-only tapes fail as `gameplay_tape_empty`.
- Non-wait actions require exactly one target token.
- `expect_reject` applies only to `interact` and requires a non-`none` command rejection.
- `expect_blocked` applies only to `move` and requires a non-`none` movement block.
- File load failures return parse results without throwing.

## Tests / Proof Commands

- `rg -n "product_gameplay_tape_tests|product_gameplay_tape_runner_tests|product_gameplay_tape_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "gameplay_tape_unknown_action|gameplay_tape_empty|expect_reject|expect_blocked|parseProductGameplayTape" tests/unit/product_gameplay_tape_tests.cpp tests/unit/product_gameplay_tape_runner_tests.cpp tests/smoke/product_gameplay_tape_smoke.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/TapeRunner.*` unless execution semantics change.
- `src/runtime/command/Command.*` unless rejection enum names change.
- `src/runtime/movement/MovementCommand.*` unless movement blocked reasons change.

## Update When

- Tape grammar, action names, expected-result syntax, parse status strings, enum string mappings, or load failure behavior changes.

## Do Not Update When

- Only tape execution, receipt fields, smoke fixture contents, or gameplay command behavior changes without changing tape parsing/model contracts.
