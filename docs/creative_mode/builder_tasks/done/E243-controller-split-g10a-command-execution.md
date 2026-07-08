# E243 - Controller Split G10a: Command Execution

Status: Ready

## Objective

Extract the gameplay command submit/tick/ledge-fall fallback cluster from
`src/app/iggy3d/gameplay/Controller.cpp` into a narrow command-execution helper.
This should preserve behavior while giving the remaining dash/move/target action
slices a real command executor seam to call.

## Context

`Controller.cpp` has already been split through:

- E232 kinematics
- E233 movement proof/debug writers
- E234 ground queries
- E235 jump/dash state helpers
- E236 wall surface queries
- E237 wall-run evaluation
- E238 traversal proof writers
- E239 player/session access
- E240 reset/fall helpers
- E241 target/outcome proof helpers
- E242 jump actions

After E242, `Controller.cpp` is about 650 lines. The next clean seam is the
command executor because `submitProductDash(...)`, `submitProductMove(...)`, and
`submitProductTargetCommand(...)` all route through the same local
`submitProductGameplayCommand(...)` helper.

## Scope

Add:

- `src/app/iggy3d/gameplay/ControllerCommandExecution.hpp`
- `src/app/iggy3d/gameplay/ControllerCommandExecution.cpp`

Move these helpers out of `Controller.cpp`:

- `commandMovementHasPhysicsFrameStats(...)`
- `tickProductGameplayCommand(...)`
- `applyProductLedgeFallMoveFallback(...)`
- `submitProductGameplayCommand(...)`

Export only:

```cpp
void submitProductGameplayCommand(Session& session,
                                  ProductAppWindowState& window,
                                  CommandRecord command,
                                  const SpatialSurfaceSet* collisionSurfaces);
```

Keep the other three moved helpers file-local in
`ControllerCommandExecution.cpp`.

Update:

- `Controller.cpp` to include `ControllerCommandExecution.hpp` and keep call
  sites only.
- `CMakeLists.txt` to compile the new `.cpp` next to the other controller split
  sources.

## Behavior To Preserve

- `gameplayInputUsed`, `gameplayCommand.submitted`, command kind, accepted state,
  rejection reason, reach gate, and command status strings.
- Move-command attempted/submitted setup and `clearProductMovementDebug(...)`.
- Collision surface proof fields and physics movement planner tick proof.
- `session.submitCommand(...)`, normal `session.tick(...)`, and
  `session.tickWithOptions(...)` selection.
- Tick status and tick reason-code mapping.
- Move debug recording after accepted move commands.
- Player position changed detection.
- Runtime movement blocked/moved/tick-failed status behavior.
- Ledge-fall move fallback behavior.
- `beginProductFallIfUnsupported(...)` call when a move does not apply the
  ledge-fall fallback and no jump is active.

## Non-Scope

Do not move or change:

- `submitProductDash(...)`
- `horizontalVelocityActive(...)`
- `updateProductGroundMovementVelocity(...)`
- `submitProductAirborneMove(...)`
- `submitProductMove(...)`
- `submitProductTargetCommand(...)`
- `queryProductGameplayTarget(...)`
- `ProductGameplayInputIntent` or input-intent sampling
- phase orchestration helpers
- jump action behavior
- dash behavior
- target query/reach behavior
- reset action behavior
- movement proof implementation
- target/outcome proof implementation
- ground-query, player-access, reset/fall, wall-run, or traversal helper
  implementations
- receipt keys/order/values
- CMake test definitions beyond adding the new source file if needed
- staging, commit, push, broad CTest, or window launch

## Dependency Guard

`ControllerCommandExecution.*` may depend on command/session execution,
`ProductAppWindowState`, `SpatialSurfaceSet`, player access, reset/fall,
ground queries, movement proof writers, target/outcome proof stringifiers, and
physics movement planner proof helpers.

It must not depend on dash-submit, move-submit, target-submit, input sampling,
phase orchestration, jump action orchestration, or target query/reach selection.

## Required Greps

Run and classify:

```sh
rg -n "commandMovementHasPhysicsFrameStats|tickProductGameplayCommand|applyProductLedgeFallMoveFallback|submitProductGameplayCommand" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerCommandExecution.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerCommandExecution.cpp
```

Expected:

- `submitProductGameplayCommand(...)` declaration in the new header.
- `submitProductGameplayCommand(...)` definition in the new `.cpp`.
- The three lower-level moved helpers defined only in the new `.cpp`.
- `Controller.cpp` retains call sites only.

Run a focused dependency grep over `ControllerCommandExecution.*` and confirm no
hits for:

```text
submitProductDash
submitProductMove
submitProductTargetCommand
queryProductGameplayTarget
sampleProductGameplayInputIntent
ProductGameplayInputIntent
applyProductDashPhase
updateProductRetainedHorizontalVelocityPhase
applyProductTargetActionPhase
applyProductResetActionPhase
applyProductGameplayActions
```

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

Stop and report instead of widening scope if:

- Moving this cluster forces movement-submit, dash-submit, target-submit, or
  input-intent code into the new file.
- Any command status, movement status, tick reason, rejection reason, reach gate,
  or receipt golden output changes.
- The new helper starts owning target query/reach policy or dash/move/target
  action policy.

## Completion Brief

Files changed:

- `CMakeLists.txt`
- `src/app/iggy3d/gameplay/Controller.cpp`
- `src/app/iggy3d/gameplay/ControllerCommandExecution.hpp`
- `src/app/iggy3d/gameplay/ControllerCommandExecution.cpp`
- `docs/creative_mode/builder_tasks/done/E243-controller-split-g10a-command-execution.md`

Exact API moved/added:

- Added `ControllerCommandExecution.hpp/.cpp`.
- Exported only `submitProductGameplayCommand(Session&, ProductAppWindowState&, CommandRecord, const SpatialSurfaceSet*)`.

Exact file-local helpers moved:

- `commandMovementHasPhysicsFrameStats(...)`
- `tickProductGameplayCommand(...)`
- `applyProductLedgeFallMoveFallback(...)`

Behavior preservation:

- Gameplay input used/submitted fields, command kind, accepted state, rejection reason, reach gate, and command status strings were preserved.
- Move-command attempted/submitted setup and `clearProductMovementDebug(...)` remain in the command execution path.
- Collision surface proof fields and physics movement planner tick proof are unchanged.
- `session.submitCommand(...)`, `session.tick(...)`, and `session.tickWithOptions(...)` selection are unchanged.
- Tick status and reason-code mapping are unchanged.
- Accepted move commands still record movement debug before physics planner tick proof.
- Player position changed detection and runtime movement blocked/moved/tick-failed behavior are unchanged.
- Ledge-fall move fallback and the `beginProductFallIfUnsupported(...)` fallback call remain in the command execution path.

Scope notes:

- `Controller.cpp` now includes `app/iggy3d/gameplay/ControllerCommandExecution.hpp` and retains call sites only.
- `CMakeLists.txt` now registers `src/app/iggy3d/gameplay/ControllerCommandExecution.cpp` next to the other controller split files.
- Dash submit, move submit, target submit, target query/reach policy, input sampling, phase orchestration, jump action behavior, reset action behavior, movement proof implementation, target/outcome proof implementation, receipt keys/order/values, staging, commit, push, broad CTest, and window launch were not changed.

Required grep classification:

- `rg -n "commandMovementHasPhysicsFrameStats|tickProductGameplayCommand|applyProductLedgeFallMoveFallback|submitProductGameplayCommand" ...` shows `submitProductGameplayCommand(...)` declared in `ControllerCommandExecution.hpp`.
- The same grep shows `submitProductGameplayCommand(...)` defined in `ControllerCommandExecution.cpp`.
- The same grep shows `commandMovementHasPhysicsFrameStats(...)`, `tickProductGameplayCommand(...)`, and `applyProductLedgeFallMoveFallback(...)` defined only in `ControllerCommandExecution.cpp`.
- The same grep shows `Controller.cpp` retains call sites only.
- Focused dependency grep over `ControllerCommandExecution.*` returned no hits for `submitProductDash`, `submitProductMove`, `submitProductTargetCommand`, `queryProductGameplayTarget`, `sampleProductGameplayInputIntent`, `ProductGameplayInputIntent`, `applyProductDashPhase`, `updateProductRetainedHorizontalVelocityPhase`, `applyProductTargetActionPhase`, `applyProductResetActionPhase`, or `applyProductGameplayActions`.

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10` passed.
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure` passed: 3/3 tests.
- `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` produced no diff.
- `git -C /Users/kogaryu/iggy3d diff --check` passed.
- Focused trailing-whitespace scan over touched files and this card passed.
