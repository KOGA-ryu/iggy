# E246 - Controller Split G13a: Target Submit Helper

Status: Ready

## Objective

Extract the target command submit cluster from
`src/app/iggy3d/gameplay/Controller.cpp` into a narrow target-action helper.
This should preserve target discovery, reach-gate handling, interact/attack
command construction, and outcome proof recording while leaving input-intent
sampling and phase ordering in `Controller.cpp`.

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
- E243 command execution
- E244 dash actions
- E245 move actions

After E245, `Controller.cpp` is about 291 lines. The remaining target cluster is
now self-contained: `submitProductTargetCommand(...)` resolves target/reach
facts, records target/outcome proof, builds interact/attack commands, and calls
the command-execution seam.

## Scope

Add:

- `src/app/iggy3d/gameplay/ControllerTargetActions.hpp`
- `src/app/iggy3d/gameplay/ControllerTargetActions.cpp`

Move out of `Controller.cpp`:

- `queryProductGameplayTarget(...)`
- `submitProductTargetCommand(...)`

Export only:

```cpp
void submitProductTargetCommand(Session& session,
                                ProductAppWindowState& window,
                                CommandKind kind,
                                std::string_view source,
                                const SpatialSurfaceSet* collisionSurfaces);
```

Keep `queryProductGameplayTarget(...)` file-local in
`ControllerTargetActions.cpp`.

Update:

- `Controller.cpp` to include `ControllerTargetActions.hpp`.
- `applyProductTargetActionPhase(...)` to keep the existing
  `submitProductTargetCommand(...)` call sites only.
- `CMakeLists.txt` to compile the new `.cpp` next to the other controller split
  sources.

## Behavior To Preserve

- Target/outcome proof clearing before target handling.
- Actor lookup through `productPlayerActor(session)`.
- Target query shape: world pointer, actor id, command kind, zero aim radius,
  and the existing targeting flags.
- `recordProductTargetProof(...)` before no-target handling.
- No-target path:
  - `gameplayInputUsed`
  - `gameplayInputSource`
  - command kind string
  - command status `"no_target"`
  - reach gate `"not_attempted"`
  - interact outcome status `"no_target"`
- Reach query shape and `rejectionReasonForReach(...)` mapping.
- Reach gate assignment from `reachGateName(...)`.
- Interact/attack command payload shape, including attack damage `3`.
- Interact outcome snapshot before command dispatch.
- Dispatch through `submitProductGameplayCommand(...)`.
- Interact outcome proof recording after dispatch.
- `interactionExecuted` and `attackExecuted` assignment conditions.

## Non-Scope

Do not move or change:

- `applyProductTargetActionPhase(...)`
- `ProductGameplayInputIntent`
- input-intent sampling
- phase orchestration
- dash action behavior
- move action behavior
- jump action behavior
- command execution helper implementation
- target/outcome proof helper implementation
- reset action behavior
- movement proof behavior
- wall-run proof/evaluation behavior
- receipt keys/order/values
- CMake test definitions beyond adding the new source file if needed
- staging, commit, push, broad CTest, or window launch

## Dependency Guard

`ControllerTargetActions.*` may depend on target/reach runtime APIs,
player access, target/outcome proof helpers, command execution,
`ProductAppWindowState`, `Session`, `CommandRecord`, `CommandKind`, and
`SpatialSurfaceSet`.

It must not depend on dash-submit, move-submit, jump actions, input-intent
sampling, phase orchestration, reset phase handling, movement proof
implementation, wall-run evaluation, or command-execution internals.

## Required Greps

Run and classify:

```sh
rg -n "queryProductGameplayTarget|submitProductTargetCommand|applyProductTargetActionPhase|recordProductTargetProof|recordProductInteractionOutcomeProof" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerTargetActions.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerTargetActions.cpp
```

Expected:

- `submitProductTargetCommand(...)` declaration in the new header.
- `submitProductTargetCommand(...)` definition in the new `.cpp`.
- `queryProductGameplayTarget(...)` defined only in the new `.cpp`.
- `Controller.cpp` retains `applyProductTargetActionPhase(...)` and target
  submit call sites only.
- Target/outcome proof calls for this submit path move with the target helper.

Run a focused dependency grep over `ControllerTargetActions.*` and confirm no
hits for:

```text
submitProductDash
submitProductMove
submitProductJump
advanceProductJump
sampleProductGameplayInputIntent
ProductGameplayInputIntent
applyProductDashPhase
updateProductRetainedHorizontalVelocityPhase
applyProductTargetActionPhase
applyProductResetActionPhase
applyProductGameplayActions
recordProductMovementDebug
updateProductMovementStateProof
evaluateProductWallRun
tickProductGameplayCommand
applyProductLedgeFallMoveFallback
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

- Moving target submit requires moving `ProductGameplayInputIntent` or phase
  orchestration.
- Any target proof, outcome proof, command status, reach gate, attack damage,
  execution flag, or receipt golden output changes.
- The new helper starts owning dash/move/jump action policy, movement proof,
  wall-run evaluation, reset handling, or command-execution internals.
