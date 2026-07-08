# E247 - Controller Split G14a: Reset Action Helper

Status: Ready

## Objective

Extract the gameplay reset action body from
`src/app/iggy3d/gameplay/Controller.cpp` into a narrow reset-action helper. This
keeps reset submission behavior owned outside the remaining controller
orchestrator while leaving input-intent sampling and phase ordering in
`Controller.cpp`.

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

After E246, `Controller.cpp` is about 223 lines and contains mostly input
intent plus phase orchestration. Reset is the last action body still embedded in
a phase helper.

## Scope

Add:

- `src/app/iggy3d/gameplay/ControllerResetActions.hpp`
- `src/app/iggy3d/gameplay/ControllerResetActions.cpp`

Move the reset submission body out of `applyProductResetActionPhase(...)` into:

```cpp
void submitProductReset(Session& session,
                        ProductAppWindowState& window,
                        std::string_view source);
```

Update:

- `Controller.cpp` to include `ControllerResetActions.hpp`.
- `applyProductResetActionPhase(...)` to keep only the `intent.resetPressed`
  guard plus a call to `submitProductReset(...)`.
- `CMakeLists.txt` to compile the new `.cpp` next to the other controller split
  sources.

## Behavior To Preserve

- Reset is attempted only when `intent.resetPressed` is true.
- `session.resetToBaseline()` still supplies the accepted/rejected state.
- Target and outcome proof clearing before reset receipt writes.
- `gameplayInputUsed` and `gameplayInputSource`.
- Command kind string `"reset"`.
- `gameplayCommand.submitted = true`.
- `gameplayCommand.accepted = reset.reset`.
- Command status mapping to `"accepted"` or `"rejected"`.
- Existing branch-gate comment location may move with the reset write if needed,
  but behavior and strings must not change.

## Non-Scope

Do not move or change:

- `applyProductResetActionPhase(...)` guard/phase ownership
- `ProductGameplayInputIntent`
- input-intent sampling
- phase orchestration
- dash action behavior
- move action behavior
- target action behavior
- jump action behavior
- command execution helper implementation
- target/outcome proof helper implementation
- movement proof behavior
- wall-run proof/evaluation behavior
- receipt keys/order/values
- CMake test definitions beyond adding the new source file if needed
- staging, commit, push, broad CTest, or window launch

## Dependency Guard

`ControllerResetActions.*` may depend on `Session`, `ProductAppWindowState`,
target/outcome proof clearing helpers, and `std::string_view`.

It must not depend on dash-submit, move-submit, target-submit, jump actions,
input-intent sampling, phase orchestration, movement proof implementation,
wall-run evaluation, target query/reach selection, or command-execution
internals.

## Required Greps

Run and classify:

```sh
rg -n "submitProductReset|applyProductResetActionPhase|resetToBaseline|gameplayCommand.kind = \"reset\"|clearProductTargetProof|clearProductOutcomeProof" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerResetActions.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerResetActions.cpp
```

Expected:

- `submitProductReset(...)` declaration in the new header.
- `submitProductReset(...)` definition in the new `.cpp`.
- `resetToBaseline(...)`, target/outcome proof clears, and reset command field
  writes live in the new `.cpp`.
- `Controller.cpp` retains `applyProductResetActionPhase(...)`, the
  `intent.resetPressed` guard, and a call site only.

Run a focused dependency grep over `ControllerResetActions.*` and confirm no
hits for:

```text
submitProductDash
submitProductMove
submitProductTargetCommand
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
queryProductGameplayTarget
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

- Moving reset submit requires moving `ProductGameplayInputIntent` or phase
  orchestration.
- Any reset command kind/status/accepted field, proof clearing behavior, or
  receipt golden output changes.
- The new helper starts owning dash/move/target/jump action policy, movement
  proof, wall-run evaluation, target query/reach policy, or command execution.
