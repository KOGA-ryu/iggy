# K14a - Product State Leaf Condensation

## Status

READY v1.1. Sole K14 batch step. Claim only at accepted K13 HEAD
`5d2a9d01a810cd26561480a1a9310ce51c871a57`. Authority:
`docs/creative_mode/builder_tasks/blocked/K14-product-state-leaf-condensation-plan.md`.

v1.1 accepts Builder's post-edit scope STOP as a plan-provenance correction.
The accepted pre-edit worktree already contained unrelated generated
`tools/__pycache__/check_branch_gate.cpython-314.pyc`; v1.0 incorrectly counted
all untracked `tools/` paths as K14-created. The corrected gate continues to
protect tracked tool changes but excludes unrelated untracked tool caches from
task-created untracked cardinality. Resume the existing unstaged K14 header
implementation. Do not delete the cache artifact, revert/restart the header
moves, rerun the clean pre-edit gate, stage, or commit before final gates pass.

On green completion, move this card to `done/`, perform the single final Git
transaction below, and send Reviewer one aggregate K14 brief. A STOP pauses
the batch. Do not split the result into smaller commits.

## Goal

Delete twenty-three one-consumer product-state headers by moving each complete
record into one app-side state cluster or the existing store/control surface
that owns its lifetime. Preserve every moved struct, every composition root,
and all behavior exactly.

Metric promise:

- app `.hpp/.cpp` files 369 -> 347;
- dependency source files 678 -> 656;
- app physical lines at most 68,450;
- direct-edge/app-fan-out/directed-pair/unordered-pair deltas all zero;
- zero SCCs and policy violations;
- no `.cpp`, test, CMake, persistence, receipt, golden, or branch-policy
  change; and
- one implementation commit.

All line limits are upper bounds. Never add spacing, comments, wrapping, or
neutral churn to reach an estimate.

## Pre-Edit Baseline

Run this before editing:

```sh
test "$(git rev-parse HEAD)" = 5d2a9d01a810cd26561480a1a9310ce51c871a57
test -z "$(git status --short --untracked-files=no -- src apps tests tools CMakeLists.txt cmake docs/branch_gate_approvals.tsv docs/architecture_dependency_policy.json)"
test "$(find src/app/iggy3d -type f \( -name '*.hpp' -o -name '*.cpp' \) | wc -l | tr -d ' ')" = 369
test "$(find src/app/iggy3d -type f \( -name '*.hpp' -o -name '*.cpp' \) -print0 | xargs -0 cat | wc -l | tr -d ' ')" = 68554
test "$(wc -l < src/app/iggy3d/creative/CreativeAuthoringStore.hpp | tr -d ' ')" = 162
test "$(wc -l < src/app/iggy3d/gameplay/GameplayStore.hpp | tr -d ' ')" = 58
test "$(wc -l < src/app/iggy3d/save/SaveSessionStore.hpp | tr -d ' ')" = 49
test "$(wc -l < src/app/iggy3d/input/InputDeviceStore.hpp | tr -d ' ')" = 27
test "$(wc -l < src/app/iggy3d/window/FrontendWindowShell.hpp | tr -d ' ')" = 48
test "$(wc -l < src/app/iggy3d/automation/AutomationControl.hpp | tr -d ' ')" = 27
test "$(wc -l < src/app/iggy3d/ProductAppWindowState.hpp | tr -d ' ')" = 52
python3 - <<'PY'
from pathlib import Path

root = Path('.')
owners = {
  'src/app/iggy3d/ProductCreativeDocumentRevisionState.hpp':
      'src/app/iggy3d/creative/CreativeAuthoringStore.hpp',
  'src/app/iggy3d/ProductCreativeUiInputState.hpp':
      'src/app/iggy3d/creative/CreativeAuthoringStore.hpp',
  'src/app/iggy3d/ProductCreativeUiLastState.hpp':
      'src/app/iggy3d/creative/CreativeAuthoringStore.hpp',
  'src/app/iggy3d/ProductCreativeUiProjectionState.hpp':
      'src/app/iggy3d/creative/CreativeAuthoringStore.hpp',
  'src/app/iggy3d/ProductCreativeUndoState.hpp':
      'src/app/iggy3d/creative/CreativeAuthoringStore.hpp',
  'src/app/iggy3d/ProductStartupState.hpp':
      'src/app/iggy3d/window/FrontendWindowShell.hpp',
  'src/app/iggy3d/ascii_room/AsciiRoomActivationState.hpp':
      'src/app/iggy3d/creative/CreativeAuthoringStore.hpp',
  'src/app/iggy3d/ascii_room/AsciiRoomDraftState.hpp':
      'src/app/iggy3d/creative/CreativeAuthoringStore.hpp',
  'src/app/iggy3d/ascii_room/AsciiRoomPreviewState.hpp':
      'src/app/iggy3d/creative/CreativeAuthoringStore.hpp',
  'src/app/iggy3d/automation/AutomationControlState.hpp':
      'src/app/iggy3d/ProductAppWindowState.hpp',
  'src/app/iggy3d/gameplay/OutcomeState.hpp':
      'src/app/iggy3d/gameplay/GameplayStore.hpp',
  'src/app/iggy3d/gameplay/TapeState.hpp':
      'src/app/iggy3d/gameplay/GameplayStore.hpp',
  'src/app/iggy3d/gameplay/TargetState.hpp':
      'src/app/iggy3d/gameplay/GameplayStore.hpp',
  'src/app/iggy3d/input/ControllerActionState.hpp':
      'src/app/iggy3d/input/InputDeviceStore.hpp',
  'src/app/iggy3d/input/ControllerModeToggleState.hpp':
      'src/app/iggy3d/input/InputDeviceStore.hpp',
  'src/app/iggy3d/menu/ProductTransitionState.hpp':
      'src/app/iggy3d/gameplay/GameplayStore.hpp',
  'src/app/iggy3d/room_editor/RoomEditorOverlayState.hpp':
      'src/app/iggy3d/creative/CreativeAuthoringStore.hpp',
  'src/app/iggy3d/room_editor/RoomEditorPreviewState.hpp':
      'src/app/iggy3d/creative/CreativeAuthoringStore.hpp',
  'src/app/iggy3d/save/SaveDeleteState.hpp':
      'src/app/iggy3d/save/SaveSessionStore.hpp',
  'src/app/iggy3d/save/SaveFlowState.hpp':
      'src/app/iggy3d/save/SaveSessionStore.hpp',
  'src/app/iggy3d/save/SaveRecoverState.hpp':
      'src/app/iggy3d/save/SaveSessionStore.hpp',
  'src/app/iggy3d/save/SelectedProductSaveState.hpp':
      'src/app/iggy3d/save/SaveSessionStore.hpp',
  'src/app/iggy3d/window/MouseCaptureState.hpp':
      'src/app/iggy3d/input/InputDeviceStore.hpp',
}

source_files = []
for base in ('src', 'apps', 'tests'):
  source_files.extend(
      p for p in (root / base).rglob('*')
      if p.is_file() and p.suffix in {'.h', '.hh', '.hpp', '.cpp', '.cc', '.cxx'})

total_lines = 0
for old_path, expected_owner in owners.items():
  old = root / old_path
  if not old.is_file():
    raise SystemExit(f'missing candidate: {old_path}')
  total_lines += len(old.read_text().splitlines())
  include_path = old_path.removeprefix('src/')
  include = f'#include "{include_path}"'
  actual = [str(path) for path in source_files if include in path.read_text(errors='ignore').splitlines()]
  if actual != [expected_owner]:
    raise SystemExit(f'{old_path} consumers: expected {[expected_owner]}, got {actual}')

if total_lines != 562:
  raise SystemExit(f'candidate lines: expected 562, got {total_lines}')
print({'candidate_count': len(owners), 'candidate_lines': total_lines})
PY
cmake -S . -B build
CCACHE_DISABLE=1 cmake --build build --target all
ctest --test-dir build -R '^(branch_gate_tool_tests|dependency_direction_tests|product_god_struct_ownership_coverage_tests|product_receipt_key_order_tests|product_window_renderer_lifecycle_tests|product_window_input_frame_tests|product_mouse_capture_policy_tests|product_controller_action_routing_tests|product_interaction_mode_state_tests|product_automation_dispatch_tests|product_creative_ui_projection_receipt_tests|product_creative_ui_input_frame_tests|product_creative_ui_command_receipt_tests|product_ascii_room_activation_tests|product_room_editor_overlay_tests|product_room_editor_preview_tests|product_gameplay_tape_runner_tests|product_menu_transitions_tests|product_save_delete_executor_tests|product_save_bridge_tests)$' --output-on-failure
python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json > /tmp/k14_dependency_before.json
python3 tools/check_branch_gate.py
git diff --check
```

The focused set must pass 20/20. The dependency JSON must report
678/314/16/16, app fan-out 185, and zero SCCs/violations. Existing compiler
warnings are baseline evidence, not permission to clean them up.

Any premise mismatch is a STOP before edits.

## v1.1 Resume Gate

The full pre-edit baseline above already passed at accepted HEAD before the
valid STOP. Do not rerun checks that require the 23 old headers to exist or the
app tree to have its pre-edit counts. Run this gate against the preserved
partial worktree instead:

```sh
test "$(git rev-parse HEAD)" = 5d2a9d01a810cd26561480a1a9310ce51c871a57
test -z "$(git diff --cached --name-only)"
test -f docs/creative_mode/builder_tasks/ready/K14a-product-state-leaf-condensation.md
test -f src/app/iggy3d/ProductCreativeAuthoringState.hpp
python3 - <<'PY'
import subprocess

expected_tracked = {
  'src/app/iggy3d/ProductAppWindowState.hpp',
  'src/app/iggy3d/ProductCreativeDocumentRevisionState.hpp',
  'src/app/iggy3d/ProductCreativeUiInputState.hpp',
  'src/app/iggy3d/ProductCreativeUiLastState.hpp',
  'src/app/iggy3d/ProductCreativeUiProjectionState.hpp',
  'src/app/iggy3d/ProductCreativeUndoState.hpp',
  'src/app/iggy3d/ProductStartupState.hpp',
  'src/app/iggy3d/ascii_room/AsciiRoomActivationState.hpp',
  'src/app/iggy3d/ascii_room/AsciiRoomDraftState.hpp',
  'src/app/iggy3d/ascii_room/AsciiRoomPreviewState.hpp',
  'src/app/iggy3d/automation/AutomationControl.hpp',
  'src/app/iggy3d/automation/AutomationControlState.hpp',
  'src/app/iggy3d/creative/CreativeAuthoringStore.hpp',
  'src/app/iggy3d/gameplay/GameplayStore.hpp',
  'src/app/iggy3d/gameplay/OutcomeState.hpp',
  'src/app/iggy3d/gameplay/TapeState.hpp',
  'src/app/iggy3d/gameplay/TargetState.hpp',
  'src/app/iggy3d/input/ControllerActionState.hpp',
  'src/app/iggy3d/input/ControllerModeToggleState.hpp',
  'src/app/iggy3d/input/InputDeviceStore.hpp',
  'src/app/iggy3d/menu/ProductTransitionState.hpp',
  'src/app/iggy3d/room_editor/RoomEditorOverlayState.hpp',
  'src/app/iggy3d/room_editor/RoomEditorPreviewState.hpp',
  'src/app/iggy3d/save/SaveDeleteState.hpp',
  'src/app/iggy3d/save/SaveFlowState.hpp',
  'src/app/iggy3d/save/SaveRecoverState.hpp',
  'src/app/iggy3d/save/SaveSessionStore.hpp',
  'src/app/iggy3d/save/SelectedProductSaveState.hpp',
  'src/app/iggy3d/window/FrontendWindowShell.hpp',
  'src/app/iggy3d/window/MouseCaptureState.hpp',
}
expected_task_untracked = {
  'src/app/iggy3d/ProductCreativeAuthoringState.hpp',
}

# Tracked tools remain protected. Untracked tool caches are not K14 provenance.
tracked = set(subprocess.run(
    ['git', 'diff', '--name-only', 'HEAD', '--', 'src', 'apps', 'tests',
     'CMakeLists.txt', 'cmake', 'tools', 'docs/branch_gate_approvals.tsv',
     'docs/architecture_dependency_policy.json'],
    check=True, capture_output=True, text=True).stdout.splitlines())
task_untracked = set(subprocess.run(
    ['git', 'ls-files', '--others', '--exclude-standard', '--', 'src', 'apps',
     'tests', 'CMakeLists.txt', 'cmake', 'docs/branch_gate_approvals.tsv',
     'docs/architecture_dependency_policy.json'],
    check=True, capture_output=True, text=True).stdout.splitlines())
if tracked != expected_tracked or task_untracked != expected_task_untracked:
  raise SystemExit(
      f'K14 resume scope differs: tracked={sorted(tracked)}, '
      f'task_untracked={sorted(task_untracked)}')
print({
  'tracked': len(tracked),
  'task_untracked': sorted(task_untracked),
  'ignored_generated_tool_artifacts': subprocess.run(
      ['git', 'ls-files', '--others', '--exclude-standard', '--',
       'tools/__pycache__'], check=True, capture_output=True,
      text=True).stdout.splitlines(),
})
PY
git diff --check
```

This gate must report 30 exact tracked K14 paths, only the new cluster in the
task-created untracked scope, and zero staged files. The generated tool-cache
line is provenance evidence only; it may be present or absent and is neither a
K14 input nor output. Any other tracked or task-scoped untracked path is a
STOP.

After this gate, continue at **Step 4 - Closure And Structural Gates**. Steps
1-3 are preserved implementation, not instructions to repeat.

## Exact Files

Create:

- `src/app/iggy3d/ProductCreativeAuthoringState.hpp`.

Delete:

- `src/app/iggy3d/ProductCreativeDocumentRevisionState.hpp`;
- `src/app/iggy3d/ProductCreativeUiInputState.hpp`;
- `src/app/iggy3d/ProductCreativeUiLastState.hpp`;
- `src/app/iggy3d/ProductCreativeUiProjectionState.hpp`;
- `src/app/iggy3d/ProductCreativeUndoState.hpp`;
- `src/app/iggy3d/ProductStartupState.hpp`;
- `src/app/iggy3d/ascii_room/AsciiRoomActivationState.hpp`;
- `src/app/iggy3d/ascii_room/AsciiRoomDraftState.hpp`;
- `src/app/iggy3d/ascii_room/AsciiRoomPreviewState.hpp`;
- `src/app/iggy3d/automation/AutomationControlState.hpp`;
- `src/app/iggy3d/gameplay/OutcomeState.hpp`;
- `src/app/iggy3d/gameplay/TapeState.hpp`;
- `src/app/iggy3d/gameplay/TargetState.hpp`;
- `src/app/iggy3d/input/ControllerActionState.hpp`;
- `src/app/iggy3d/input/ControllerModeToggleState.hpp`;
- `src/app/iggy3d/menu/ProductTransitionState.hpp`;
- `src/app/iggy3d/room_editor/RoomEditorOverlayState.hpp`;
- `src/app/iggy3d/room_editor/RoomEditorPreviewState.hpp`;
- `src/app/iggy3d/save/SaveDeleteState.hpp`;
- `src/app/iggy3d/save/SaveFlowState.hpp`;
- `src/app/iggy3d/save/SaveRecoverState.hpp`;
- `src/app/iggy3d/save/SelectedProductSaveState.hpp`; and
- `src/app/iggy3d/window/MouseCaptureState.hpp`.

Update only:

- `src/app/iggy3d/creative/CreativeAuthoringStore.hpp`;
- `src/app/iggy3d/gameplay/GameplayStore.hpp`;
- `src/app/iggy3d/save/SaveSessionStore.hpp`;
- `src/app/iggy3d/input/InputDeviceStore.hpp`;
- `src/app/iggy3d/window/FrontendWindowShell.hpp`;
- `src/app/iggy3d/automation/AutomationControl.hpp`;
- `src/app/iggy3d/ProductAppWindowState.hpp`; and
- this card when moved to `done/`.

No `.cpp`, test, CMake, branch ledger, architecture policy, forwarding header,
alias, planner/spec/optimization document, or unrelated file may change.

## Step 1 - Create The App-Side Creative State Cluster

Create `src/app/iggy3d/ProductCreativeAuthoringState.hpp` with `#pragma once`,
direct `<cstdint>` and `<string>` includes, and `namespace iggy3d`.

Move each complete ownership comment and struct definition source-equivalently
in this exact order:

1. `ProductAsciiRoomDraftState` from `ascii_room/AsciiRoomDraftState.hpp`;
2. `ProductAsciiRoomPreviewState` from `ascii_room/AsciiRoomPreviewState.hpp`;
3. `ProductAsciiRoomActivationState` from
   `ascii_room/AsciiRoomActivationState.hpp`;
4. `ProductRoomEditorOverlayState` from
   `room_editor/RoomEditorOverlayState.hpp`;
5. `ProductRoomEditorPreviewState` from
   `room_editor/RoomEditorPreviewState.hpp`;
6. `ProductCreativeDocumentRevisionState` from its app-root header;
7. `ProductCreativeUndoState` from its app-root header;
8. `ProductCreativeUiProjectionState` from its app-root header;
9. `ProductCreativeUiInputState` from its app-root header; and
10. `ProductCreativeUiLastState` from its app-root header.

Do not rewrite comments, fields, wrapping, or defaults. This file is an
app-window state cluster, not a Creative kernel surface. Add no store,
function, alias, behavior, or kernel include.

In `creative/CreativeAuthoringStore.hpp`, replace the ten corresponding old
includes with exactly:

```cpp
#include "app/iggy3d/ProductCreativeAuthoringState.hpp"
```

Leave `ProductWorldSetupState`, `ProductWorldCreationState`, and the complete
`CreativeAuthoringStore` definition byte-equivalent.

Delete all ten absorbed old headers.

## Step 2 - Fold Gameplay, Save, And Input Records

In `gameplay/GameplayStore.hpp`, remove the four old includes and move these
complete comment/definition blocks immediately inside `namespace iggy3d`,
before `GameplayStore`, in member-use order:

1. `ProductGameplayTargetState`;
2. `ProductGameplayOutcomeState`;
3. `ProductGameplayTapeState`; and
4. `ProductTransitionState`.

Do not move transition behavior into this header. The state joins its sole
value owner; existing transition APIs remain where they are. Leave the
`GameplayStore` block byte-equivalent and delete the four old headers.

In `save/SaveSessionStore.hpp`, remove the four old includes and move complete
blocks before `SaveSessionStore` in member-use order:

1. `ProductSelectedProductSaveState`;
2. `ProductSaveFlowState`;
3. `ProductSaveDeleteState`; and
4. `ProductSaveRecoverState`.

Leave the store block byte-equivalent and delete the four old headers.

In `input/InputDeviceStore.hpp`, remove the three old includes and move
complete blocks before `InputDeviceStore` in member-use order:

1. `ProductMouseCaptureState`;
2. `ProductControllerModeToggleState`; and
3. `ProductControllerActionState`.

Do not move policy/routing behavior. Leave the store block byte-equivalent and
delete the three old headers.

## Step 3 - Fold Startup And Automation State

In `window/FrontendWindowShell.hpp`, remove the `ProductStartupState.hpp`
include and move its complete comment/definition block before the existing
`ProductVulkanMenuState`. Leave `ProductVulkanMenuState` and
`FrontendWindowShell` byte-equivalent. Delete the old startup header.

In `automation/AutomationControl.hpp`:

1. add direct `<cstdint>`, `<string>`, and `<string_view>` includes as required;
2. directly include `app/frontend/MenuInput.hpp` for `MenuOwner`;
3. move the complete `ProductAutomationControlState` comment/definition block
   inside `namespace iggy3d`, before `ProductAutomationControlContext`;
4. retain the `ProductAppWindowState` and `FrontendState` forward declarations;
5. leave `ProductAutomationControlContext` and both function declarations
   byte-equivalent; and
6. do not include `ProductAppWindowState.hpp`.

In `ProductAppWindowState.hpp`, replace:

```cpp
#include "app/iggy3d/automation/AutomationControlState.hpp"
```

with:

```cpp
#include "app/iggy3d/automation/AutomationControl.hpp"
```

Leave `ProductAppWindowState` byte-equivalent. Delete
`automation/AutomationControlState.hpp`.

## Step 4 - Closure And Structural Gates

Before building, run this exact scope check:

```sh
python3 - <<'PY'
import subprocess

expected_tracked = {
  'src/app/iggy3d/ProductAppWindowState.hpp',
  'src/app/iggy3d/ProductCreativeDocumentRevisionState.hpp',
  'src/app/iggy3d/ProductCreativeUiInputState.hpp',
  'src/app/iggy3d/ProductCreativeUiLastState.hpp',
  'src/app/iggy3d/ProductCreativeUiProjectionState.hpp',
  'src/app/iggy3d/ProductCreativeUndoState.hpp',
  'src/app/iggy3d/ProductStartupState.hpp',
  'src/app/iggy3d/ascii_room/AsciiRoomActivationState.hpp',
  'src/app/iggy3d/ascii_room/AsciiRoomDraftState.hpp',
  'src/app/iggy3d/ascii_room/AsciiRoomPreviewState.hpp',
  'src/app/iggy3d/automation/AutomationControl.hpp',
  'src/app/iggy3d/automation/AutomationControlState.hpp',
  'src/app/iggy3d/creative/CreativeAuthoringStore.hpp',
  'src/app/iggy3d/gameplay/GameplayStore.hpp',
  'src/app/iggy3d/gameplay/OutcomeState.hpp',
  'src/app/iggy3d/gameplay/TapeState.hpp',
  'src/app/iggy3d/gameplay/TargetState.hpp',
  'src/app/iggy3d/input/ControllerActionState.hpp',
  'src/app/iggy3d/input/ControllerModeToggleState.hpp',
  'src/app/iggy3d/input/InputDeviceStore.hpp',
  'src/app/iggy3d/menu/ProductTransitionState.hpp',
  'src/app/iggy3d/room_editor/RoomEditorOverlayState.hpp',
  'src/app/iggy3d/room_editor/RoomEditorPreviewState.hpp',
  'src/app/iggy3d/save/SaveDeleteState.hpp',
  'src/app/iggy3d/save/SaveFlowState.hpp',
  'src/app/iggy3d/save/SaveRecoverState.hpp',
  'src/app/iggy3d/save/SaveSessionStore.hpp',
  'src/app/iggy3d/save/SelectedProductSaveState.hpp',
  'src/app/iggy3d/window/FrontendWindowShell.hpp',
  'src/app/iggy3d/window/MouseCaptureState.hpp',
}
expected_task_untracked = {'src/app/iggy3d/ProductCreativeAuthoringState.hpp'}

# Tracked tools remain protected. Untracked tool caches are not K14 provenance.
tracked = set(subprocess.run(
    ['git', 'diff', '--name-only', 'HEAD', '--', 'src', 'apps', 'tests',
     'CMakeLists.txt', 'cmake', 'tools', 'docs/branch_gate_approvals.tsv',
     'docs/architecture_dependency_policy.json'],
    check=True, capture_output=True, text=True).stdout.splitlines())
task_untracked = set(subprocess.run(
    ['git', 'ls-files', '--others', '--exclude-standard', '--', 'src', 'apps',
     'tests', 'CMakeLists.txt', 'cmake',
     'docs/branch_gate_approvals.tsv', 'docs/architecture_dependency_policy.json'],
    check=True, capture_output=True, text=True).stdout.splitlines())
if tracked != expected_tracked or task_untracked != expected_task_untracked:
  raise SystemExit(
      f'K14 scope differs: tracked={sorted(tracked)}, '
      f'task_untracked={sorted(task_untracked)}')
print({'tracked': len(tracked), 'task_untracked': sorted(task_untracked)})
PY
```

Then run exact parity and one-owner verification:

```sh
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

moved = {
  'ProductCreativeDocumentRevisionState': (
      'src/app/iggy3d/ProductCreativeDocumentRevisionState.hpp',
      'src/app/iggy3d/ProductCreativeAuthoringState.hpp'),
  'ProductCreativeUiInputState': (
      'src/app/iggy3d/ProductCreativeUiInputState.hpp',
      'src/app/iggy3d/ProductCreativeAuthoringState.hpp'),
  'ProductCreativeUiLastState': (
      'src/app/iggy3d/ProductCreativeUiLastState.hpp',
      'src/app/iggy3d/ProductCreativeAuthoringState.hpp'),
  'ProductCreativeUiProjectionState': (
      'src/app/iggy3d/ProductCreativeUiProjectionState.hpp',
      'src/app/iggy3d/ProductCreativeAuthoringState.hpp'),
  'ProductCreativeUndoState': (
      'src/app/iggy3d/ProductCreativeUndoState.hpp',
      'src/app/iggy3d/ProductCreativeAuthoringState.hpp'),
  'ProductStartupState': (
      'src/app/iggy3d/ProductStartupState.hpp',
      'src/app/iggy3d/window/FrontendWindowShell.hpp'),
  'ProductAsciiRoomActivationState': (
      'src/app/iggy3d/ascii_room/AsciiRoomActivationState.hpp',
      'src/app/iggy3d/ProductCreativeAuthoringState.hpp'),
  'ProductAsciiRoomDraftState': (
      'src/app/iggy3d/ascii_room/AsciiRoomDraftState.hpp',
      'src/app/iggy3d/ProductCreativeAuthoringState.hpp'),
  'ProductAsciiRoomPreviewState': (
      'src/app/iggy3d/ascii_room/AsciiRoomPreviewState.hpp',
      'src/app/iggy3d/ProductCreativeAuthoringState.hpp'),
  'ProductAutomationControlState': (
      'src/app/iggy3d/automation/AutomationControlState.hpp',
      'src/app/iggy3d/automation/AutomationControl.hpp'),
  'ProductGameplayOutcomeState': (
      'src/app/iggy3d/gameplay/OutcomeState.hpp',
      'src/app/iggy3d/gameplay/GameplayStore.hpp'),
  'ProductGameplayTapeState': (
      'src/app/iggy3d/gameplay/TapeState.hpp',
      'src/app/iggy3d/gameplay/GameplayStore.hpp'),
  'ProductGameplayTargetState': (
      'src/app/iggy3d/gameplay/TargetState.hpp',
      'src/app/iggy3d/gameplay/GameplayStore.hpp'),
  'ProductControllerActionState': (
      'src/app/iggy3d/input/ControllerActionState.hpp',
      'src/app/iggy3d/input/InputDeviceStore.hpp'),
  'ProductControllerModeToggleState': (
      'src/app/iggy3d/input/ControllerModeToggleState.hpp',
      'src/app/iggy3d/input/InputDeviceStore.hpp'),
  'ProductTransitionState': (
      'src/app/iggy3d/menu/ProductTransitionState.hpp',
      'src/app/iggy3d/gameplay/GameplayStore.hpp'),
  'ProductRoomEditorOverlayState': (
      'src/app/iggy3d/room_editor/RoomEditorOverlayState.hpp',
      'src/app/iggy3d/ProductCreativeAuthoringState.hpp'),
  'ProductRoomEditorPreviewState': (
      'src/app/iggy3d/room_editor/RoomEditorPreviewState.hpp',
      'src/app/iggy3d/ProductCreativeAuthoringState.hpp'),
  'ProductSaveDeleteState': (
      'src/app/iggy3d/save/SaveDeleteState.hpp',
      'src/app/iggy3d/save/SaveSessionStore.hpp'),
  'ProductSaveFlowState': (
      'src/app/iggy3d/save/SaveFlowState.hpp',
      'src/app/iggy3d/save/SaveSessionStore.hpp'),
  'ProductSaveRecoverState': (
      'src/app/iggy3d/save/SaveRecoverState.hpp',
      'src/app/iggy3d/save/SaveSessionStore.hpp'),
  'ProductSelectedProductSaveState': (
      'src/app/iggy3d/save/SelectedProductSaveState.hpp',
      'src/app/iggy3d/save/SaveSessionStore.hpp'),
  'ProductMouseCaptureState': (
      'src/app/iggy3d/window/MouseCaptureState.hpp',
      'src/app/iggy3d/input/InputDeviceStore.hpp'),
}

preserved = {
  'CreativeAuthoringStore': 'src/app/iggy3d/creative/CreativeAuthoringStore.hpp',
  'GameplayStore': 'src/app/iggy3d/gameplay/GameplayStore.hpp',
  'SaveSessionStore': 'src/app/iggy3d/save/SaveSessionStore.hpp',
  'InputDeviceStore': 'src/app/iggy3d/input/InputDeviceStore.hpp',
  'FrontendWindowShell': 'src/app/iggy3d/window/FrontendWindowShell.hpp',
  'ProductAutomationControlContext': 'src/app/iggy3d/automation/AutomationControl.hpp',
  'ProductAppWindowState': 'src/app/iggy3d/ProductAppWindowState.hpp',
}

def head_text(path):
  return subprocess.run(
      ['git', 'show', f'HEAD:{path}'], check=True, capture_output=True,
      text=True).stdout

def struct_block(text, name):
  match = re.search(
      rf'^struct {re.escape(name)} \{{.*?^\}};', text,
      flags=re.MULTILINE | re.DOTALL)
  if match is None:
    raise SystemExit(f'missing struct block: {name}')
  return match.group(0)

for name, (old_path, new_path) in moved.items():
  before = struct_block(head_text(old_path), name)
  after = struct_block(Path(new_path).read_text(), name)
  if before != after:
    raise SystemExit(f'moved struct changed: {name}')

for name, path in preserved.items():
  before = struct_block(head_text(path), name)
  after = struct_block(Path(path).read_text(), name)
  if before != after:
    raise SystemExit(f'composition block changed: {name}')

source_files = []
for base in ('src', 'apps', 'tests'):
  source_files.extend(
      p for p in Path(base).rglob('*')
      if p.is_file() and p.suffix in {'.h', '.hh', '.hpp', '.cpp', '.cc', '.cxx'})
for name, (_, expected_owner) in moved.items():
  pattern = re.compile(rf'^struct {re.escape(name)} \{{', re.MULTILINE)
  actual = [str(path) for path in source_files if pattern.search(path.read_text(errors='ignore'))]
  if actual != [expected_owner]:
    raise SystemExit(f'{name} definitions: expected {[expected_owner]}, got {actual}')

control = Path('src/app/iggy3d/automation/AutomationControl.hpp').read_text()
for declaration in (
    'void applyProductAutomationControl(ProductAutomationControlContext& context);',
    'bool automationFailurePreservesLoaded(std::string_view status);'):
  if control.count(declaration) != 1 or head_text(
      'src/app/iggy3d/automation/AutomationControl.hpp').count(declaration) != 1:
    raise SystemExit(f'automation declaration changed: {declaration}')

print({'moved_structs': len(moved), 'preserved_blocks': len(preserved)})
PY
```

The script compares each moved struct definition against accepted HEAD and
proves one final owner. Preserve and visually review the contiguous ownership
comments as part of each moved block; do not rewrite them.

Run basename and metric closure:

```sh
python3 - <<'PY'
from pathlib import Path

deleted = [
  'ProductCreativeDocumentRevisionState.hpp',
  'ProductCreativeUiInputState.hpp',
  'ProductCreativeUiLastState.hpp',
  'ProductCreativeUiProjectionState.hpp',
  'ProductCreativeUndoState.hpp',
  'ProductStartupState.hpp',
  'AsciiRoomActivationState.hpp',
  'AsciiRoomDraftState.hpp',
  'AsciiRoomPreviewState.hpp',
  'AutomationControlState.hpp',
  'OutcomeState.hpp',
  'TapeState.hpp',
  'TargetState.hpp',
  'ControllerActionState.hpp',
  'ControllerModeToggleState.hpp',
  'ProductTransitionState.hpp',
  'RoomEditorOverlayState.hpp',
  'RoomEditorPreviewState.hpp',
  'SaveDeleteState.hpp',
  'SaveFlowState.hpp',
  'SaveRecoverState.hpp',
  'SelectedProductSaveState.hpp',
  'MouseCaptureState.hpp',
]

search_paths = [Path('CMakeLists.txt')]
for base in ('src', 'apps', 'tests', 'cmake'):
  search_paths.extend(path for path in Path(base).rglob('*') if path.is_file())
for basename in deleted:
  hits = [str(path) for path in search_paths if basename in path.read_text(errors='ignore')]
  if hits:
    raise SystemExit(f'stale basename {basename}: {hits}')
print({'retired_basenames': len(deleted)})
PY
test "$(find src/app/iggy3d -type f \( -name '*.hpp' -o -name '*.cpp' \) | wc -l | tr -d ' ')" = 347
test "$(find src/app/iggy3d -type f \( -name '*.hpp' -o -name '*.cpp' \) -print0 | xargs -0 cat | wc -l | tr -d ' ')" -le 68450
test "$(wc -l < src/app/iggy3d/ProductCreativeAuthoringState.hpp | tr -d ' ')" -le 230
test "$(wc -l < src/app/iggy3d/creative/CreativeAuthoringStore.hpp | tr -d ' ')" -le 165
test "$(wc -l < src/app/iggy3d/gameplay/GameplayStore.hpp | tr -d ' ')" -le 145
test "$(wc -l < src/app/iggy3d/save/SaveSessionStore.hpp | tr -d ' ')" -le 105
test "$(wc -l < src/app/iggy3d/input/InputDeviceStore.hpp | tr -d ' ')" -le 75
test "$(wc -l < src/app/iggy3d/window/FrontendWindowShell.hpp | tr -d ' ')" -le 95
test "$(wc -l < src/app/iggy3d/automation/AutomationControl.hpp | tr -d ' ')" -le 65
test "$(wc -l < src/app/iggy3d/ProductAppWindowState.hpp | tr -d ' ')" -le 55
```

## Final Verification

Run before staging:

```sh
cmake -S . -B build
CCACHE_DISABLE=1 cmake --build build --target all
ctest --test-dir build -R '^(branch_gate_tool_tests|dependency_direction_tests|product_god_struct_ownership_coverage_tests|product_receipt_key_order_tests|product_window_renderer_lifecycle_tests|product_window_input_frame_tests|product_mouse_capture_policy_tests|product_controller_action_routing_tests|product_interaction_mode_state_tests|product_automation_dispatch_tests|product_creative_ui_projection_receipt_tests|product_creative_ui_input_frame_tests|product_creative_ui_command_receipt_tests|product_ascii_room_activation_tests|product_room_editor_overlay_tests|product_room_editor_preview_tests|product_gameplay_tape_runner_tests|product_menu_transitions_tests|product_save_delete_executor_tests|product_save_bridge_tests)$' --output-on-failure
python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json > /tmp/k14_dependency_after.json
python3 - <<'PY'
import json

before = json.load(open('/tmp/k14_dependency_before.json'))
after = json.load(open('/tmp/k14_dependency_after.json'))
before_scan = before['scan']
after_scan = after['scan']
if before_scan['source_file_count'] != 678 or after_scan['source_file_count'] != 656:
  raise SystemExit(f'source counts: {before_scan["source_file_count"]} -> {after_scan["source_file_count"]}')
for key in ('direct_edge_count', 'directed_pair_count', 'unordered_pair_count'):
  if after_scan[key] != before_scan[key]:
    raise SystemExit(f'{key} changed: {before_scan[key]} -> {after_scan[key]}')
if before['fan_out']['app']['edge_count'] != 185 or after['fan_out']['app']['edge_count'] != 185:
  raise SystemExit('app fan-out changed')
if after['strongly_connected_components'] or not after['policy']['passed'] or after['policy']['violations']:
  raise SystemExit('dependency policy failed')
print({
  'sources': [before_scan['source_file_count'], after_scan['source_file_count']],
  'edges': after_scan['direct_edge_count'],
  'pairs': [after_scan['directed_pair_count'], after_scan['unordered_pair_count']],
  'app_fan_out': after['fan_out']['app']['edge_count'],
})
PY
python3 tools/check_branch_gate.py
git diff --exit-code HEAD -- CMakeLists.txt cmake tests tools/check_branch_gate.py tests/tools/branch_gate_tests.py docs/branch_gate_approvals.tsv docs/architecture_dependency_policy.json src/runtime src/render src/projection src/app/iggy3d/receipt src/app/iggy3d/debug src/runtime/save src/runtime/replay tests/golden docs/file_specs docs/creative_mode/optimization
git diff --check
```

The focused set must pass 20/20. Protected-surface output must be empty. Rerun
the exact scope, parity/ownership, basename, and metric gates after any edit.
Do not run broad CTest or launch an app/window.

## Single Git Transaction

After every unstaged gate passes, move this card to `done/` with an ordinary
worktree move. Then make exactly one permission-bearing command request
containing all four lines:

```sh
git add -A -- docs/creative_mode/builder_tasks/done/K14a-product-state-leaf-condensation.md src/app/iggy3d/ProductAppWindowState.hpp src/app/iggy3d/ProductCreativeAuthoringState.hpp src/app/iggy3d/ProductCreativeDocumentRevisionState.hpp src/app/iggy3d/ProductCreativeUiInputState.hpp src/app/iggy3d/ProductCreativeUiLastState.hpp src/app/iggy3d/ProductCreativeUiProjectionState.hpp src/app/iggy3d/ProductCreativeUndoState.hpp src/app/iggy3d/ProductStartupState.hpp src/app/iggy3d/ascii_room/AsciiRoomActivationState.hpp src/app/iggy3d/ascii_room/AsciiRoomDraftState.hpp src/app/iggy3d/ascii_room/AsciiRoomPreviewState.hpp src/app/iggy3d/automation/AutomationControl.hpp src/app/iggy3d/automation/AutomationControlState.hpp src/app/iggy3d/creative/CreativeAuthoringStore.hpp src/app/iggy3d/gameplay/GameplayStore.hpp src/app/iggy3d/gameplay/OutcomeState.hpp src/app/iggy3d/gameplay/TapeState.hpp src/app/iggy3d/gameplay/TargetState.hpp src/app/iggy3d/input/ControllerActionState.hpp src/app/iggy3d/input/ControllerModeToggleState.hpp src/app/iggy3d/input/InputDeviceStore.hpp src/app/iggy3d/menu/ProductTransitionState.hpp src/app/iggy3d/room_editor/RoomEditorOverlayState.hpp src/app/iggy3d/room_editor/RoomEditorPreviewState.hpp src/app/iggy3d/save/SaveDeleteState.hpp src/app/iggy3d/save/SaveFlowState.hpp src/app/iggy3d/save/SaveRecoverState.hpp src/app/iggy3d/save/SaveSessionStore.hpp src/app/iggy3d/save/SelectedProductSaveState.hpp src/app/iggy3d/window/FrontendWindowShell.hpp src/app/iggy3d/window/MouseCaptureState.hpp
python3 tools/check_branch_gate.py --cached
git diff --cached --check
git commit -m "codex: condense product state records (K14)"
```

Run those lines in one approved shell transaction. If any line fails, STOP
without an automatic retry and report exact staged/unstaged state.

After commit, run read-only checks:

```sh
python3 tools/check_branch_gate.py --diff HEAD^..HEAD
git diff --check HEAD^ HEAD
git show --stat --oneline HEAD
```

## Stop Conditions

- Any accepted baseline or v1.1 resume, file/line/consumer, all-target,
  20-test, policy, branch-gate, or whitespace premise fails.
- A candidate has another include consumer/value owner, a definition/comment
  cannot move source-equivalently, or a final owner differs from the map.
- The work needs a `.cpp`, CMake, test, behavior, alias, forwarding header,
  field/store-member change, public API change, or include cycle.
- A preserved store/root/context block changes, an old basename remains, a
  moved type has zero/multiple definitions, or a hard line/file bound fails.
- The exact changed-file set differs, a protected concern changes, or a
  pre-existing warning would need cleanup.
- Dependency final state differs from 656/314/16/16/app-185 with zero
  SCCs/violations.
- Post-edit all-target, 20/20 CTest, parity, ownership, branch, protected diff,
  or whitespace check fails.
- The final Git transaction needs a second approval/retry.

## Completion Brief

Report:

- commit hash;
- all twenty-three old -> new ownership mappings;
- source-equivalent struct/comment proof and exact one-owner results;
- byte-equivalent CreativeAuthoringStore, GameplayStore, SaveSessionStore,
  InputDeviceStore, FrontendWindowShell, automation context/API, and
  ProductAppWindowState proof;
- new cluster include boundary and final line counts for all eight surfaces;
- final app 347-file / app-LOC facts;
- pre/post all-target and 20/20 focused CTest results;
- exact dependency delta and final 656/314/16/16/app-185 policy state;
- stale-basename, protected-diff, branch, whitespace, and one-Git-transaction
  results; and
- post-acceptance file-spec/Cartographer freshness items for Planner.

Commit, then request aggregate K14 review. There is no next child.
