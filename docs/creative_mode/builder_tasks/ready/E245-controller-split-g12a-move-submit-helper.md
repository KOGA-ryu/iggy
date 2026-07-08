# E245 - Controller Split G12a: Move Submit Helper

Status: Ready

## Objective

Extract the movement submit cluster from
`src/app/iggy3d/gameplay/Controller.cpp` into a narrow move-action helper. This
should preserve ground retained velocity, airborne manual movement, wall-run
air control, and move-command dispatch while leaving input-intent sampling and
phase orchestration in `Controller.cpp`.

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

After E244, `Controller.cpp` is about 438 lines. The next action seam is the
movement submit cluster: `submitProductMove(...)` owns the local movement
policy, while the remaining controller only needs to know whether retained
horizontal velocity should keep submitting movement.

## Scope

Add:

- `src/app/iggy3d/gameplay/ControllerMoveActions.hpp`
- `src/app/iggy3d/gameplay/ControllerMoveActions.cpp`

Move out of `Controller.cpp`:

- `horizontalVelocityActive(...)`
- `updateProductGroundMovementVelocity(...)`
- `submitProductAirborneMove(...)`
- `submitProductMove(...)`

Export:

```cpp
bool productHorizontalVelocityActive(const ProductAppWindowState& window);

void submitProductMove(Session& session,
                       ProductAppWindowState& window,
                       float moveX,
                       float moveY,
                       bool sprinting,
                       std::string_view source,
                       const SpatialSurfaceSet* collisionSurfaces);
```

Keep these file-local in `ControllerMoveActions.cpp`:

- `updateProductGroundMovementVelocity(...)`
- `submitProductAirborneMove(...)`

Rename `horizontalVelocityActive(...)` to the exported
`productHorizontalVelocityActive(...)` so the public helper name is controller
specific and does not look like a generic math primitive.

Update:

- `Controller.cpp` to include `ControllerMoveActions.hpp`.
- `productGameplayIntentHasMovement(...)` to call
  `productHorizontalVelocityActive(window)`.
- `updateProductRetainedHorizontalVelocityPhase(...)` to keep the
  `submitProductMove(...)` call site only.
- `CMakeLists.txt` to compile the new `.cpp` next to the other controller split
  sources.

## Behavior To Preserve

- Existing retained-horizontal-velocity activity threshold.
- Ground acceleration/deceleration choice based on input intent.
- Ground velocity clamp and writes to `groundVelocityX/Z`.
- Target/outcome proof clearing before move handling.
- Missing-player status and velocity clearing.
- Movement profile recording.
- Airborne manual movement path while jump/fall owns vertical motion.
- Wall-run tangent movement, wall-run speed multiplier clamp, and
  `"wall_run_input_away"` clear reason.
- Airborne mutation failure status/reason strings.
- Airborne moved status, `playerPositionChanged`, and movement debug recording.
- Ground retained velocity delta and zero-delta early return.
- Move command payload shape and dispatch through
  `submitProductGameplayCommand(...)`.
- `gameplayInputSource` assignment before command dispatch.

## Non-Scope

Do not move or change:

- `ProductGameplayInputIntent`
- `productGameplayIntentHasMovement(...)` except for the helper call rename
- `updateProductRetainedHorizontalVelocityPhase(...)`
- phase orchestration
- dash action behavior
- jump action behavior
- target submit helpers
- target query/reach behavior
- command execution helper implementation
- movement proof helper implementation
- wall-run evaluation/query helper implementation
- reset action behavior
- receipt keys/order/values
- CMake test definitions beyond adding the new source file if needed
- staging, commit, push, broad CTest, or window launch

## Dependency Guard

`ControllerMoveActions.*` may depend on kinematics, movement tuning, movement
proof writers, player access, wall-query helpers, target/outcome proof clearing,
command execution, `ProductAppWindowState`, `Session`, `CommandRecord`,
`EntityState`, and `SpatialSurfaceSet`.

It must not depend on target-submit helpers, target query/reach selection,
input-intent sampling, phase orchestration, dash action implementation, jump
action implementation, reset phase handling, or command-execution internals.

## Required Greps

Run and classify:

```sh
rg -n "horizontalVelocityActive|productHorizontalVelocityActive|updateProductGroundMovementVelocity|submitProductAirborneMove|submitProductMove|updateProductRetainedHorizontalVelocityPhase|productGameplayIntentHasMovement" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerMoveActions.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerMoveActions.cpp
```

Expected:

- `productHorizontalVelocityActive(...)` and `submitProductMove(...)`
  declarations in the new header.
- `productHorizontalVelocityActive(...)` and `submitProductMove(...)`
  definitions in the new `.cpp`.
- `updateProductGroundMovementVelocity(...)` and
  `submitProductAirborneMove(...)` defined only in the new `.cpp`.
- `Controller.cpp` retains `productGameplayIntentHasMovement(...)`,
  `updateProductRetainedHorizontalVelocityPhase(...)`, and call sites only.
- No remaining `horizontalVelocityActive(...)` symbol.

Run a focused dependency grep over `ControllerMoveActions.*` and confirm no hits
for:

```text
submitProductDash
submitProductTargetCommand
queryProductGameplayTarget
sampleProductGameplayInputIntent
ProductGameplayInputIntent
applyProductDashPhase
updateProductRetainedHorizontalVelocityPhase
applyProductTargetActionPhase
applyProductResetActionPhase
applyProductGameplayActions
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

- Moving move submit requires moving `ProductGameplayInputIntent` or phase
  orchestration.
- Any movement status/reason, wall-run clear reason, command payload, velocity
  update, or receipt golden output changes.
- The new helper starts owning target submit, target query/reach policy, dash
  action policy, jump action policy, or command-execution internals.
