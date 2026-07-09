# File Spec

Files: `src/app/iggy3d/gameplay/TapeRunner.hpp`, `src/app/iggy3d/gameplay/TapeRunner.cpp`

Verified at: `f288fbd8`

## Owns

- Product gameplay tape execution request/result packets.
- Scripted command submission over parsed gameplay tape steps.
- Expected rejection and expected movement-block handling.
- Loop/completion proof facts for inventory, doors, treasure, NPCs, objectives, AI command facts, session outcome, and runtime hash.
- Options-driven tape load/run bridge that records run facts into the product window.

## Does Not Own

- Tape file parsing, session command admission rules, movement/tick kernels, active room construction, collision building, or receipt field formatting.
- Product input routing or gameplay controller live input.

## Reads

- `Session`, `ProductGameplayTape`, optional active room/collision/window pointers, physics move planner flag, command log boundary, world/inventory/objective/combat/AI state, and product app options.

## Writes / Mutates

- Submits script commands to `Session`.
- Ticks gameplay tape execution and may refresh active room collision through window/request collision state.
- Records tape run facts into `ProductAppWindowState` through internal receipt/proof helpers.

## Calls Out To / Wires Out To

- `loadProductGameplayTapeFile(...)`.
- `Session::submitCommand(...)`.
- `tickProductGameplayTape(...)`.
- `ensureActiveRoomCollisionFresh(...)` and `productActiveRoomCollisionSurfaces(...)`.
- Product tape action/rejection/movement-block naming helpers.

## Called By / Entry Points

- `productGameplayTapeSessionOutcomeName(...)`.
- `runProductGameplayTape(...)`.
- `runProductGameplayTapeFromOptions(...)`.
- `AppKernel.cpp` and tape runner tests.
- Grep proof: `rg -n "runProductGameplayTape|runProductGameplayTapeFromOptions|productGameplayTapeSessionOutcomeName" src tests/unit tests/smoke`.

## Invariants

- Missing session, missing tape, and empty tape fail explicitly.
- Expected rejection/movement-block steps are counted and must not be treated as ordinary executed steps.
- Collision freshness is checked before movement-sensitive tape ticks.
- Result proof fields must identify failed step/action/target/rejection/block without requiring a live window.
- Runtime state hash in the result reflects the session after the run.

## Tests / Proof Commands

- `rg -n "product_gameplay_tape_runner_tests|product_gameplay_tape_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "runProductGameplayTape|expectedRejectedStepCount|expectedBlockedStepCount|gameplay_tape_completed" tests/unit/product_gameplay_tape_runner_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/Tape.*` unless tape parse/action enums change.
- `src/runtime/session/*` unless command admission/tick behavior changes.
- `src/app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.*` unless freshness behavior changes.

## Update When

- Tape run request/result fields, command execution loop, failure statuses, expected rejection/block behavior, collision freshness use, or proof recording changes.

## Do Not Update When

- Only tape file syntax, live input routing, active room building, or renderer/HUD behavior changes.
