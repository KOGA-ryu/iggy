# E249: Controller Split G16a - Action Phase Orchestration

## Objective

Extract the remaining private gameplay action phase orchestration from
`Controller.cpp` into a narrow controller-owned helper, while keeping
`applyProductGameplayActions(...)` as the public facade.

This is a mechanical split after E248. The new helper should own phase ordering
and call the already-extracted action/proof/query seams. It must not change
gameplay behavior, receipt fields, command execution, or input sampling.

## Current Context

After E248, `src/app/iggy3d/gameplay/Controller.cpp` is about 180 lines and
contains only:

- private phase helpers:
  - `updateProductJumpTimingPhase(...)`
  - `resolveProductWallRunCandidatePhase(...)`
  - `applyProductActiveMovementStatePhase(...)`
  - `publishProductMovementProofPhase(...)`
  - `applyProductDashPhase(...)`
  - `updateProductRetainedHorizontalVelocityPhase(...)`
  - `applyProductTargetActionPhase(...)`
  - `applyProductResetActionPhase(...)`
- public facade:
  - `applyProductGameplayActions(...)`

The already-extracted seams are stable enough for this slice:

- `ControllerInputIntent.*`
- `ControllerDashActions.*`
- `ControllerMoveActions.*`
- `ControllerTargetActions.*`
- `ControllerResetActions.*`
- `ControllerJumpActions.*`
- `ControllerJumpDashState.*`
- `ControllerMovementProof.*`
- `ControllerPlayerAccess.*`
- `ControllerWallRunEvaluation.*`

## Scope

Add:

- `src/app/iggy3d/gameplay/ControllerActionPhases.hpp`
- `src/app/iggy3d/gameplay/ControllerActionPhases.cpp`

Add the new `.cpp` to the `iggy3d` CMake source list near the other controller
split files.

Move these private helpers out of `Controller.cpp` into
`ControllerActionPhases.cpp` as file-local helpers:

- `updateProductJumpTimingPhase(...)`
- `resolveProductWallRunCandidatePhase(...)`
- `applyProductActiveMovementStatePhase(...)`
- `publishProductMovementProofPhase(...)`
- `applyProductDashPhase(...)`
- `updateProductRetainedHorizontalVelocityPhase(...)`
- `applyProductTargetActionPhase(...)`
- `applyProductResetActionPhase(...)`

Expose only this new helper from `ControllerActionPhases.hpp`:

```cpp
void applyProductGameplayActionPhases(Session& session,
                                      const ProductGameplayInputIntent& intent,
                                      ProductAppWindowState& window,
                                      std::string_view source,
                                      const SpatialSurfaceSet* collisionSurfaces);
```

Forward-declare `ProductAppWindowState`, `ProductGameplayInputIntent`,
`Session`, and `SpatialSurfaceSet` in the header where possible. Include
`<string_view>`.

Update `Controller.cpp` so it includes `ControllerActionPhases.hpp` and keeps
only the public facade:

```cpp
void applyProductGameplayActions(Session& session,
                                 const ActionState& actions,
                                 ProductAppWindowState& window,
                                 std::string_view source,
                                 const SpatialSurfaceSet* collisionSurfaces) {
  const ProductGameplayInputIntent intent =
      sampleProductGameplayInputIntent(actions);
  applyProductGameplayActionPhases(
      session, intent, window, source, collisionSurfaces);
}
```

## Required Behavior Preservation

Preserve the exact current phase order inside
`applyProductGameplayActionPhases(...)`:

1. `updateProductJumpTimingPhase(...)`
2. `applyProductDashPhase(...)`
3. if dash phase returns true, return early
4. `updateProductRetainedHorizontalVelocityPhase(...)`
5. `applyProductTargetActionPhase(...)`
6. `applyProductResetActionPhase(...)`
7. `publishProductMovementProofPhase(...)`

Preserve all existing branch-gate comments that move with the phase helpers.

Preserve all existing calls and their argument order:

- `advanceProductDashCooldown(...)`
- `clearProductWallRunActiveProof(...)`
- `submitProductJump(...)`
- `applyProductJumpReleaseCut(...)`
- `advanceProductJump(...)`
- `productPlayerEntity(...)`
- `evaluateProductWallRun(...)`
- `publishProductWallRunEvaluation(...)`
- `updateProductMovementStateProof(...)`
- `submitProductDash(...)`
- `productGameplayIntentHasMovement(...)`
- `submitProductMove(...)`
- `submitProductTargetCommand(...)`
- `submitProductReset(...)`

## Non-Goals

Do not change:

- `applyProductGameplayActions(...)` public signature.
- input sampling behavior or `ControllerInputIntent.*`.
- dash, move, target, reset, jump, wall-run, movement-proof, player-access, or
  command-execution helper implementations.
- command payloads, command submission, session ticking, target query behavior,
  movement proof behavior, wall-run evaluation behavior, reset/fall behavior, or
  receipt keys/order/values.
- tests except build-system registration if needed by existing target builds.
- receipt golden files.

Do not introduce a broad controller class or stateful phase object.

## Required Grep Classification

Run:

```sh
rg -n "updateProductJumpTimingPhase|resolveProductWallRunCandidatePhase|applyProductActiveMovementStatePhase|publishProductMovementProofPhase|applyProductDashPhase|updateProductRetainedHorizontalVelocityPhase|applyProductTargetActionPhase|applyProductResetActionPhase|applyProductGameplayActionPhases|applyProductGameplayActions" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/Controller.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerActionPhases.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerActionPhases.cpp
```

Expected:

- `ControllerActionPhases.hpp` declares only
  `applyProductGameplayActionPhases(...)`.
- `ControllerActionPhases.cpp` defines
  `applyProductGameplayActionPhases(...)` and owns the lower-level phase helper
  definitions.
- `Controller.cpp` retains `applyProductGameplayActions(...)` and only calls
  `applyProductGameplayActionPhases(...)`.

Run a dependency grep over `ControllerActionPhases.*`:

```sh
rg -n "ActionState|sampleProductGameplayInputIntent|submitProductGameplayCommand|tickProductGameplayCommand|applyProductLedgeFallMoveFallback|queryProductGameplayTarget|CommandRecord" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerActionPhases.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/gameplay/ControllerActionPhases.cpp
```

Expected:

- No hits. The action-phase helper should not sample input, own command
  execution internals, or re-own target query internals.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_gameplay_controller_tests product_active_room_collision_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_active_room_collision_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched source files and this
card.

## Completion Brief Checklist

Report:

- Files changed.
- Exact API added.
- Which helpers moved and which stayed in `Controller.cpp`.
- Confirmation that phase order and dash early-return behavior are unchanged.
- Required grep classifications.
- Focused build/CTest results.
- Receipt golden diff result.
- Diff/whitespace check results.
- Confirmation that no staging, commit, push, broad CTest, or window launch was
  performed.
