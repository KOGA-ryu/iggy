# K15a - Automation Ownership Split

## Status

READY v1.0. K15 batch step 1 of 2. Claim only at accepted K14 HEAD
`15ac73f3ea92848afa01669914bfdd137c6ce15b`. Authority:
`docs/creative_mode/builder_tasks/blocked/K15-automation-ownership-and-lookup-plan.md`.

On green completion, move this card to `done/`, commit once, then claim
`K15b-automation-lookup-deduplication.md` without waiting for Reviewer. A STOP
pauses the whole batch. K15a is source-equivalent ownership work; do not perform
K15b's lookup rewrite in this commit.

## Goal

Split 1,889-line `Automation.cpp` into parsing/common, metadata/resolver, and
world-setup owners behind byte-unchanged `Automation.hpp`. Absorb three
dispatch-only micro-headers into `AutomationDispatch.hpp` and delete them.

Metric promise after K15a:

- automation files 14 -> 13;
- implementations 7 -> 9; headers 7 -> 4;
- `Automation.cpp` at most 600 lines;
- `AutomationMetadata.cpp` at most 830 lines;
- `AutomationWorldSetup.cpp` at most 640 lines;
- `AutomationDispatch.hpp` at most 110 lines;
- dependency source files 656 -> 655;
- direct-edge/app-fan-out/directed-pair/unordered-pair deltas all zero;
- no public header, function body, test, or behavior changes; and
- one implementation commit.

Line limits are upper bounds, never padding targets.

## Pre-Edit Baseline

Run before editing:

```sh
test "$(git rev-parse HEAD)" = 15ac73f3ea92848afa01669914bfdd137c6ce15b
test -z "$(git status --short --untracked-files=no -- src apps tests tools CMakeLists.txt cmake docs/branch_gate_approvals.tsv docs/architecture_dependency_policy.json)"
test "$(find src/app/iggy3d/automation -maxdepth 1 -type f \( -name '*.hpp' -o -name '*.cpp' \) | wc -l | tr -d ' ')" = 14
test "$(find src/app/iggy3d/automation -maxdepth 1 -type f -name '*.cpp' | wc -l | tr -d ' ')" = 7
test "$(find src/app/iggy3d/automation -maxdepth 1 -type f -name '*.hpp' | wc -l | tr -d ' ')" = 7
test "$(find src/app/iggy3d/automation -maxdepth 1 -type f \( -name '*.hpp' -o -name '*.cpp' \) -print0 | xargs -0 cat | wc -l | tr -d ' ')" = 4708
test "$(wc -l < src/app/iggy3d/automation/Automation.cpp | tr -d ' ')" = 1889
test "$(wc -l < src/app/iggy3d/automation/Automation.hpp | tr -d ' ')" = 378
test "$(wc -l < src/app/iggy3d/automation/AutomationDispatch.hpp | tr -d ' ')" = 56
test "$(wc -l < src/app/iggy3d/automation/AutomationGameplay.hpp | tr -d ' ')" = 23
test "$(wc -l < src/app/iggy3d/automation/AutomationSaveBrowser.hpp | tr -d ' ')" = 25
test "$(wc -l < src/app/iggy3d/automation/AutomationSystem.hpp | tr -d ' ')" = 24
python3 - <<'PY'
from pathlib import Path

expected = {
  'AutomationGameplay.hpp': {
    'src/app/iggy3d/automation/AutomationDispatch.cpp',
    'src/app/iggy3d/automation/AutomationGameplay.cpp',
  },
  'AutomationSaveBrowser.hpp': {
    'src/app/iggy3d/automation/AutomationDispatch.cpp',
    'src/app/iggy3d/automation/AutomationSaveBrowser.cpp',
  },
  'AutomationSystem.hpp': {
    'src/app/iggy3d/automation/AutomationDispatch.cpp',
    'src/app/iggy3d/automation/AutomationSystem.cpp',
  },
}
files = [
  path
  for base in ('src', 'apps', 'tests')
  for path in Path(base).rglob('*')
  if path.is_file() and path.suffix in {'.hpp', '.cpp', '.h', '.cc', '.cxx'}
]
for basename, wanted in expected.items():
  actual = {
    str(path) for path in files
    if basename in path.read_text(errors='ignore')
  }
  if actual != wanted:
    raise SystemExit(f'{basename}: expected {sorted(wanted)}, got {sorted(actual)}')
print({key: sorted(value) for key, value in expected.items()})
PY
python3 - <<'PY'
from pathlib import Path
import re

text = Path('src/app/iggy3d/automation/Automation.cpp').read_text()
unique_names = '''
productAutomationCommandCategoryName productAutomationValueKindName
productAutomationCommandIdName parseProductAutomationBool
parseProductAutomationFloat splitProductAutomationCsv
parseProductAutomationCsvFloat parseProductAutomationFloorCommand
parseProductAutomationWallCommand parseAutomationTableValue
parseProductRoomEditorDirection parseProductRoomEditorTool
parseProductRoomEditorInputAction parseProductAutomationOwner
hasProductAutomationKey readProductAutomationCommands
makeProductAutomationCommandRegistry findProductAutomationCommandSpec
productAutomationCanonicalKey findProductAutomationCommandDispatchSpec
resolveProductAutomationCommandDispatch resolveProductMenuShortcutAutomation
resolveProductMenuInputAutomation resolveProductFrontendSelectAutomation
resolveProductSettingsTabAutomation resolveProductDevToolsCategoryAutomation
resolveProductSaveSelectionAutomation resolveProductGameplayAxisAutomation
resolveProductNonEmptyStringAutomation
resolveProductDungeonDraftDirectionAutomation
resolveProductDungeonDraftPaintAutomation parseProductAutomationSize
resolveProductDungeonDraftCellAutomation
resolveProductSaveBrowserBoolAutomation recordWorldSetupDraftState
dungeonDraftCursorFromWindow recordDungeonDraftOperation
resetDungeonDraftWindowCursor applyDungeonDraftPaintGlyph
selectDungeonDraftPaintGlyph readAutomationTextFile
titleFromAutomationAsciiRoomPath markAutomationApplied
applyProductCommonAutomationCommand applyProductWorldSetupAutomationCommand
'''.split()
if len(unique_names) != 45 or len(set(unique_names)) != 45:
  raise SystemExit('K15 unique function map must contain 45 names')
for name in unique_names:
  count = len(re.findall(
      rf'(?m)^[A-Za-z_][^\n]*\b{re.escape(name)}\s*\(', text))
  if count != 1:
    raise SystemExit(f'{name}: expected one definition start, got {count}')
if len(re.findall(
    r'(?m)^[A-Za-z_][^\n]*\bresolveProductAutomationBool\s*\(',
    text)) != 2:
  raise SystemExit('resolveProductAutomationBool must have exactly two overloads')
print({'unique_definitions': 45, 'bool_overloads': 2, 'total': 47})
PY
test "$(rg -c 'static constexpr std::array lookup' src/app/iggy3d/automation/Automation.cpp)" = 8
test "$(rg -c 'std::distance' src/app/iggy3d/automation/Automation.cpp)" = 8
test "$(rg -c 'std::min' src/app/iggy3d/automation/Automation.cpp)" = 8
test "$(rg -c 'src/app/iggy3d/automation/Automation\.cpp' CMakeLists.txt)" = 1
cmake -S . -B build
CCACHE_DISABLE=1 cmake --build build --target all
ctest --test-dir build -R '^(branch_gate_tool_tests|dependency_direction_tests|product_automation_command_registry_tests|product_automation_dispatch_tests|product_new_world_menu_action_tests|product_window_input_frame_tests|product_interaction_mode_state_tests|product_room_editor_action_controller_tests|product_ascii_room_activation_tests|product_room_editor_preview_tests|product_save_delete_executor_tests|product_gameplay_tape_runner_tests|product_room_editing_automation_smoke|product_automation_menu_smoke|product_world_setup_smoke|product_save_delete_recover_smoke|product_controller_input_smoke|product_gameplay_tape_smoke)$' --output-on-failure
python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json > /tmp/k15_dependency_before.json
python3 tools/check_branch_gate.py
git diff --check
```

The focused set must pass 18/18. Dependency baseline must be
656/314/16/16/app-185 with zero SCCs/violations. Existing warnings are not K15
work. Any mismatch is a STOP before edits.

## Exact Files

Create:

- `src/app/iggy3d/automation/AutomationMetadata.cpp`;
- `src/app/iggy3d/automation/AutomationWorldSetup.cpp`.

Delete:

- `src/app/iggy3d/automation/AutomationGameplay.hpp`;
- `src/app/iggy3d/automation/AutomationSaveBrowser.hpp`;
- `src/app/iggy3d/automation/AutomationSystem.hpp`.

Update only:

- `CMakeLists.txt`;
- `docs/branch_gate_approvals.tsv`;
- `src/app/iggy3d/automation/Automation.cpp`;
- `src/app/iggy3d/automation/AutomationDispatch.hpp`;
- `src/app/iggy3d/automation/AutomationDispatch.cpp`;
- `src/app/iggy3d/automation/AutomationGameplay.cpp`;
- `src/app/iggy3d/automation/AutomationSaveBrowser.cpp`;
- `src/app/iggy3d/automation/AutomationSystem.cpp`; and
- this card when moved to `done/`.

`Automation.hpp`, `AutomationControl.*`, `AutomationRoomEditing.*`, all tests,
and `cmake/iggy3d_tests.cmake` are protected and must remain byte-for-byte
unchanged.

## Step 1 - Create `AutomationMetadata.cpp`

Include `Automation.hpp` as the owner contract. Add exactly this marker outside
function bodies:

```cpp
// branch-gate-relocation: BG-1231 from=src/app/iggy3d/automation/Automation.cpp
```

Move these complete definitions source-equivalently, in original relative
order:

- the three `productAutomation*Name` functions;
- registry construction, registry lookup, and canonical-key resolution;
- dispatch-spec construction/lookup and dispatch resolution;
- menu-shortcut, menu-input, frontend-select, settings-tab, dev-tools-category,
  and save-selection resolvers;
- both `resolveProductAutomationBool` overloads;
- dungeon-draft-direction resolver; and
- save-browser-bool resolver.

The governing plan lists all 18 definitions explicitly. Move local row structs,
all `rows` and `lookup` arrays, fallback entries, branch comments, and statement
order unchanged. K15a must still contain 8 copied `lookup` arrays, 8
`std::distance`, and 8 `std::min` in this new owner.

Use only direct standard/app includes required by those moved bodies. Do not
include runtime, render, projection, content, config, or core headers directly.

## Step 2 - Create `AutomationWorldSetup.cpp`

Include `Automation.hpp` as the owner contract. Add exactly this marker outside
function bodies:

```cpp
// branch-gate-relocation: BG-1232 from=src/app/iggy3d/automation/Automation.cpp
```

Move these complete definitions source-equivalently, in original relative
order:

- `recordWorldSetupDraftState`;
- `dungeonDraftCursorFromWindow`;
- `recordDungeonDraftOperation`;
- `resetDungeonDraftWindowCursor`;
- `applyDungeonDraftPaintGlyph`;
- `selectDungeonDraftPaintGlyph`;
- `readAutomationTextFile`;
- `titleFromAutomationAsciiRoomPath`; and
- `applyProductWorldSetupAutomationCommand`.

Keep the two file/title helpers private in one anonymous namespace. Move every
branch-gate comment with its statement. Preserve command comparison order,
screen/owner checks, parse order, status/reason strings, state writes,
callbacks, applied recording, and return values.

Use only direct standard/app includes required by those bodies. Do not add a
new internal header or department edge.

## Step 3 - Reduce `Automation.cpp`

Retain exactly the 20 uniquely named definitions listed under the governing
plan's `Automation.cpp retains` section, in original order. Retain
`AutomationParserRow` and `RoomEditorInputActionRow` source-equivalently.

Remove includes only after their last direct use moves. Do not rewrite parser
tables, command-file behavior, common-command branches, public signatures, or
the two file-local row types.

The final file must contain no moved metadata/world definition, no relocation
marker, and none of the eight copied `lookup` arrays. Those arrays belong
source-equivalently in `AutomationMetadata.cpp` until K15b.

## Step 4 - Condense Dispatch Context Headers

In `AutomationDispatch.hpp`, after existing forward declarations and before
`ProductAutomationDispatchContext`, move complete blocks in this order:

1. `ProductAutomationGameplayContext` plus
   `applyProductGameplayAutomationCommand`;
2. `ProductAutomationSaveBrowserContext` plus
   `applyProductSaveBrowserAutomationCommand`;
3. `ProductAutomationSystemContext` plus
   `applyProductSystemAutomationCommand`.

Preserve each struct member/default and declaration source-equivalently. Leave
the existing `ProductAutomationDispatchContext` and
`ProductAutomationAppContext` blocks byte-equivalent.

Replace the self-header include in each corresponding implementation with
`AutomationDispatch.hpp`. Remove all three obsolete includes from
`AutomationDispatch.cpp`. Delete the three headers. Do not change any
implementation namespace body.

## Step 5 - CMake And Branch Ledger

In `CMakeLists.txt`, add exactly one source entry for each new implementation
adjacent to `Automation.cpp`:

- `src/app/iggy3d/automation/AutomationMetadata.cpp`;
- `src/app/iggy3d/automation/AutomationWorldSetup.cpp`.

Do not change target membership or test registration.

Append exactly two approval rows using the existing TSV schema:

- BG-1231: exact metadata/registry/resolver branch-set relocation from
  `Automation.cpp` to `AutomationMetadata.cpp`;
- BG-1232: exact world-setup branch-set relocation from `Automation.cpp` to
  `AutomationWorldSetup.cpp`.

Do not modify the branch checker or oracle.

## Mechanical Ownership And Parity Gates

Run before staging:

```sh
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

root = Path('src/app/iggy3d/automation')

def at_head(path):
  return subprocess.run(
      ['git', 'show', f'HEAD:{path}'], check=True,
      capture_output=True, text=True).stdout

def definition_blocks(text, name):
  blocks = []
  for match in re.finditer(
      rf'(?m)^[A-Za-z_][^\n]*\b{re.escape(name)}\s*\(', text):
    start = match.start()
    brace = text.find('{', match.end())
    if brace < 0:
      raise SystemExit(f'{name}: opening brace not found')
    depth = 0
    state = 'code'
    i = brace
    while i < len(text):
      char = text[i]
      nxt = text[i + 1] if i + 1 < len(text) else ''
      if state == 'line':
        if char == '\n':
          state = 'code'
      elif state == 'block':
        if char == '*' and nxt == '/':
          state = 'code'
          i += 1
      elif state in {'single', 'double'}:
        quote = "'" if state == 'single' else '"'
        if char == '\\':
          i += 1
        elif char == quote:
          state = 'code'
      else:
        if char == '/' and nxt == '/':
          state = 'line'
          i += 1
        elif char == '/' and nxt == '*':
          state = 'block'
          i += 1
        elif char == "'":
          state = 'single'
        elif char == '"':
          state = 'double'
        elif char == '{':
          depth += 1
        elif char == '}':
          depth -= 1
          if depth == 0:
            blocks.append(text[start:i + 1].strip())
            break
      i += 1
    else:
      raise SystemExit(f'{name}: closing brace not found')
  return blocks

old = at_head('src/app/iggy3d/automation/Automation.cpp')
owners = {
  'src/app/iggy3d/automation/Automation.cpp': '''
parseProductAutomationBool parseProductAutomationFloat
splitProductAutomationCsv parseProductAutomationCsvFloat
parseProductAutomationFloorCommand parseProductAutomationWallCommand
parseAutomationTableValue parseProductRoomEditorDirection
parseProductRoomEditorTool parseProductRoomEditorInputAction
parseProductAutomationOwner hasProductAutomationKey
readProductAutomationCommands resolveProductGameplayAxisAutomation
resolveProductNonEmptyStringAutomation resolveProductDungeonDraftPaintAutomation
parseProductAutomationSize resolveProductDungeonDraftCellAutomation
markAutomationApplied applyProductCommonAutomationCommand
'''.split(),
  'src/app/iggy3d/automation/AutomationMetadata.cpp': '''
productAutomationCommandCategoryName productAutomationValueKindName
productAutomationCommandIdName makeProductAutomationCommandRegistry
findProductAutomationCommandSpec productAutomationCanonicalKey
findProductAutomationCommandDispatchSpec resolveProductAutomationCommandDispatch
resolveProductMenuShortcutAutomation resolveProductMenuInputAutomation
resolveProductFrontendSelectAutomation resolveProductSettingsTabAutomation
resolveProductDevToolsCategoryAutomation resolveProductSaveSelectionAutomation
resolveProductDungeonDraftDirectionAutomation
resolveProductSaveBrowserBoolAutomation
'''.split(),
  'src/app/iggy3d/automation/AutomationWorldSetup.cpp': '''
recordWorldSetupDraftState dungeonDraftCursorFromWindow
recordDungeonDraftOperation resetDungeonDraftWindowCursor
applyDungeonDraftPaintGlyph selectDungeonDraftPaintGlyph
readAutomationTextFile titleFromAutomationAsciiRoomPath
applyProductWorldSetupAutomationCommand
'''.split(),
}

all_names = [name for names in owners.values() for name in names]
if len(all_names) != 45 or len(set(all_names)) != 45:
  raise SystemExit('K15 owner map must contain 45 unique names')
for owner, names in owners.items():
  current = Path(owner).read_text()
  for name in names:
    before = definition_blocks(old, name)
    after = definition_blocks(current, name)
    if len(before) != 1 or after != before:
      raise SystemExit(f'{name}: source/owner mismatch in {owner}')

bool_before = definition_blocks(old, 'resolveProductAutomationBool')
bool_after = definition_blocks(
    Path('src/app/iggy3d/automation/AutomationMetadata.cpp').read_text(),
    'resolveProductAutomationBool')
if len(bool_before) != 2 or bool_after != bool_before:
  raise SystemExit('resolveProductAutomationBool overloads changed or misplaced')

definition_owners = {name: [] for name in all_names}
for path in root.glob('*.cpp'):
  text = path.read_text()
  for name in all_names:
    if definition_blocks(text, name):
      definition_owners[name].append(str(path))
expected = {
  name: [owner]
  for owner, names in owners.items()
  for name in names
}
if definition_owners != expected:
  raise SystemExit(f'definition owners differ: {definition_owners}')
bool_owners = [
  str(path) for path in root.glob('*.cpp')
  if definition_blocks(path.read_text(), 'resolveProductAutomationBool')
]
if bool_owners != ['src/app/iggy3d/automation/AutomationMetadata.cpp']:
  raise SystemExit(f'bool overload owner differs: {bool_owners}')
print({'unique_definitions': 45, 'bool_overloads': 2, 'total': 47})
PY
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

def at_head(path):
  return subprocess.run(
      ['git', 'show', f'HEAD:{path}'], check=True,
      capture_output=True, text=True).stdout

def struct_block(text, name):
  match = re.search(
      rf'^struct {re.escape(name)} \{{.*?^\}};', text,
      flags=re.MULTILINE | re.DOTALL)
  if match is None:
    raise SystemExit(f'missing struct: {name}')
  return match.group(0)

def declaration_block(text, name):
  prefix = f'ProductAutomationExecutionResult {name}'
  if text.count(prefix) != 1:
    raise SystemExit(f'{name}: expected one declaration')
  start = text.index(prefix)
  return text[start:text.index(';', start) + 1]

dispatch_path = 'src/app/iggy3d/automation/AutomationDispatch.hpp'
dispatch = Path(dispatch_path).read_text()
moved = {
  'ProductAutomationGameplayContext':
      'src/app/iggy3d/automation/AutomationGameplay.hpp',
  'ProductAutomationSaveBrowserContext':
      'src/app/iggy3d/automation/AutomationSaveBrowser.hpp',
  'ProductAutomationSystemContext':
      'src/app/iggy3d/automation/AutomationSystem.hpp',
}
for name, old_path in moved.items():
  if struct_block(dispatch, name) != struct_block(at_head(old_path), name):
    raise SystemExit(f'moved context changed: {name}')
declarations = {
  'applyProductGameplayAutomationCommand':
      'src/app/iggy3d/automation/AutomationGameplay.hpp',
  'applyProductSaveBrowserAutomationCommand':
      'src/app/iggy3d/automation/AutomationSaveBrowser.hpp',
  'applyProductSystemAutomationCommand':
      'src/app/iggy3d/automation/AutomationSystem.hpp',
}
for name, old_path in declarations.items():
  if declaration_block(dispatch, name) != declaration_block(
      at_head(old_path), name):
    raise SystemExit(f'moved declaration changed: {name}')
for name in ('ProductAutomationDispatchContext', 'ProductAutomationAppContext'):
  if struct_block(dispatch, name) != struct_block(at_head(dispatch_path), name):
    raise SystemExit(f'existing dispatch context changed: {name}')

for path in (
    'src/app/iggy3d/automation/AutomationDispatch.cpp',
    'src/app/iggy3d/automation/AutomationGameplay.cpp',
    'src/app/iggy3d/automation/AutomationSaveBrowser.cpp',
    'src/app/iggy3d/automation/AutomationSystem.cpp',
):
  current = Path(path).read_text()
  before = at_head(path)
  current_body = current[current.index('namespace iggy3d {'):]
  before_body = before[before.index('namespace iggy3d {'):]
  if current_body != before_body:
    raise SystemExit(f'implementation namespace body changed: {path}')

if Path('src/app/iggy3d/automation/Automation.hpp').read_text() != at_head(
    'src/app/iggy3d/automation/Automation.hpp'):
  raise SystemExit('Automation.hpp changed')
for path in (
    'src/app/iggy3d/automation/AutomationControl.hpp',
    'src/app/iggy3d/automation/AutomationControl.cpp',
    'src/app/iggy3d/automation/AutomationRoomEditing.hpp',
    'src/app/iggy3d/automation/AutomationRoomEditing.cpp',
):
  if Path(path).read_text() != at_head(path):
    raise SystemExit(f'protected automation owner changed: {path}')
print({
  'moved_contexts': 3,
  'moved_declarations': 3,
  'preserved_dispatch_contexts': 2,
})
PY
```

## Structural And Branch Gates

```sh
python3 - <<'PY'
import subprocess

expected_tracked = {
  'CMakeLists.txt',
  'docs/branch_gate_approvals.tsv',
  'src/app/iggy3d/automation/Automation.cpp',
  'src/app/iggy3d/automation/AutomationDispatch.cpp',
  'src/app/iggy3d/automation/AutomationDispatch.hpp',
  'src/app/iggy3d/automation/AutomationGameplay.cpp',
  'src/app/iggy3d/automation/AutomationGameplay.hpp',
  'src/app/iggy3d/automation/AutomationSaveBrowser.cpp',
  'src/app/iggy3d/automation/AutomationSaveBrowser.hpp',
  'src/app/iggy3d/automation/AutomationSystem.cpp',
  'src/app/iggy3d/automation/AutomationSystem.hpp',
}
expected_task_untracked = {
  'src/app/iggy3d/automation/AutomationMetadata.cpp',
  'src/app/iggy3d/automation/AutomationWorldSetup.cpp',
}
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
      f'K15a scope differs: tracked={sorted(tracked)}, '
      f'task_untracked={sorted(task_untracked)}')
print({'tracked': len(tracked), 'task_untracked': sorted(task_untracked)})
PY
python3 - <<'PY'
from pathlib import Path
import subprocess
import tools.check_branch_gate as gate

diff = subprocess.run(
    ['git', 'diff', '--unified=0'], check=True,
    capture_output=True, text=True).stdout
for path in (
    'src/app/iggy3d/automation/AutomationMetadata.cpp',
    'src/app/iggy3d/automation/AutomationWorldSetup.cpp',
):
  result = subprocess.run(
      ['git', 'diff', '--no-index', '--unified=0', '--', '/dev/null', path],
      check=False, capture_output=True, text=True)
  if result.returncode not in (0, 1):
    raise SystemExit(result.stderr)
  diff += '\n' + result.stdout
findings = gate.scan_diff(
    diff, gate.load_approval_ids(Path('docs/branch_gate_approvals.tsv')), False)
if findings:
  for finding in findings:
    print(f'{finding.path}:{finding.new_line}: '
          f'{finding.statement}: {finding.reason}')
  raise SystemExit('combined unstaged branch gate failed')
print('combined unstaged branch gate: ok')
PY
python3 - <<'PY'
from pathlib import Path

for basename in (
    'AutomationGameplay.hpp',
    'AutomationSaveBrowser.hpp',
    'AutomationSystem.hpp',
):
  hits = []
  for base in ('src', 'apps', 'tests', 'cmake'):
    hits.extend(
        str(path) for path in Path(base).rglob('*')
        if path.is_file() and basename in path.read_text(errors='ignore'))
  if basename in Path('CMakeLists.txt').read_text():
    hits.append('CMakeLists.txt')
  if hits:
    raise SystemExit(f'stale basename {basename}: {hits}')
print('three retired header basenames absent')
PY
test "$(find src/app/iggy3d/automation -maxdepth 1 -type f \( -name '*.hpp' -o -name '*.cpp' \) | wc -l | tr -d ' ')" = 13
test "$(find src/app/iggy3d/automation -maxdepth 1 -type f -name '*.cpp' | wc -l | tr -d ' ')" = 9
test "$(find src/app/iggy3d/automation -maxdepth 1 -type f -name '*.hpp' | wc -l | tr -d ' ')" = 4
test "$(wc -l < src/app/iggy3d/automation/Automation.cpp | tr -d ' ')" -le 600
test "$(wc -l < src/app/iggy3d/automation/AutomationMetadata.cpp | tr -d ' ')" -le 830
test "$(wc -l < src/app/iggy3d/automation/AutomationWorldSetup.cpp | tr -d ' ')" -le 640
test "$(wc -l < src/app/iggy3d/automation/AutomationDispatch.hpp | tr -d ' ')" -le 110
test "$(rg -c 'static constexpr std::array lookup' src/app/iggy3d/automation/AutomationMetadata.cpp)" = 8
test "$(rg -c 'std::distance' src/app/iggy3d/automation/AutomationMetadata.cpp)" = 8
test "$(rg -c 'std::min' src/app/iggy3d/automation/AutomationMetadata.cpp)" = 8
test "$(rg -c 'src/app/iggy3d/automation/AutomationMetadata\.cpp' CMakeLists.txt)" = 1
test "$(rg -c 'src/app/iggy3d/automation/AutomationWorldSetup\.cpp' CMakeLists.txt)" = 1
git diff --check
```

Do not pad files. A natural hard-bound miss is a STOP.

## Post-Edit Verification

```sh
cmake -S . -B build
CCACHE_DISABLE=1 cmake --build build --target all
ctest --test-dir build -R '^(branch_gate_tool_tests|dependency_direction_tests|product_automation_command_registry_tests|product_automation_dispatch_tests|product_new_world_menu_action_tests|product_window_input_frame_tests|product_interaction_mode_state_tests|product_room_editor_action_controller_tests|product_ascii_room_activation_tests|product_room_editor_preview_tests|product_save_delete_executor_tests|product_gameplay_tape_runner_tests|product_room_editing_automation_smoke|product_automation_menu_smoke|product_world_setup_smoke|product_save_delete_recover_smoke|product_controller_input_smoke|product_gameplay_tape_smoke)$' --output-on-failure
python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json > /tmp/k15a_dependency_after.json
python3 - <<'PY'
import json

before = json.load(open('/tmp/k15_dependency_before.json'))
after = json.load(open('/tmp/k15a_dependency_after.json'))
if before['scan']['source_file_count'] != 656:
  raise SystemExit('K15 baseline source count changed')
if after['scan']['source_file_count'] != 655:
  raise SystemExit(f'K15a source count: {after["scan"]["source_file_count"]}')
for key in ('direct_edge_count', 'directed_pair_count', 'unordered_pair_count'):
  if after['scan'][key] != before['scan'][key]:
    raise SystemExit(f'{key} changed')
if before['fan_out']['app']['edge_count'] != 185 or after['fan_out']['app']['edge_count'] != 185:
  raise SystemExit('app fan-out changed')
if after['strongly_connected_components'] or not after['policy']['passed']:
  raise SystemExit('dependency policy failed')
print({'sources': [656, 655], 'edges': 314, 'pairs': [16, 16], 'app': 185})
PY
python3 tools/check_branch_gate.py
git diff --exit-code HEAD -- tests cmake/iggy3d_tests.cmake tools/check_branch_gate.py tests/tools/branch_gate_tests.py docs/architecture_dependency_policy.json src/runtime src/render src/projection src/runtime/save src/runtime/replay src/app/iggy3d/receipt tests/golden docs/file_specs docs/creative_mode/optimization
git diff --check
```

The focused set must pass 18/18. Rerun all ownership, scope, branch, stale-name,
line, and CMake gates after any correction. Do not run broad CTest or launch a
window.

## Single Git Transaction

After every unstaged gate passes, move this card to `done/` with an ordinary
worktree move. Then make one permission-bearing command request containing:

```sh
git add -A -- CMakeLists.txt docs/branch_gate_approvals.tsv docs/creative_mode/builder_tasks/done/K15a-automation-ownership-split.md src/app/iggy3d/automation/Automation.cpp src/app/iggy3d/automation/AutomationMetadata.cpp src/app/iggy3d/automation/AutomationWorldSetup.cpp src/app/iggy3d/automation/AutomationDispatch.hpp src/app/iggy3d/automation/AutomationDispatch.cpp src/app/iggy3d/automation/AutomationGameplay.hpp src/app/iggy3d/automation/AutomationGameplay.cpp src/app/iggy3d/automation/AutomationSaveBrowser.hpp src/app/iggy3d/automation/AutomationSaveBrowser.cpp src/app/iggy3d/automation/AutomationSystem.hpp src/app/iggy3d/automation/AutomationSystem.cpp
python3 tools/check_branch_gate.py --cached
git diff --cached --check
git commit -m "codex: split automation ownership (K15a)"
```

Do not include the absent ready-card path. If any line fails, STOP without an
automatic retry and report exact staged/unstaged state.

After commit:

```sh
python3 tools/check_branch_gate.py --diff HEAD^..HEAD
git diff --check HEAD^ HEAD
git show --stat --oneline HEAD
```

Then claim K15b. Do not request aggregate review yet.

## Stop Conditions

- Any accepted baseline, source-clean, file/line/consumer/definition/table,
  all-target, 18-test, policy, branch, or whitespace premise fails.
- A moved definition/context/declaration cannot remain source-equivalent, or a
  final owner differs from the exact map.
- `Automation.hpp`, a protected implementation, any implementation namespace
  body, test, or public API must change.
- The work needs a new header, duplicate helper, changed row/branch/order,
  behavior re-pin, department edge, or unrelated cleanup.
- An old basename remains, CMake/source ownership differs, folder shape is not
  13/9/4, or a line bound fails.
- Dependency final state differs from 655/314/16/16/app-185 with zero
  SCCs/violations.
- Final build, 18/18 tests, parity, branch, protected diff, or whitespace fails.
- The final Git transaction needs a retry.

## Completion Brief

Append the standard completion brief and report commit hash, all 47 definition
owners/parity, three context-header folds, protected public/implementation
proof, branch relocations, final line/folder metrics, build and 18/18 results,
dependency delta, and transaction result.

Then continue to `K15b-automation-lookup-deduplication.md`.
