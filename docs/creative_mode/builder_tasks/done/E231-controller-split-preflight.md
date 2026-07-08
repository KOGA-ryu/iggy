# E231 - Controller Split Preflight

## Status

Done.

## Objective

Audit current `src/app/iggy3d/gameplay/Controller.cpp` at HEAD and choose the
first safe split card for the new refactor-target backlog item:
`docs/refactor_targets.md` target #2, "Split Controller.cpp".

## Current Shape

- `src/app/iggy3d/gameplay/Controller.cpp`: 2428 lines.
- `src/app/iggy3d/gameplay/Controller.hpp`: 19 lines.
- Public export remains only:
  - `applyProductGameplayActions(Session&, const ActionState&, ProductAppWindowState&, std::string_view, const SpatialSurfaceSet*)`
- `Controller.cpp` contains one anonymous namespace with roughly these line
  ranges:
  - Lines 30-209: constants, command/target/outcome stringifiers, player lookup,
    movement debug clearing, horizontal speed proof.
  - Lines 212-380: wall-run candidate/active result structs and publishing.
  - Lines 381-423: movement profile/speed helpers and player position mutation.
  - Lines 424-676: ground query/reset/fall support.
  - Lines 677-766: jump timing/proof support.
  - Lines 768-1328: traversal, wall-jump, wall-run detection/evaluation, and
    traversal jump application.
  - Lines 1329-1443: jump advancement and jump submit path.
  - Lines 1444-1586: jump submit and dash submit path.
  - Lines 1587-1687: first-person direction, desired velocity, retained ground
    velocity, and pure horizontal velocity helpers.
  - Lines 1688-1819: airborne/ledge movement debug and airborne movement.
  - Lines 1820-1993: target/outcome proof and runtime movement debug copying.
  - Lines 1994-2180: command submission/tick, ledge-fall fallback, ground move
    submit.
  - Lines 2182-2238: target command submit.
  - Lines 2240-2405: input intent sampling and orchestration phases.
  - Lines 2408-2427: public `applyProductGameplayActions(...)` composition root.

## Caller/Test Surface

- Production direct callers:
  - `src/app/iggy3d/window/InputFrame.cpp`
  - `src/app/iggy3d/automation/AutomationGameplay.cpp`
  - `src/app/iggy3d/gameplay/ScriptedDriver.cpp`
- Direct test callers:
  - `tests/unit/product_gameplay_controller_tests.cpp`
  - `tests/unit/product_active_room_collision_tests.cpp`
- Focused behavioral guard:
  - `product_gameplay_controller_tests` is the main high-value gate. It covers
    manual movement, acceleration/deceleration, jump/coyote/buffer/cut/fall,
    wall jump, wall run, traversal, reset zones, dash, target interaction, and
    physics planner behavior through `applyProductGameplayActions(...)`.
- Smoke-level guard if a later split touches wall/traversal/dash receipts:
  - `product_gameplay_controls_smoke`
  - `product_frame_metrics_cli_smoke` for wall-run frame metrics only if that
    path is touched.

## Split Ordering Decision

Do not start with the wall-run/wall-jump cluster. It is large and cross-coupled:
it reads/writes jump state, wall-run proof state, traversal proof state,
movement debug facts, room collision, and player transforms.

Start with a very small mechanical extraction:

1. **G1 - Controller Kinematics Extraction**
   - Move pure first-person direction, movement profile/speed, desired velocity,
     move delta, horizontal velocity approach, and horizontal clamp helpers into
     a new controller-owned kinematics helper.
   - Add direct unit tests for the pure helper.
   - Repoint `Controller.cpp` call sites with no behavior change.

2. **G2 - Movement Proof Helpers**
   - Move clear/record/publish helpers for movement/jump/traversal/wall-run
     proof structs, after G1 gives shared kinematics names.

3. **G3 - Ground/Reset/Fall Queries**
   - Move ground query, reset-zone, fall-start, and ledge-fall fallback helpers.

4. **G4 - Jump/Dash Submit**
   - Move jump timing/submit/advance and dash submit paths.

5. **G5 - Wall Traversal**
   - Move wall-jump and wall-run candidate/active evaluation after G2-G4 reduce
     its dependency fan-in.

6. **G6 - Command/Target Orchestration**
   - Move command submission, target proof/outcome handling, and input intent
     phases only after previous helpers are split.

## Recommended Next Card

Release `E232-controller-split-g1-kinematics.md`.

## E232 Scope

- Add:
  - `src/app/iggy3d/gameplay/ControllerKinematics.hpp`
  - `src/app/iggy3d/gameplay/ControllerKinematics.cpp`
  - `tests/unit/product_gameplay_controller_kinematics_tests.cpp`
- Move/rename only pure helpers:
  - `manualFirstPersonDirection(...)`
  - `manualFirstPersonMaxSpeedMetersPerSecond(...)`
  - `manualFirstPersonMovementProfile(...)`
  - `manualFirstPersonMoveDelta(...)`
  - `manualFirstPersonDesiredVelocity(...)`
  - `moveHorizontalVelocityToward(...)`
  - `clampHorizontalVelocity(...)`
- Keep `Controller.cpp` as the orchestration owner.
- Do not move jump, dash, wall-run, wall-jump, traversal, command submission,
  target/outcome proof, reset/fall, or collision query logic.

## Suggested E232 Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_kinematics_tests product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_kinematics_tests|product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

## Self-Blockers For E232

- Stop if extracting the pure helpers requires changing `applyProductGameplayActions(...)`
  behavior or signatures.
- Stop if the helper starts depending on `Session`, `ProductAppWindowState`,
  `SpatialSurfaceSet`, or receipt/proof stores.
- Stop if CMake fallout expands beyond adding the new helper source and test
  target.

## Commands Run

- `git -C /Users/kogaryu/iggy3d status --short`
- `git -C /Users/kogaryu/iggy3d log --oneline -5`
- `sed -n '1,260p' /Users/kogaryu/iggy3d/docs/refactor_targets.md`
- `wc -l /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.hpp`
- `rg -n "...function inventory..." /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.cpp`
- `sed` reads over `Controller.cpp` in five line ranges.
- Focused `rg` over production/test callers and gameplay controller tests.

## Confirmation

- No source, test, CMake, fixture, receipt golden, staging, push, or window
  launch changes were made by this preflight.
