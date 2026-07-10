# K16a - Room-Editing Automation Dispatch Split

## Status

READY v1.1. K16 batch step 1 of 1. Prerequisite: accepted K15 HEAD is
`fa050916d529134271df6dc446a8dedd0b7f5fdd`, no task is claimed, and this is
the sole ready card.

v1.1 is an execution-only repair. It replaces shell-substitution inventory
checks with direct Python assertions after two pre-edit attempts lost escaping
while nesting the card inside quoted `zsh`/`bash -lc` wrappers. Those attempts
did not reach repository checks and are not STOPs. Run every fenced block
directly as the command body; do not interpolate an entire block into another
quoted shell command. No architecture, owner map, scope, metric, or acceptance
gate changed.

Authority:
`docs/creative_mode/builder_tasks/blocked/K16-room-editing-automation-dispatch-split-plan.md`.

On completion, move this card to `done/`, commit once, and send Reviewer one
aggregate K16 brief. A STOP pauses the batch.

## Goal

Split the 1,113-line room-editing automation implementation into a retained
operation/preview owner and a new command-dispatch owner without changing the
public header or behavior.

Final metric promise:

- `AutomationRoomEditing.hpp` byte-for-byte unchanged;
- exactly 31 definitions in `AutomationRoomEditing.cpp` and 15 in new
  `AutomationRoomEditingDispatch.cpp` from the governing map;
- both implementations at most 560/680 lines respectively;
- automation folder 14 files / 10 implementations / 4 headers / at most 4,710
  physical lines;
- one Session include remains in `AutomationRoomEditing.cpp`, none in the new
  dispatcher;
- dependency policy 656/314/16/16/app-185 with no SCCs/violations;
- no test or consumer changes; and
- one implementation commit.

Do not pad files to a line estimate.

## Prerequisite And Pre-Edit Gate

```sh
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

def run(args, check=True):
  return subprocess.run(args, check=check, capture_output=True, text=True)

def require(condition, message):
  if not condition:
    raise SystemExit(message)

def physical_lines(path):
  return path.read_bytes().count(b'\n')

def definition_line_count(path):
  pattern = re.compile(
      r'^[A-Za-z_][A-Za-z0-9_:<>, &*]* '
      r'[A-Za-z_][A-Za-z0-9_]*\(')
  return sum(pattern.match(line) is not None
             for line in path.read_text().splitlines())

require(run(['git', 'rev-parse', 'HEAD']).stdout.strip() ==
        'fa050916d529134271df6dc446a8dedd0b7f5fdd',
        'accepted K15 HEAD changed')
require(run(['git', 'show', '-s', '--format=%s', 'HEAD']).stdout.strip() ==
        'codex: deduplicate automation lookups (K15b)',
        'accepted K15 subject changed')

claimed = sorted(
    str(path) for path in Path('docs/creative_mode/builder_tasks/claimed').iterdir()
    if path.is_file() and path.name != '.gitkeep')
ready = sorted(
    str(path) for path in Path('docs/creative_mode/builder_tasks/ready').iterdir()
    if path.is_file())
require(not claimed, f'task already claimed: {claimed}')
require(ready == [
    'docs/creative_mode/builder_tasks/ready/'
    'K16a-room-editing-automation-dispatch-split.md'],
    f'ready bucket differs: {ready}')

tracked_status = run([
    'git', 'status', '--short', '--untracked-files=no', '--', 'src', 'apps',
    'tests', 'tools', 'CMakeLists.txt', 'cmake',
    'docs/branch_gate_approvals.tsv',
    'docs/architecture_dependency_policy.json']).stdout.splitlines()
require(not tracked_status,
        f'tracked task surface is dirty: {tracked_status}')

paths = run(
    ['git', 'ls-files', '--others', '--exclude-standard', '--', 'src', 'apps',
     'tests', 'tools', 'CMakeLists.txt', 'cmake',
     'docs/branch_gate_approvals.tsv',
     'docs/architecture_dependency_policy.json']).stdout.splitlines()
unexpected = [path for path in paths
              if not path.startswith('tools/__pycache__/')]
require(not unexpected,
        f'unexpected task-surface untracked files: {unexpected}')

folder = Path('src/app/iggy3d/automation')
files = sorted([*folder.glob('*.hpp'), *folder.glob('*.cpp')])
cpp_files = [path for path in files if path.suffix == '.cpp']
hpp_files = [path for path in files if path.suffix == '.hpp']
source = folder / 'AutomationRoomEditing.cpp'
header = folder / 'AutomationRoomEditing.hpp'
dispatch = folder / 'AutomationRoomEditingDispatch.cpp'
require((len(files), len(cpp_files), len(hpp_files)) == (13, 9, 4),
        'automation folder is not 13/9/4')
require(sum(physical_lines(path) for path in files) == 4680,
        'automation folder is not 4,680 physical lines')
require(physical_lines(source) == 1113,
        'AutomationRoomEditing.cpp is not 1,113 lines')
require(physical_lines(header) == 136,
        'AutomationRoomEditing.hpp is not 136 lines')
require(definition_line_count(source) == 46,
        'AutomationRoomEditing.cpp does not have 46 mapped definitions')
require(not dispatch.exists(), 'dispatch owner already exists')
require(Path('CMakeLists.txt').read_text().count(
    'src/app/iggy3d/automation/AutomationRoomEditing.cpp') == 1,
    'CMake room-editing owner cardinality changed')

wrapper_pattern = (
    'applyProductRoomEditorMoveAutomation|'
    'applyProductRoomEditorToolAutomation|'
    'applyProductRoomEditorCycleToolAutomation|'
    'applyProductRoomEditorWallDirectionAutomation|'
    'applyProductRoomEditorPlaceAutomation|'
    'applyProductEditorInputAutomation')
callers = run([
    'rg', '-l', '--glob',
    '!src/app/iggy3d/automation/AutomationRoomEditing.cpp',
    wrapper_pattern, 'src', 'tests'], check=False)
require(callers.returncode in (0, 1), 'wrapper caller scan failed')
require(not callers.stdout.splitlines(),
        f'dispatch-only wrapper caller appeared: {callers.stdout.splitlines()}')
require(source.read_text().count('runtime/session/Session.hpp') == 1,
        'Session include cardinality changed')
bg_scan = run([
    'rg', '-n', 'BG-1234', 'src', 'tests',
    'docs/branch_gate_approvals.tsv'], check=False)
require(bg_scan.returncode in (0, 1), 'BG-1234 scan failed')
require(not bg_scan.stdout.splitlines(), 'BG-1234 already exists')
print({
    'ignored_generated_cache': sorted(set(paths) - set(unexpected)),
    'automation': [len(files), len(cpp_files), len(hpp_files), 4680],
    'room_editing': [physical_lines(source), definition_line_count(source)],
})
PY
cmake -S . -B build
CCACHE_DISABLE=1 cmake --build build --target all
ctest --test-dir build -R '^(branch_gate_tool_tests|dependency_direction_tests|product_automation_command_registry_tests|product_automation_dispatch_tests|product_window_input_frame_tests|product_interaction_mode_state_tests|product_room_authoring_controller_tests|product_room_editor_action_controller_tests|room_editor_input_tests|product_room_editor_cursor_tests|product_room_editor_preview_tests|product_room_editing_state_tests|product_ascii_room_activation_tests|product_room_editing_automation_smoke|product_automation_menu_smoke|product_editor_floor_save_continue_smoke|product_editor_combined_save_continue_smoke|product_editor_wall_direction_hotkey_smoke)$' --output-on-failure
python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json > /tmp/k16a_dependency_before.json
python3 - <<'PY'
import json

report = json.load(open('/tmp/k16a_dependency_before.json'))
expected = {
    'source_file_count': 655,
    'direct_edge_count': 314,
    'directed_pair_count': 16,
    'unordered_pair_count': 16,
}
for key, value in expected.items():
  if report['scan'][key] != value:
    raise SystemExit(f'{key}: expected {value}, got {report["scan"][key]}')
if report['fan_out']['app']['edge_count'] != 185:
  raise SystemExit('app fan-out is not 185')
if report['strongly_connected_components'] or not report['policy']['passed']:
  raise SystemExit('dependency baseline is not green')
print({'baseline': expected, 'app': 185})
PY
python3 tools/check_branch_gate.py
git diff --check
```

The focused set must pass 18/18. A prerequisite, caller, count, build, test,
branch, or dependency mismatch is a STOP. Do not reinterpret it.

## Exact Files

Update only:

- `CMakeLists.txt`;
- `docs/branch_gate_approvals.tsv`;
- `src/app/iggy3d/automation/AutomationRoomEditing.cpp`;
- create
  `src/app/iggy3d/automation/AutomationRoomEditingDispatch.cpp`; and
- this card when moved to `done/`.

Do not edit `AutomationRoomEditing.hpp`, another automation source, any test,
`cmake/iggy3d_tests.cmake`, consumer, branch tool/oracle, policy, file spec,
runtime/render/projection/save/replay/receipt/golden path, or ASCII behavior.

## Step 1 - Retain The Operation And Preview Owner

Keep `AutomationRoomEditing.hpp` byte-for-byte unchanged.

In `AutomationRoomEditing.cpp`, retain these 31 complete definitions in their
current relative order:

1. `signedRoomEditorPreviewDelta`;
2. `clearProductRoomEditorPreview`;
3. `markProductRoomEditorPreviewCleared`;
4. `recordProductRoomEditorPreviewResult`;
5. `copyRoomEditingStateToWindow`;
6. `recordProductRoomEditingStart`;
7. `recordProductRoomEditingLeave`;
8. `recordProductRoomEditingOperation`;
9. `recordProductRoomEditorCursorResult`;
10. `rejectProductRoomEditorNotReady`;
11. `recordProductRoomEditorActionResult`;
12. `startProductRoomEditAutomationFromAsciiDraft`;
13. `startProductRoomEditAutomationFromActiveRoom`;
14. `applyProductRoomEditAutomation`;
15. `undoProductRoomEditAutomation`;
16. `redoProductRoomEditAutomation`;
17. `applyProductRoomEditorMousePickAutomation`;
18. `buildProductRoomEditorPreviewAutomation`;
19. `confirmProductRoomEditorPreviewAutomation`;
20. `baseProductRoomEditorPreviewInputResult`;
21. `productRoomEditorPreviewMatchesCursor`;
22. `recordProductRoomEditorPreviewInputReceipt`;
23. `rejectProductRoomEditorPreviewInputNotReady`;
24. `applyProductRoomEditorPreviewInputBuild`;
25. `applyProductRoomEditorPreviewInputConfirm`;
26. `applyProductRoomEditorPreviewInputPlace`;
27. `applyProductRoomEditorPreviewInputCancel`;
28. `isProductRoomEditorPreviewInputAction`;
29. `applyProductRoomEditorPreviewInputAction`;
30. `productRoomEditorMousePickAnchor`; and
31. `productRoomEditorMousePickViewportConfig`.

`productRoomEditorMousePickAnchor` and the viewport-config definition may move
upward within this same file to close the namespace after extraction. Preserve
their complete definition text. Keep `runtime/session/Session.hpp` here and
remove includes only when the retained definitions no longer use them.

Do not change any body, signature, linkage, order inside a function, comment,
branch annotation, string, or state write.

## Step 2 - Create The Dispatch Owner

Create `AutomationRoomEditingDispatch.cpp`. It includes
`AutomationRoomEditing.hpp` first and owns exactly these 15 definitions in the
listed concern order.

First move the six undeclared dispatch-only wrappers, preserving definition
text and linkage:

1. `applyProductRoomEditorMoveAutomation`;
2. `applyProductRoomEditorToolAutomation`;
3. `applyProductRoomEditorCycleToolAutomation`;
4. `applyProductRoomEditorWallDirectionAutomation`;
5. `applyProductRoomEditorPlaceAutomation`; and
6. `applyProductEditorInputAutomation`.

Then add this marker outside every function body:

```cpp
// branch-gate-relocation: BG-1234 from=src/app/iggy3d/automation/AutomationRoomEditing.cpp
```

Move the unchanged private `RoomEditorMousePickValue` record and these nine
definitions:

7. `unhandledRoomEditingAutomation`;
8. `failRoomEditingAutomation`;
9. `passRoomEditingAutomation`;
10. `failRoomEditorPreviewAutomation`;
11. `parseRoomEditorMousePickValue`;
12. `roomEditorReady`;
13. `applyRoomEditingOperationResult`;
14. `applyRoomEditorCursorResult`; and
15. `applyProductRoomEditingAutomationCommand`.

Keep private helpers in anonymous namespaces. Use only direct includes needed
by these definitions. The new file must have zero `runtime/` includes; it may
pass `context.activeSession` to the public anchor declaration without needing a
complete Session type.

Do not add an internal header, expose the six wrappers publicly, duplicate a
definition, split the dispatcher into more functions, or rewrite the branch
ladder/table behavior.

## Step 3 - Register Exact Relocation

In `CMakeLists.txt`, add exactly one
`src/app/iggy3d/automation/AutomationRoomEditingDispatch.cpp` entry immediately
after `AutomationRoomEditing.cpp`.

Append one approval row to `docs/branch_gate_approvals.tsv`:

```text
BG-1234	src/app/iggy3d/automation/AutomationRoomEditingDispatch.cpp	exact room-editing automation command-dispatch branch-set relocation from AutomationRoomEditing.cpp	codex	2026-07-09
```

Do not modify the checker, oracle, existing approval rows, test registration,
or public header.

## Definition-Parity And Structure Gate

Run before staging:

```sh
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

source = 'src/app/iggy3d/automation/AutomationRoomEditing.cpp'
dispatch = 'src/app/iggy3d/automation/AutomationRoomEditingDispatch.cpp'
before = subprocess.run(
    ['git', 'show', f'HEAD:{source}'], check=True,
    capture_output=True, text=True).stdout
retained_text = Path(source).read_text()
dispatch_text = Path(dispatch).read_text()

def definition_blocks(text, name):
  blocks = []
  for match in re.finditer(
      rf'(?m)^[A-Za-z_][^\n]*\b{re.escape(name)}\s*\(', text):
    start = match.start()
    brace = text.find('{', match.end())
    if brace < 0:
      continue
    depth = 0
    state = 'code'
    i = brace
    while i < len(text):
      char = text[i]
      nxt = text[i + 1] if i + 1 < len(text) else ''
      if state == 'line':
        if char == '\n': state = 'code'
      elif state == 'block':
        if char == '*' and nxt == '/': state = 'code'; i += 1
      elif state in {'single', 'double'}:
        quote = "'" if state == 'single' else '"'
        if char == '\\': i += 1
        elif char == quote: state = 'code'
      else:
        if char == '/' and nxt == '/': state = 'line'; i += 1
        elif char == '/' and nxt == '*': state = 'block'; i += 1
        elif char == "'": state = 'single'
        elif char == '"': state = 'double'
        elif char == '{': depth += 1
        elif char == '}':
          depth -= 1
          if depth == 0:
            blocks.append(text[start:i + 1])
            break
      i += 1
  return blocks

retained = [
  'signedRoomEditorPreviewDelta',
  'clearProductRoomEditorPreview',
  'markProductRoomEditorPreviewCleared',
  'recordProductRoomEditorPreviewResult',
  'copyRoomEditingStateToWindow',
  'recordProductRoomEditingStart',
  'recordProductRoomEditingLeave',
  'recordProductRoomEditingOperation',
  'recordProductRoomEditorCursorResult',
  'rejectProductRoomEditorNotReady',
  'recordProductRoomEditorActionResult',
  'startProductRoomEditAutomationFromAsciiDraft',
  'startProductRoomEditAutomationFromActiveRoom',
  'applyProductRoomEditAutomation',
  'undoProductRoomEditAutomation',
  'redoProductRoomEditAutomation',
  'applyProductRoomEditorMousePickAutomation',
  'buildProductRoomEditorPreviewAutomation',
  'confirmProductRoomEditorPreviewAutomation',
  'baseProductRoomEditorPreviewInputResult',
  'productRoomEditorPreviewMatchesCursor',
  'recordProductRoomEditorPreviewInputReceipt',
  'rejectProductRoomEditorPreviewInputNotReady',
  'applyProductRoomEditorPreviewInputBuild',
  'applyProductRoomEditorPreviewInputConfirm',
  'applyProductRoomEditorPreviewInputPlace',
  'applyProductRoomEditorPreviewInputCancel',
  'isProductRoomEditorPreviewInputAction',
  'applyProductRoomEditorPreviewInputAction',
  'productRoomEditorMousePickAnchor',
  'productRoomEditorMousePickViewportConfig',
]
moved = [
  'applyProductRoomEditorMoveAutomation',
  'applyProductRoomEditorToolAutomation',
  'applyProductRoomEditorCycleToolAutomation',
  'applyProductRoomEditorWallDirectionAutomation',
  'applyProductRoomEditorPlaceAutomation',
  'applyProductEditorInputAutomation',
  'unhandledRoomEditingAutomation',
  'failRoomEditingAutomation',
  'passRoomEditingAutomation',
  'failRoomEditorPreviewAutomation',
  'parseRoomEditorMousePickValue',
  'roomEditorReady',
  'applyRoomEditingOperationResult',
  'applyRoomEditorCursorResult',
  'applyProductRoomEditingAutomationCommand',
]

for name, expected_owner, other_owner in (
    [(name, retained_text, dispatch_text) for name in retained] +
    [(name, dispatch_text, retained_text) for name in moved]):
  old = definition_blocks(before, name)
  owned = definition_blocks(expected_owner, name)
  other = definition_blocks(other_owner, name)
  if len(old) != 1 or len(owned) != 1 or other:
    raise SystemExit(
        f'{name}: ownership differs old={len(old)} owned={len(owned)} '
        f'other={len(other)}')
  if old[0] != owned[0]:
    raise SystemExit(f'{name}: definition text changed')

struct_pattern = r'(?ms)^struct RoomEditorMousePickValue \{.*?^\};'
old_struct = re.findall(struct_pattern, before)
new_struct = re.findall(struct_pattern, dispatch_text)
if len(old_struct) != 1 or new_struct != old_struct or re.search(
    struct_pattern, retained_text):
  raise SystemExit('RoomEditorMousePickValue ownership/text changed')

if retained_text.count('template <typename T>') != 1:
  raise SystemExit('retained template preamble changed')

def require(condition, message):
  if not condition:
    raise SystemExit(message)

def physical_lines(path):
  return path.read_bytes().count(b'\n')

def definition_line_count(path):
  pattern = re.compile(
      r'^[A-Za-z_][A-Za-z0-9_:<>, &*]* '
      r'[A-Za-z_][A-Za-z0-9_]*\(')
  return sum(pattern.match(line) is not None
             for line in path.read_text().splitlines())

retained_path = Path(source)
dispatch_path = Path(dispatch)
header_path = Path(
    'src/app/iggy3d/automation/AutomationRoomEditing.hpp')
folder = retained_path.parent
files = sorted([*folder.glob('*.hpp'), *folder.glob('*.cpp')])
cpp_files = [path for path in files if path.suffix == '.cpp']
hpp_files = [path for path in files if path.suffix == '.hpp']
ledger = Path('docs/branch_gate_approvals.tsv').read_text()
cmake = Path('CMakeLists.txt').read_text()
require(dispatch_text.count('branch-gate-relocation: BG-1234') == 1,
        'BG-1234 relocation marker cardinality differs')
require(sum(line.startswith('BG-1234\t')
            for line in ledger.splitlines()) == 1,
        'BG-1234 ledger cardinality differs')
require(cmake.count(
    'src/app/iggy3d/automation/AutomationRoomEditingDispatch.cpp') == 1,
    'CMake dispatch owner cardinality differs')
require(retained_text.count('runtime/session/Session.hpp') == 1,
        'retained Session include cardinality differs')
require('#include "runtime/' not in dispatch_text,
        'dispatch owner has a direct runtime include')
require(definition_line_count(retained_path) == 31,
        'retained definition-line count is not 31')
require(definition_line_count(dispatch_path) == 15,
        'dispatch definition-line count is not 15')
require(physical_lines(retained_path) <= 560,
        'retained owner exceeds 560 lines')
require(physical_lines(dispatch_path) <= 680,
        'dispatch owner exceeds 680 lines')
require((len(files), len(cpp_files), len(hpp_files)) == (14, 10, 4),
        'final automation folder is not 14/10/4')
require(sum(physical_lines(path) for path in files) <= 4710,
        'final automation folder exceeds 4,710 lines')
print({'retained_definitions': len(retained),
       'dispatch_definitions': len(moved),
       'definition_text_preserved': len(retained) + len(moved),
       'lines': [physical_lines(retained_path),
                 physical_lines(dispatch_path)],
       'folder': [len(files), len(cpp_files), len(hpp_files),
                  sum(physical_lines(path) for path in files)]})
PY
git diff --exit-code HEAD -- src/app/iggy3d/automation/AutomationRoomEditing.hpp
git diff --check
```

If natural source exceeds a hard bound, STOP. Do not compress unrelated code
or add/remove neutral spacing to manipulate a metric.

## Scope And Final Verification

```sh
python3 - <<'PY'
import subprocess

expected_tracked = {
  'CMakeLists.txt',
  'docs/branch_gate_approvals.tsv',
  'src/app/iggy3d/automation/AutomationRoomEditing.cpp',
}
expected_untracked = {
  'src/app/iggy3d/automation/AutomationRoomEditingDispatch.cpp',
}
tracked = set(subprocess.run(
    ['git', 'diff', '--name-only', 'HEAD', '--', 'src', 'apps', 'tests',
     'CMakeLists.txt', 'cmake', 'tools', 'docs/branch_gate_approvals.tsv',
     'docs/architecture_dependency_policy.json'],
    check=True, capture_output=True, text=True).stdout.splitlines())
untracked = set(subprocess.run(
    ['git', 'ls-files', '--others', '--exclude-standard', '--', 'src', 'apps',
     'tests', 'CMakeLists.txt', 'cmake',
     'docs/branch_gate_approvals.tsv',
     'docs/architecture_dependency_policy.json'],
    check=True, capture_output=True, text=True).stdout.splitlines())
if tracked != expected_tracked or untracked != expected_untracked:
  raise SystemExit(
      f'K16a scope differs: tracked={sorted(tracked)}, '
      f'untracked={sorted(untracked)}')
print({'tracked': sorted(tracked), 'untracked': sorted(untracked)})
PY
cmake -S . -B build
CCACHE_DISABLE=1 cmake --build build --target all
ctest --test-dir build -R '^(branch_gate_tool_tests|dependency_direction_tests|product_automation_command_registry_tests|product_automation_dispatch_tests|product_window_input_frame_tests|product_interaction_mode_state_tests|product_room_authoring_controller_tests|product_room_editor_action_controller_tests|room_editor_input_tests|product_room_editor_cursor_tests|product_room_editor_preview_tests|product_room_editing_state_tests|product_ascii_room_activation_tests|product_room_editing_automation_smoke|product_automation_menu_smoke|product_editor_floor_save_continue_smoke|product_editor_combined_save_continue_smoke|product_editor_wall_direction_hotkey_smoke)$' --output-on-failure
python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json > /tmp/k16a_dependency_after.json
python3 - <<'PY'
import json

before = json.load(open('/tmp/k16a_dependency_before.json'))
after = json.load(open('/tmp/k16a_dependency_after.json'))
expected_before = {
    'source_file_count': 655,
    'direct_edge_count': 314,
    'directed_pair_count': 16,
    'unordered_pair_count': 16,
}
for key, value in expected_before.items():
  if before['scan'][key] != value:
    raise SystemExit(f'baseline {key} drifted')
expected_after = dict(expected_before)
expected_after['source_file_count'] = 656
for key, value in expected_after.items():
  if after['scan'][key] != value:
    raise SystemExit(f'final {key}: expected {value}, got {after["scan"][key]}')
if before['fan_out']['app']['edge_count'] != 185 or after['fan_out']['app']['edge_count'] != 185:
  raise SystemExit('app fan-out changed')
if after['strongly_connected_components'] or not after['policy']['passed']:
  raise SystemExit('dependency policy failed')
print({'sources': 656, 'edges': 314, 'pairs': [16, 16], 'app': 185})
PY
python3 tools/check_branch_gate.py
git diff --exit-code HEAD -- cmake tests tools/check_branch_gate.py tests/tools docs/architecture_dependency_policy.json src/app/iggy3d/automation/AutomationRoomEditing.hpp src/app/iggy3d/automation/Automation.hpp src/app/iggy3d/automation/Automation.cpp src/app/iggy3d/automation/AutomationMetadata.cpp src/app/iggy3d/automation/AutomationWorldSetup.cpp src/app/iggy3d/automation/AutomationControl.hpp src/app/iggy3d/automation/AutomationControl.cpp src/app/iggy3d/automation/AutomationDispatch.hpp src/app/iggy3d/automation/AutomationDispatch.cpp src/app/iggy3d/automation/AutomationGameplay.cpp src/app/iggy3d/automation/AutomationSaveBrowser.cpp src/app/iggy3d/automation/AutomationSystem.cpp src/app/iggy3d/window src/app/iggy3d/room_editor src/app/iggy3d/receipt src/runtime src/projection src/render tests/golden docs/file_specs docs/creative_mode/optimization
git diff --check
```

The focused set must pass 18/18. Rerun the definition, structure, scope,
dependency, branch, and whitespace gates after any correction. No broad CTest
or app/window launch.

## Single Git Transaction

After every unstaged gate passes, move this card to `done/`. Then make one
permission-bearing request containing:

```sh
git add -A -- CMakeLists.txt docs/branch_gate_approvals.tsv docs/creative_mode/builder_tasks/done/K16a-room-editing-automation-dispatch-split.md src/app/iggy3d/automation/AutomationRoomEditing.cpp src/app/iggy3d/automation/AutomationRoomEditingDispatch.cpp
python3 tools/check_branch_gate.py --cached
git diff --cached --check
git commit -m "codex: split room-editing automation dispatch (K16)"
```

Do not include the absent ready-card path. On failure, STOP without an
automatic retry.

After commit:

```sh
python3 tools/check_branch_gate.py --diff HEAD^..HEAD
git diff --check HEAD^ HEAD
git show --stat --oneline HEAD
```

## Stop Conditions

- Accepted HEAD, clean task surfaces, sole-ready-card, folder/line/definition,
  caller, CMake, branch, build, 18-test, or dependency premise fails.
- A mapped function or the mouse-pick record cannot move with exact text.
- A public/internal header, new declaration, consumer, test, or another source
  edit is required.
- The new dispatcher requires a runtime include or duplicates the Session edge.
- Function ownership is not exactly 31/15, a definition is lost/duplicated, or
  either implementation/folder exceeds its hard maximum naturally.
- A command order, branch, parse rule, owner check, string, state write,
  receipt, preview lifecycle, or public behavior must change.
- Dependency delta is not exactly +1 source and zero elsewhere.
- Scope, protected diff, branch gate, all-target build, 18/18 tests, whitespace,
  or Git transaction fails.

## Completion Brief

- Files changed: `CMakeLists.txt`, `docs/branch_gate_approvals.tsv`, retained
  `AutomationRoomEditing.cpp`, new `AutomationRoomEditingDispatch.cpp`, and
  this completed card.
- Behavior changed: none. This is a source-equivalent ownership split. The
  public header, command order, branches, strings, state writes, consumers,
  tests, persistence, receipts, and goldens are unchanged.
- Ownership: retained source owns exactly 31 mapped definitions at 496 lines;
  new dispatch source owns exactly 15 mapped definitions at 631 lines. All 46
  definition blocks and `RoomEditorMousePickValue` compare exactly to accepted
  K15. The automation folder is 14 files / 10 cpp / 4 hpp / 4,694 lines.
- Repair during takeover: the preserved partial handoff contained a duplicate
  `parseRoomEditorMousePickValue` definition and misplaced the private
  record/parser ahead of their original helper order. The duplicate was
  removed, original relative order restored, and obsolete includes pruned
  before verification.
- Dependency boundary: `AutomationRoomEditing.cpp` remains the sole direct
  Session include in this concern. `AutomationRoomEditingDispatch.cpp` has no
  direct runtime include. Policy is 656 sources / 314 direct edges / 16
  directed and unordered pairs / app fan-out 185 / zero SCCs and violations.
- Tests/checks run: `cmake -S . -B build`; cache-disabled all-target build;
  exact focused CTest 18/18; definition/record parity; exact scope; protected
  diffs; BG-1234 branch relocation; branch-gate tool; dependency policy; and
  whitespace checks all passed.
- Commit: this card is included in the single K16 implementation commit; its
  exact hash is reported in the Reviewer handoff after the transaction.
- Concerns/deferred: none within K16. Post-acceptance file-spec freshness stays
  in the Cartographer queue and is not part of this implementation commit.
