# E244 - Controller Split G11a: Dash Submit Helper

Status: Ready

## Objective

Extract the dash submit helper from `src/app/iggy3d/gameplay/Controller.cpp`
into a narrow dash-action helper. This keeps the dash policy owned outside the
remaining controller orchestrator without moving input-intent sampling or phase
ordering.

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

After E243, `Controller.cpp` is about 493 lines. `submitProductDash(...)` is now
a clean leaf submitter: it prepares dash state and a move command, then calls the
new command-execution seam.

## Scope

Add:

- `src/app/iggy3d/gameplay/ControllerDashActions.hpp`
- `src/app/iggy3d/gameplay/ControllerDashActions.cpp`

Move out of `Controller.cpp`:

- `submitProductDash(...)`

Export:

```cpp
void submitProductDash(Session& session,
                       ProductAppWindowState& window,
                       float moveX,
                       float moveY,
                       std::string_view source,
                       const SpatialSurfaceSet* collisionSurfaces);
```

Update:

- `Controller.cpp` to include `ControllerDashActions.hpp` and keep the
  `submitProductDash(...)` call site in `applyProductDashPhase(...)`.
- `CMakeLists.txt` to compile the new `.cpp` next to the other controller split
  sources.

## Behavior To Preserve

- Target and outcome proof clearing before dash handling.
- `gameplayInputUsed`, `gameplayInputSource`, and `gameplayDash.requested`.
- Cooldown rejection with status `"cooldown"` and reason
  `"gameplay_dash_cooldown"`.
- Missing-player rejection with status `"missing_player"` and reason
  `"gameplay_dash_missing_player"`.
- Dash direction from `productManualFirstPersonDirection(...)` and viewport
  camera yaw.
- Dash distance as `dashSpeedMetersPerSecond * dashDurationSeconds`.
- Accepted dash status/reason strings, speed, distance, cooldown, direction,
  movement profile, and max speed fields.
- Move command payload shape and dispatch through
  `submitProductGameplayCommand(...)`.

## Non-Scope

Do not move or change:

- `applyProductDashPhase(...)`
- `advanceProductDashCooldown(...)`
- `rejectProductDash(...)`
- jump/dash state helper implementation
- movement submit helpers
- target submit helpers
- command execution helper implementation
- input-intent sampling
- phase orchestration
- jump action behavior
- target query/reach behavior
- reset action behavior
- movement proof implementation
- target/outcome proof implementation
- receipt keys/order/values
- CMake test definitions beyond adding the new source file if needed
- staging, commit, push, broad CTest, or window launch

## Dependency Guard

`ControllerDashActions.*` may depend on dash state helpers, kinematics, movement
tuning, player access, target/outcome proof clearing, command execution,
`ProductAppWindowState`, `Session`, `CommandRecord`, and `SpatialSurfaceSet`.

It must not depend on movement-submit helpers, target-submit helpers,
input-intent sampling, phase orchestration, jump action orchestration, reset
phase handling, target query/reach selection, or command-execution internals.

## Required Greps

Run and classify:

```sh
rg -n "submitProductDash|applyProductDashPhase|rejectProductDash|advanceProductDashCooldown" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerDashActions.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerDashActions.cpp
```

Expected:

- `submitProductDash(...)` declaration in the new header.
- `submitProductDash(...)` definition in the new `.cpp`.
- `Controller.cpp` retains only the call in `applyProductDashPhase(...)`.
- `applyProductDashPhase(...)` remains in `Controller.cpp`.
- `rejectProductDash(...)` remains owned by `ControllerJumpDashState.*`.
- `advanceProductDashCooldown(...)` remains owned by
  `ControllerJumpDashState.*` and called from the jump-timing phase.

Run a focused dependency grep over `ControllerDashActions.*` and confirm no hits
for:

```text
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

- Moving dash submit requires moving phase orchestration or input-intent types.
- Any dash status/reason, movement profile/speed, command payload, or receipt
  golden output changes.
- The new helper starts owning move-submit, target-submit, target query, or
  command-execution internals.
