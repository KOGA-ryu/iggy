# K5a - Controller Domain Owner Condensation

## Status

READY. K5 batch step 1 of 2. Claim only after K4 is accepted; accepted baseline
is `ad9caf71`. Authority:
`docs/creative_mode/builder_tasks/blocked/K5-controller-tail-condensation-plan.md`.

On green completion, commit this card, move it to `done/`, and continue to K5b
without waiting for Reviewer. A STOP pauses the whole K5 batch.

## Goal

Replace seven fragmented controller concerns with three right-sized domain
owners while preserving all symbols and gameplay behavior.

## Exact File Operations

Create:

- `src/app/iggy3d/gameplay/ControllerJumpDash.hpp`
- `src/app/iggy3d/gameplay/ControllerJumpDash.cpp`
- `src/app/iggy3d/gameplay/ControllerTargeting.hpp`
- `src/app/iggy3d/gameplay/ControllerTargeting.cpp`
- `src/app/iggy3d/gameplay/ControllerProof.hpp`
- `src/app/iggy3d/gameplay/ControllerProof.cpp`

Delete after their contents and declarations have one replacement owner:

- `src/app/iggy3d/gameplay/ControllerJumpActions.hpp/.cpp`
- `src/app/iggy3d/gameplay/ControllerDashActions.hpp/.cpp`
- `src/app/iggy3d/gameplay/ControllerJumpDashState.hpp/.cpp`
- `src/app/iggy3d/gameplay/ControllerTargetActions.hpp/.cpp`
- `src/app/iggy3d/gameplay/ControllerTargetOutcomeProof.hpp/.cpp`
- `src/app/iggy3d/gameplay/ControllerMovementProof.hpp/.cpp`
- `src/app/iggy3d/gameplay/ControllerTraversalProof.hpp/.cpp`

Update only these existing consumers and registration:

- `CMakeLists.txt`
- `src/app/iggy3d/gameplay/ControllerActionPhases.cpp`
- `src/app/iggy3d/gameplay/ControllerCommandExecution.cpp`
- `src/app/iggy3d/gameplay/ControllerMoveActions.cpp`
- `src/app/iggy3d/gameplay/ControllerResetActions.cpp`
- `src/app/iggy3d/gameplay/ControllerResetFall.cpp`

Do not edit `Controller.hpp/.cpp`, `ControllerActionPhases.hpp`,
`ControllerPlayerAccess.*`, tests, policy JSON, or any non-gameplay source.

## Required Ownership

`ControllerJumpDash.hpp/.cpp` is the sole owner of every declaration and
definition currently in JumpActions, DashActions, and JumpDashState. Preserve
the existing exported names, including jump timing/state helpers,
`advanceProductJump`, `submitProductJump`, and `submitProductDash`. Merge the
source sections without reordering statements inside a function or rewriting
the jump/dash algorithms.

`ControllerTargeting.hpp/.cpp` is the sole owner of
`ProductInteractionOutcomeSnapshot`, target/outcome name and proof helpers,
target query/reach behavior, and `submitProductTargetCommand`. Keep private
query/status helpers private. Preserve enum switches and fail/default behavior.

`ControllerProof.hpp/.cpp` is the sole owner of movement debug/state/profile
publication and traversal/wall-jump proof publication. Preserve every proof
field assignment and status/reason string.

Update consumers by replacing old concern includes with the one corresponding
new concern include. Do not add forwarding headers or leave duplicate
definitions. In `CMakeLists.txt`, replace the seven old cpp entries with exactly
the three new cpp entries.

## Mechanical Checkpoint

After the merge:

- exactly 15 `Controller*.cpp` and 15 `Controller*.hpp` files exist;
- all three new cpp files are between 150 and 700 lines;
- production source count is 706;
- dependency graph is exactly 317 direct edges, 16 directed pairs, 16 unordered
  pairs, zero SCCs, zero policy violations;
- app direct fan-out is exactly 188;
- `docs/architecture_dependency_policy.json` remains byte-unchanged in K5a.

## Focused Verification

    cmake -S . -B build
    cmake --build build --target iggy3d_app product_gameplay_controller_tests product_gameplay_controller_kinematics_tests product_controller_action_routing_tests product_active_room_collision_tests
    ctest --test-dir build -R '^(product_gameplay_controller_tests|product_gameplay_controller_kinematics_tests|product_controller_action_routing_tests|product_active_room_collision_tests)$' --output-on-failure
    python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json
    find src/app/iggy3d/gameplay -maxdepth 1 -name 'Controller*.cpp' | wc -l
    find src/app/iggy3d/gameplay -maxdepth 1 -name 'Controller*.hpp' | wc -l
    rg -n 'Controller(JumpActions|DashActions|JumpDashState|TargetActions|TargetOutcomeProof|MovementProof|TraversalProof)' CMakeLists.txt cmake src tests
    git diff --check

The two counts must each be 15. The stale-filename grep must return no match.
Report the graph's source-file count, edge/pair counts, app fan-out, SCCs, and
policy result explicitly.

## Stop Conditions

- Any moved function needs a signature, behavior, branch-gate, status/reason
  string, or statement-order change to compile.
- A symbol collision cannot be solved by declaration/include cleanup alone.
- A forwarding header, additional cpp, umbrella internal header, test edit, or
  public `Controller.hpp` edit appears necessary.
- Any new owner is over 700 lines or mixes a fourth concern.
- The graph differs from 706 / 317 / 188 / 16 / 16 / zero SCC, or policy fails.
- A focused build or test regresses.

## Completion Brief

Append created/deleted paths, one-owner evidence for all seven absorbed pairs,
new line counts, exact graph metrics, focused test results, and confirmation
that Controller, PlayerAccess, tests, policy, ASCII, and non-gameplay code were
untouched. On success, commit and continue to K5b without per-slice review.
