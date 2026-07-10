# K11a - Input Action Metadata Condensation

## Status

READY v1.1, NATURAL-FORMATTING RULING. K11 batch step 1 of 1. Claim only after K10 aggregate acceptance at
`c0e623291bb8dea2d73ff72fe4eff43315433819`. Authority:
`docs/creative_mode/builder_tasks/blocked/K11-input-action-metadata-condensation-plan.md`.

On green completion, move this card to `done/`, perform the single final Git
transaction below, and send Reviewer one aggregate K11 brief. A STOP pauses the
batch.

## v1.1 Line-Bound Ruling

The naturally formatted `InputActions.cpp` is accepted at 295 physical lines.
The lower range was an estimate, not a quota. Keep the natural formatting and
remove any blank lines or wrapping added solely to reach 300. The hard size
stop remains the 340-line upper bound; no code/table/branch/public-contract
change is authorized by this ruling.

## Goal

Merge `InputAction`, its descriptor registry, and neutral-input bindings into
`InputActions.hpp/.cpp`, reducing `src/app/input/` from 18 files to 14 without
changing symbols, metadata, binding precedence, or behavior.

## Pre-Edit Gate

```sh
test "$(find src/app/input -maxdepth 1 -type f | wc -l | tr -d ' ')" = 18
cmake -S . -B build
CCACHE_DISABLE=1 cmake --build build --target all
ctest --test-dir build -R '^(product_window_input_frame_tests|room_editor_input_tests|product_controller_action_map_tests|product_controller_action_routing_tests|product_gameplay_controller_tests|product_creative_input_frame_tests|product_creative_navigate_fly_tests|product_automation_command_registry_tests|product_automation_dispatch_tests|product_active_room_collision_tests|product_new_world_menu_action_tests|dependency_direction_tests)$' --output-on-failure
python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json > /tmp/k11_dependency_before.json
rg -n '#include "app/input/(InputAction|InputActionRegistry|InputBindings)\.hpp"' src apps tests
rg -n 'inputActionName|inputActionGroup|inputActionGroupName|inputActionDescriptor|inputActionFeatureName|inputActionOwnerName|inputActionHandledBeforeMenu|inputActionKeepsGameplayActive|defaultInputBindings|actionForInput' src apps tests
rg -n 'BG-1225|BG-1226' src/app/input docs/branch_gate_approvals.tsv
git diff --check
```

The focused baseline must pass 12/12. The survey must show exactly the three
old owners, ten public functions, 18 input files, and no BG-1225/BG-1226.
Otherwise STOP without edits.

## Exact Files

Create by unstaged worktree rename, not `git mv`:

- `src/app/input/InputActions.hpp` from `InputAction.hpp`;
- `src/app/input/InputActions.cpp` from `InputActionRegistry.cpp`.

Delete after absorption:

- `src/app/input/InputAction.cpp`;
- `src/app/input/InputActionRegistry.hpp`;
- `src/app/input/InputBindings.hpp`;
- `src/app/input/InputBindings.cpp`.

Update only:

- `CMakeLists.txt`;
- `docs/branch_gate_approvals.tsv`;
- the include consumers listed below.

Do not modify `ActionState`, `InputDeviceEvent`, `InputRouter`, `KeyboardInput`,
`MouseInput`, or `GamepadInput` bodies. No forwarding files or aliases.

## Final Header

`InputActions.hpp` contains the unchanged action/group enums, descriptor model,
device-kind enum, binding model, and all ten declarations from the parent plan.
Preserve enum value order, member order/defaults, signatures, and the complete
`NeutralInput` contract by including `InputDeviceEvent.hpp`.

## Final Implementation

Keep the renamed registry TU as the base. Preserve its descriptor table and
five projection functions unchanged. Absorb the action name/group functions
and binding table/lookup without reordering any function body or binding row.

Add exactly:

- `// branch-gate: BG-1225` immediately before the moved group-name switch;
- `// branch-gate: BG-1226` immediately before the moved first-match `if`.

Add ledger rows at `src/app/input/InputActions.cpp` using these descriptions:

- `relocated input action group name mapping policy`;
- `relocated neutral input binding first match policy`.

No other branch comment, condition, or table rewrite.

## Exact Consumer Surface

Only old-header to `InputActions.hpp` include replacement is allowed in:

- `src/app/input/ActionState.hpp`;
- `src/app/input/GamepadInput.hpp/.cpp`;
- `src/app/input/KeyboardInput.hpp/.cpp`;
- `src/app/input/MouseInput.hpp/.cpp`;
- `src/app/input/InputRouter.hpp`;
- `src/app/iggy3d/automation/Automation.hpp`;
- `src/app/iggy3d/automation/AutomationRoomEditing.hpp`;
- `src/app/iggy3d/creative/bridge/InputFrame.hpp`;
- `src/app/iggy3d/input/ControllerActionMap.hpp`;
- `src/app/iggy3d/input/InputDeviceStore.hpp`;
- `src/app/iggy3d/menu/ActionHandlers.hpp`;
- `src/app/iggy3d/menu/InputRouter.hpp`;
- `src/app/iggy3d/save/SaveSlotOperations.hpp`;
- `src/app/iggy3d/view/CameraController.cpp`;
- `tests/unit/product_active_room_collision_tests.cpp`;
- `tests/unit/product_creative_navigate_fly_tests.cpp`;
- `tests/unit/product_new_world_menu_action_tests.cpp`;
- `tests/unit/product_window_input_frame_tests.cpp`;
- `tests/unit/product_window_renderer_lifecycle_tests.cpp`;
- `tests/unit/room_editor_input_tests.cpp`.

No consumer body, assertion, comment, or additional include change.

## CMake And Mechanical Checkpoint

Replace the three old source entries with exactly one
`src/app/input/InputActions.cpp` entry. Then prove:

- input folder 18 -> 14;
- `InputActions.hpp` 100-130 lines and `InputActions.cpp` 280-340;
- five public types and ten public functions each have one owner;
- descriptor table size/rows and default binding rows/order are unchanged;
- old basenames `InputAction.cpp`, `InputAction.hpp`, `InputActionRegistry`, and
  `InputBindings` have zero live source/CMake/test references;
- `git diff HEAD --summary --find-renames=40%` recognizes both worktree renames;
- dependency deltas are source `-4`, direct/app fan-out `0/0`, pair deltas
  `0/0`, with zero SCCs/violations;
- no retained input body, test body, golden, save/hash/replay, policy, or
  unrelated file differs.

## Post-Edit Verification

```sh
cmake -S . -B build
CCACHE_DISABLE=1 cmake --build build --target all
ctest --test-dir build -R '^(product_window_input_frame_tests|room_editor_input_tests|product_controller_action_map_tests|product_controller_action_routing_tests|product_gameplay_controller_tests|product_creative_input_frame_tests|product_creative_navigate_fly_tests|product_automation_command_registry_tests|product_automation_dispatch_tests|product_active_room_collision_tests|product_new_world_menu_action_tests|dependency_direction_tests)$' --output-on-failure
python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json > /tmp/k11_dependency_after.json
python3 - <<'PY'
import json

with open('/tmp/k11_dependency_before.json') as stream:
  before = json.load(stream)
with open('/tmp/k11_dependency_after.json') as stream:
  after = json.load(stream)

checks = {
    'source_file_delta': (after['scan']['source_file_count'] - before['scan']['source_file_count'], -4),
    'direct_edge_delta': (after['scan']['direct_edge_count'] - before['scan']['direct_edge_count'], 0),
    'directed_pair_delta': (after['scan']['directed_pair_count'] - before['scan']['directed_pair_count'], 0),
    'unordered_pair_delta': (after['scan']['unordered_pair_count'] - before['scan']['unordered_pair_count'], 0),
    'app_fan_out_delta': (after['fan_out']['app']['edge_count'] - before['fan_out']['app']['edge_count'], 0),
}
for name, (actual, expected) in checks.items():
  if actual != expected:
    raise SystemExit(f'{name}: expected {expected}, got {actual}')
if after['strongly_connected_components']:
  raise SystemExit('K11 introduced an SCC')
if not after['policy']['passed'] or after['policy']['violations']:
  raise SystemExit('K11 violated dependency policy')
print(checks)
PY
test "$(find src/app/input -maxdepth 1 -type f | wc -l | tr -d ' ')" = 14
wc -l src/app/input/InputActions.hpp src/app/input/InputActions.cpp
rg -n 'InputAction\.(hpp|cpp)|InputActionRegistry|InputBindings' src apps tests CMakeLists.txt cmake
rg -n 'BG-1225|BG-1226' src/app/input/InputActions.cpp docs/branch_gate_approvals.tsv
git diff HEAD --summary --find-renames=40%
git diff HEAD -- tests/golden src/runtime/save src/runtime/replay docs/architecture_dependency_policy.json
python3 tools/check_branch_gate.py
git diff --check
```

The stale-basename and protected-surface commands return no output. Branch gate
output is green with exactly BG-1225/BG-1226. Do not run broad CTest or launch
an app/window.

## Single Git Transaction

After every unstaged gate passes, move this card to `done/` using a worktree
operation. Then make exactly one permission-bearing command request containing:

```sh
git add -A -- CMakeLists.txt docs/branch_gate_approvals.tsv docs/creative_mode/builder_tasks/done/K11a-input-action-metadata-condensation.md src/app/input src/app/iggy3d/automation/Automation.hpp src/app/iggy3d/automation/AutomationRoomEditing.hpp src/app/iggy3d/creative/bridge/InputFrame.hpp src/app/iggy3d/input/ControllerActionMap.hpp src/app/iggy3d/input/InputDeviceStore.hpp src/app/iggy3d/menu/ActionHandlers.hpp src/app/iggy3d/menu/InputRouter.hpp src/app/iggy3d/save/SaveSlotOperations.hpp src/app/iggy3d/view/CameraController.cpp tests/unit/product_active_room_collision_tests.cpp tests/unit/product_creative_navigate_fly_tests.cpp tests/unit/product_new_world_menu_action_tests.cpp tests/unit/product_window_input_frame_tests.cpp tests/unit/product_window_renderer_lifecycle_tests.cpp tests/unit/room_editor_input_tests.cpp
python3 tools/check_branch_gate.py --cached
git diff --cached --check
git commit -m "codex: condense input action metadata (K11)"
```

Run all four lines in one approved shell transaction. If any line fails, STOP
without an automatic retry and report exact staged/unstaged state.

After commit, run read-only checks:

```sh
python3 tools/check_branch_gate.py --diff HEAD^..HEAD
git diff --check HEAD^ HEAD
```

## Stop Conditions

- Any pre-edit premise or 12-test baseline fails.
- A retained input pair, consumer/test body, symbol, enum/model/table/binding
  order or value, lookup behavior, receipt, golden, or persistence must change.
- More than BG-1225/BG-1226 comments are needed.
- A final file exceeds its bound, old basename remains, rename detection fails,
  or exact folder/dependency deltas differ.
- Post-edit all-target, 12/12 CTest, branch gate, policy, protection, or
  whitespace checks fail.
- The final Git transaction needs a second approval/retry.

## Completion Brief

Append the old->new map, final type/function ownership, descriptor and binding
order proof, duplicate-input first-match proof, BG-1225/BG-1226 handling,
pre/post all-target and 12/12 CTest results, 18->14 folder/line metrics, exact
dependency deltas, rename summary, protected diffs, and single Git-transaction
result. Flag post-acceptance file-spec freshness for Planner; do not edit those
docs.

Commit, then request aggregate K11 review. There is no next child.
