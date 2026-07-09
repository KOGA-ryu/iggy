# K5b - Controller Micro Owner Fold And Closeout

## Status

READY. K5 batch step 2 of 2. Claim only after K5a is committed and its card is
in `done/`, with the K5a report at 706 source files and 317/16/16, zero SCCs.
Authority:
`docs/creative_mode/builder_tasks/blocked/K5-controller-tail-condensation-plan.md`.

This is the K5 batch closeout. Reviewer receives one aggregate two-commit code
review brief only after this card is committed, unless K5a or K5b STOPs.

## Goal

Fold the two genuine one-concern micro-owners into ActionPhases, delete their
four files, and ratchet the dependency policy to the completed K5 graph.

## Exact File Operations

Update only:

- `CMakeLists.txt`
- `src/app/iggy3d/gameplay/Controller.cpp`
- `src/app/iggy3d/gameplay/ControllerActionPhases.hpp`
- `src/app/iggy3d/gameplay/ControllerActionPhases.cpp`
- `docs/architecture_dependency_policy.json`

Delete:

- `src/app/iggy3d/gameplay/ControllerInputIntent.hpp`
- `src/app/iggy3d/gameplay/ControllerInputIntent.cpp`
- `src/app/iggy3d/gameplay/ControllerResetActions.hpp`
- `src/app/iggy3d/gameplay/ControllerResetActions.cpp`

Do not edit any other Controller pair. In particular,
`ControllerPlayerAccess.hpp/.cpp`, `Controller.hpp`, and every K5a owner are
frozen in this card.

## Required Fold

Move `ProductGameplayInputIntent` and the declarations of
`sampleProductGameplayInputIntent` and `productGameplayIntentHasMovement` into
`ControllerActionPhases.hpp`. Move both definitions into
`ControllerActionPhases.cpp` unchanged. `Controller.cpp` includes only
`ControllerActionPhases.hpp` for this internal contract and keeps its existing
sample-once, dispatch-once body.

Move `submitProductReset` into the anonymous namespace in
`ControllerActionPhases.cpp` because `applyProductResetActionPhase` is its only
caller. Preserve the reset call, target/outcome clearing order, input source,
command facts, and accepted/rejected status expression. Do not expose
`submitProductReset` from the header.

Remove the two obsolete cpp entries from `CMakeLists.txt`. Do not create a
replacement file or compatibility header.

## Final Layout And Policy Ratchet

Exactly 13 Controller cpp and 13 Controller headers remain, matching the parent
plan's final table. Production source count is 702.

The dependency graph remains exactly 317 direct edges, 16 directed pairs, 16
unordered pairs, app fan-out 188, zero SCCs, zero violations. Update only:

- `ratchets.max_direct_edges`: 320 -> 317
- `ratchets.departments.app.max_direct_edge_out`: 191 -> 188

Do not change allowed pairs, restricted targets, other department ceilings, or
`allowed_exceptions`.

## Focused Verification

    cmake -S . -B build
    cmake --build build --target iggy3d_app product_gameplay_controller_tests product_gameplay_controller_kinematics_tests product_controller_action_routing_tests product_active_room_collision_tests
    ctest --test-dir build -R '^(product_gameplay_controller_tests|product_gameplay_controller_kinematics_tests|product_controller_action_routing_tests|product_active_room_collision_tests|dependency_direction_tests)$' --output-on-failure
    python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json
    find src/app/iggy3d/gameplay -maxdepth 1 -name 'Controller*.cpp' | wc -l
    find src/app/iggy3d/gameplay -maxdepth 1 -name 'Controller*.hpp' | wc -l
    rg -n 'Controller(InputIntent|ResetActions)' CMakeLists.txt cmake src tests
    rg -n 'max_direct_edges|max_direct_edge_out|allowed_exceptions' docs/architecture_dependency_policy.json
    git diff --check

The Controller counts must each be 13. The stale-filename grep must return no
match. Report the exact final file list and graph metrics, not only policy pass.

## Stop Conditions

- `Controller.cpp`, phase ordering, reset behavior, or any intent field/value
  must change rather than relocate.
- Any caller of InputIntent or ResetActions exists outside the listed files.
- PlayerAccess, a K5a owner, test source, or non-listed subsystem needs edits.
- The final Controller file counts differ from 13/13.
- The graph differs from 702 / 317 / 188 / 16 / 16 / zero SCC, needs an
  exception, or another policy field must change.
- A focused build or test regresses.

## Completion Brief

Append deleted paths, final 13-pair list, phase/reset preservation evidence,
exact policy diff and live graph, focused tests, and protected-file evidence.
Commit the card, then send Reviewer one aggregate K5 brief listing both commits
and request milestone code review.
