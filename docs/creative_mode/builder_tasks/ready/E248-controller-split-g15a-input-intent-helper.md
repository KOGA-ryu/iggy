# E248 - Controller Split G15a: Input Intent Helper

Status: Ready

## Objective

Extract the product gameplay input-intent struct and sampling helpers from
`src/app/iggy3d/gameplay/Controller.cpp` into a small input-intent helper. This
leaves `Controller.cpp` focused on phase ordering while preserving exactly how
actions are sampled and how retained horizontal velocity keeps movement active.

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
- E246 target actions
- E247 reset actions

After E247, `Controller.cpp` is about 215 lines. The remaining implementation is
mostly input-intent sampling plus phase orchestration. This card extracts only
the reusable intent data and sampling predicates; phase ordering stays local.

## Scope

Add:

- `src/app/iggy3d/gameplay/ControllerInputIntent.hpp`
- `src/app/iggy3d/gameplay/ControllerInputIntent.cpp`

Move out of `Controller.cpp`:

- `ProductGameplayInputIntent`
- `sampleProductGameplayInputIntent(...)`
- `productGameplayIntentHasMovement(...)`

Export:

```cpp
struct ProductGameplayInputIntent {
  float moveX = 0.0F;
  float moveY = 0.0F;
  bool sprinting = false;
  bool jumpPressed = false;
  bool jumpReleased = false;
  bool dashPressed = false;
  bool interactPressed = false;
  bool attackPressed = false;
  bool resetPressed = false;
};

ProductGameplayInputIntent sampleProductGameplayInputIntent(
    const ActionState& actions);

bool productGameplayIntentHasMovement(
    const ProductGameplayInputIntent& intent,
    const ProductAppWindowState& window);
```

Update:

- `Controller.cpp` to include `ControllerInputIntent.hpp` and retain call sites
  only.
- `CMakeLists.txt` to compile the new `.cpp` next to the other controller split
  sources.

## Behavior To Preserve

- `moveX` still comes from `InputAction::PlayerMoveX`.
- `moveY` still comes from `InputAction::PlayerMoveY`.
- `sprinting` still comes from `InputAction::PlayerSprint`.
- `jumpPressed` / `jumpReleased` still come from `PlayerJump`.
- `dashPressed` still comes from `PlayerDash`.
- `interactPressed` still comes from `PlayerInteract`.
- `attackPressed` still comes from `PlayerAttack`.
- `resetPressed` still comes from `PlayerRetryOrReset`.
- Movement intent remains true when `moveX != 0.0F`, `moveY != 0.0F`, or
  `productHorizontalVelocityActive(window)`.

## Non-Scope

Do not move or change:

- `updateProductJumpTimingPhase(...)`
- `resolveProductWallRunCandidatePhase(...)`
- `applyProductActiveMovementStatePhase(...)`
- `publishProductMovementProofPhase(...)`
- `applyProductDashPhase(...)`
- `updateProductRetainedHorizontalVelocityPhase(...)`
- `applyProductTargetActionPhase(...)`
- `applyProductResetActionPhase(...)`
- `applyProductGameplayActions(...)`
- dash/move/target/reset/jump action behavior
- wall-run evaluation behavior
- movement proof behavior
- command execution behavior
- receipt keys/order/values
- CMake test definitions beyond adding the new source file if needed
- staging, commit, push, broad CTest, or window launch

## Dependency Guard

`ControllerInputIntent.*` may depend on `ActionState`, `InputAction`,
`ProductAppWindowState`, and `ControllerMoveActions.hpp` for
`productHorizontalVelocityActive(...)`.

It must not depend on session, collision surfaces, command submission, dash,
move, target, reset, jump actions, wall-run evaluation, movement proof writers,
or phase orchestration.

## Required Greps

Run and classify:

```sh
rg -n "ProductGameplayInputIntent|sampleProductGameplayInputIntent|productGameplayIntentHasMovement|productHorizontalVelocityActive" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerInputIntent.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerInputIntent.cpp
```

Expected:

- `ProductGameplayInputIntent` definition in the new header.
- `sampleProductGameplayInputIntent(...)` and
  `productGameplayIntentHasMovement(...)` declarations in the new header.
- `sampleProductGameplayInputIntent(...)` and
  `productGameplayIntentHasMovement(...)` definitions in the new `.cpp`.
- `Controller.cpp` retains call sites/usages only.
- `productHorizontalVelocityActive(...)` is called only from the new input
  intent helper among these three files.

Run a focused dependency grep over `ControllerInputIntent.*` and confirm no hits
for:

```text
Session
SpatialSurfaceSet
CommandRecord
submitProductDash
submitProductMove
submitProductTargetCommand
submitProductReset
submitProductJump
advanceProductJump
applyProductDashPhase
updateProductRetainedHorizontalVelocityPhase
applyProductTargetActionPhase
applyProductResetActionPhase
applyProductGameplayActions
recordProductMovementDebug
updateProductMovementStateProof
evaluateProductWallRun
submitProductGameplayCommand
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

- Moving input intent requires moving phase orchestration or action submitters.
- Any sampled action, movement-intent predicate, or receipt golden output
  changes.
- The new helper starts depending on session, command execution, collision
  surfaces, action submitters, wall-run evaluation, or movement proof writers.
