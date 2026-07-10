# K15b - Automation Lookup Deduplication

## Status

READY v1.0. K15 batch step 2 of 2. Prerequisite:
`done/K15a-automation-ownership-split.md` exists and HEAD is the committed K15a
result with subject `codex: split automation ownership (K15a)`.

Authority:
`docs/creative_mode/builder_tasks/blocked/K15-automation-ownership-and-lookup-plan.md`.

Do not claim before K15a is committed and green. On completion, move this card
to `done/`, commit once, and send Reviewer one aggregate K15 brief covering
both commits. A STOP pauses the batch.

## Goal

Replace eight copied lookup arrays and their distance/min selection logic in
`AutomationMetadata.cpp` with one private row finder. Preserve every row,
valid mapping, invalid fallback, public API, and command behavior.

Final metric promise:

- exactly one private `findAutomationRow` definition and eight calls;
- zero `static constexpr std::array lookup` arrays;
- zero `std::distance` and zero `std::min` in `AutomationMetadata.cpp`;
- `AutomationMetadata.cpp` at most 720 lines;
- automation folder at most 4,680 physical lines and remains 13/9/4;
- dependency policy remains 655/314/16/16/app-185;
- only the metadata implementation, focused registry test, branch ledger, and
  this card change; and
- one implementation commit.

Do not pad files to a line estimate.

## Prerequisite And Pre-Edit Gate

```sh
test -f docs/creative_mode/builder_tasks/done/K15a-automation-ownership-split.md
test "$(git show -s --format=%s HEAD)" = "codex: split automation ownership (K15a)"
test -z "$(git status --short --untracked-files=no -- src apps tests tools CMakeLists.txt cmake docs/branch_gate_approvals.tsv docs/architecture_dependency_policy.json)"
test -f src/app/iggy3d/automation/AutomationMetadata.cpp
test -f src/app/iggy3d/automation/AutomationWorldSetup.cpp
test ! -e src/app/iggy3d/automation/AutomationGameplay.hpp
test ! -e src/app/iggy3d/automation/AutomationSaveBrowser.hpp
test ! -e src/app/iggy3d/automation/AutomationSystem.hpp
test "$(find src/app/iggy3d/automation -maxdepth 1 -type f \( -name '*.hpp' -o -name '*.cpp' \) | wc -l | tr -d ' ')" = 13
test "$(find src/app/iggy3d/automation -maxdepth 1 -type f -name '*.cpp' | wc -l | tr -d ' ')" = 9
test "$(find src/app/iggy3d/automation -maxdepth 1 -type f -name '*.hpp' | wc -l | tr -d ' ')" = 4
test "$(rg -c 'static constexpr std::array lookup' src/app/iggy3d/automation/AutomationMetadata.cpp)" = 8
test "$(rg -c 'std::distance' src/app/iggy3d/automation/AutomationMetadata.cpp)" = 8
test "$(rg -c 'std::min' src/app/iggy3d/automation/AutomationMetadata.cpp)" = 8
test "$(rg -c 'findAutomationRow' src/app/iggy3d/automation/AutomationMetadata.cpp || true)" = 0
test "$(wc -l < src/app/iggy3d/automation/AutomationMetadata.cpp | tr -d ' ')" -le 830
cmake -S . -B build
CCACHE_DISABLE=1 cmake --build build --target product_automation_command_registry_tests
ctest --test-dir build -R '^product_automation_command_registry_tests$' --output-on-failure
python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json > /tmp/k15b_dependency_before.json
python3 tools/check_branch_gate.py
git diff --check
```

The dependency baseline must be 655/314/16/16/app-185 with zero
SCCs/violations. A lookup count or prerequisite mismatch is a STOP.

## Exact Files

Update only:

- `docs/branch_gate_approvals.tsv`;
- `src/app/iggy3d/automation/AutomationMetadata.cpp`;
- `tests/unit/product_automation_command_registry_tests.cpp`; and
- this card when moved to `done/`.

No CMake, header, other implementation, branch tool/oracle, or unrelated test
may change.

## Step 1 - Add One Private Row Finder

Inside `AutomationMetadata.cpp`, add one anonymous namespace immediately after
`namespace iggy3d {`. It owns exactly one result model and one helper:

```cpp
template <typename Row, std::size_t Count>
AutomationRowLookup<Row> findAutomationRow(
    const std::array<Row, Count>& rows,
    std::string_view value,
    const Row& fallback);
```

where the preceding private result model is:

```cpp
template <typename Row>
struct AutomationRowLookup {
  const Row* row = nullptr;
  bool found = false;
};
```

Implementation contract:

1. perform one `std::find_if` comparing `candidate.name == value`;
2. add exactly one branch comment and not-found branch:

```cpp
// branch-gate: BG-1233
if (row == rows.end()) {
  return {&fallback, false};
}
```

3. return `{&*row, true}` on success;
4. never return a null selected-row pointer; and
5. own no fallback policy, logging, or public declaration. Callers supply the
   fallback row.

Append one BG-1233 ledger row describing the private automation lookup-miss
policy. Do not add another branch/ternary or modify the checker/oracle.

## Step 2 - Route Eight Resolvers

In each listed function, preserve the local row struct and the sole
`static constexpr std::array rows` initializer byte-for-byte. Replace the old
copied array's final `unknown` row with one local `static constexpr` fallback
row carrying the same initializer. Call
`findAutomationRow(rows, value, fallback)` once, delete the copied `lookup`
array, and delete all distance/min/index selection:

1. `resolveProductMenuShortcutAutomation`;
2. `resolveProductMenuInputAutomation`;
3. `resolveProductFrontendSelectAutomation`;
4. `resolveProductSettingsTabAutomation`;
5. `resolveProductDevToolsCategoryAutomation`;
6. the `ProductBoolAutomationResult` overload of
   `resolveProductAutomationBool`;
7. `resolveProductDungeonDraftDirectionAutomation`; and
8. `resolveProductSaveBrowserBoolAutomation`.

Use `AutomationRowLookup::found` for `valid` and its always-non-null `row`
pointer for the selected value. Add no branch or ternary in any resolver.
Preserve these exact fallback rows/rules:

- menu shortcut miss: `valid=false`, `routeRequested=false`, and
  `inputAction=spec.inputAction`;
- menu input miss: `InputAction::None`;
- frontend-select miss: `FrontendAction::None`;
- settings-tab miss: `FrontendSettingsTab::None`;
- dev-tools miss: `FrontendDevToolsCategory::None`;
- generic-bool miss: `requested=false`;
- dungeon-direction miss: `ProductDungeonDraftDirection::Up`;
- save-browser-bool miss: write `false` to `out`, then return false.

Do not change valid row values, row order, row names, return shapes, public
overloads, or callers. Do not turn the helper into a public/template utility
header.

## Step 3 - Pin Fallback Semantics

In `product_automation_command_registry_tests.cpp`, extend only existing
resolver coverage:

- for `menu.confirm`'s dispatch spec, assert `yes` is valid/routes,
  `no` is valid/does not route, and `teleport` is invalid/does not route; all
  three retain `InputAction::MenuConfirm`;
- extend existing invalid menu-input assertion with `InputAction::None`;
- extend existing invalid frontend-select assertion with
  `FrontendAction::None`;
- extend existing invalid settings assertion with
  `FrontendSettingsTab::None`;
- extend existing invalid dev-tools assertion with
  `FrontendDevToolsCategory::None`;
- extend existing invalid generic-bool assertion with `requested == false`;
- extend existing invalid dungeon-direction assertion with fallback `Up`; and
- set the save-browser bool output to true immediately before the invalid call,
  then assert the invalid call writes false.

Do not reorganize the test harness, rename existing assertions, or add a new
test target.

## Row-Parity And Lookup Gates

Run before staging:

```sh
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

path = 'src/app/iggy3d/automation/AutomationMetadata.cpp'
before = subprocess.run(
    ['git', 'show', f'HEAD:{path}'], check=True,
    capture_output=True, text=True).stdout
after = Path(path).read_text()

def definition_blocks(text, name):
  blocks = []
  for match in re.finditer(
      rf'(?m)^[A-Za-z_][^\n]*\b{re.escape(name)}\s*\(', text):
    start = match.start()
    brace = text.find('{', match.end())
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

def local_struct(block):
  match = re.search(r'(?ms)^  struct [A-Za-z_][A-Za-z0-9_]* \{.*?^  \};', block)
  if match is None:
    raise SystemExit('local row struct missing')
  return match.group(0)

def rows_block(block):
  marker = 'static constexpr std::array rows'
  start = block.index(marker)
  brace = block.index('{', start)
  depth = 0
  i = brace
  while i < len(block):
    if block[i] == '{': depth += 1
    elif block[i] == '}':
      depth -= 1
      if depth == 0:
        end = i + 1
        if end < len(block) and block[end] == ';': end += 1
        return block[start:end]
    i += 1
  raise SystemExit('rows block not closed')

targets = [
  ('resolveProductMenuShortcutAutomation', 0),
  ('resolveProductMenuInputAutomation', 0),
  ('resolveProductFrontendSelectAutomation', 0),
  ('resolveProductSettingsTabAutomation', 0),
  ('resolveProductDevToolsCategoryAutomation', 0),
  ('resolveProductAutomationBool', 0),
  ('resolveProductDungeonDraftDirectionAutomation', 0),
  ('resolveProductSaveBrowserBoolAutomation', 0),
]
for name, index in targets:
  old_blocks = definition_blocks(before, name)
  new_blocks = definition_blocks(after, name)
  if len(old_blocks) <= index or len(new_blocks) <= index:
    raise SystemExit(f'{name}: definition missing')
  old_block = old_blocks[index]
  new_block = new_blocks[index]
  if local_struct(old_block) != local_struct(new_block):
    raise SystemExit(f'{name}: local row struct changed')
  if rows_block(old_block) != rows_block(new_block):
    raise SystemExit(f'{name}: rows initializer changed')
print({'row_structs_and_tables_preserved': len(targets)})
PY
test "$(rg -c 'static constexpr std::array lookup' src/app/iggy3d/automation/AutomationMetadata.cpp || true)" = 0
test "$(rg -c 'std::distance' src/app/iggy3d/automation/AutomationMetadata.cpp || true)" = 0
test "$(rg -c 'std::min' src/app/iggy3d/automation/AutomationMetadata.cpp || true)" = 0
test "$(rg -c 'static constexpr [A-Za-z_][A-Za-z0-9_]* fallback' src/app/iggy3d/automation/AutomationMetadata.cpp)" = 8
test "$(rg -c 'findAutomationRow' src/app/iggy3d/automation/AutomationMetadata.cpp)" = 9
test "$(rg -c 'AutomationRowLookup<Row> findAutomationRow' src/app/iggy3d/automation/AutomationMetadata.cpp)" = 1
test "$(rg -c 'struct AutomationRowLookup' src/app/iggy3d/automation/AutomationMetadata.cpp)" = 1
test "$(rg -c 'branch-gate: BG-1233' src/app/iggy3d/automation/AutomationMetadata.cpp)" = 1
test "$(wc -l < src/app/iggy3d/automation/AutomationMetadata.cpp | tr -d ' ')" -le 720
test "$(find src/app/iggy3d/automation -maxdepth 1 -type f \( -name '*.hpp' -o -name '*.cpp' \) -print0 | xargs -0 cat | wc -l | tr -d ' ')" -le 4680
git diff --check
```

The `findAutomationRow` count is one definition plus eight calls. If natural
code exceeds a hard bound, STOP rather than padding or compressing unrelated
code.

## Scope And Final Verification

```sh
python3 - <<'PY'
import subprocess

expected = {
  'docs/branch_gate_approvals.tsv',
  'src/app/iggy3d/automation/AutomationMetadata.cpp',
  'tests/unit/product_automation_command_registry_tests.cpp',
}
actual = set(subprocess.run(
    ['git', 'diff', '--name-only', 'HEAD', '--', 'src', 'apps', 'tests',
     'CMakeLists.txt', 'cmake', 'tools', 'docs/branch_gate_approvals.tsv',
     'docs/architecture_dependency_policy.json'],
    check=True, capture_output=True, text=True).stdout.splitlines())
task_untracked = set(subprocess.run(
    ['git', 'ls-files', '--others', '--exclude-standard', '--', 'src', 'apps',
     'tests', 'CMakeLists.txt', 'cmake', 'docs/branch_gate_approvals.tsv',
     'docs/architecture_dependency_policy.json'],
    check=True, capture_output=True, text=True).stdout.splitlines())
if actual != expected or task_untracked:
  raise SystemExit(
      f'K15b scope differs: tracked={sorted(actual)}, '
      f'task_untracked={sorted(task_untracked)}')
print(sorted(actual))
PY
cmake -S . -B build
CCACHE_DISABLE=1 cmake --build build --target all
ctest --test-dir build -R '^(branch_gate_tool_tests|dependency_direction_tests|product_automation_command_registry_tests|product_automation_dispatch_tests|product_new_world_menu_action_tests|product_window_input_frame_tests|product_interaction_mode_state_tests|product_room_editor_action_controller_tests|product_ascii_room_activation_tests|product_room_editor_preview_tests|product_save_delete_executor_tests|product_gameplay_tape_runner_tests|product_room_editing_automation_smoke|product_automation_menu_smoke|product_world_setup_smoke|product_save_delete_recover_smoke|product_controller_input_smoke|product_gameplay_tape_smoke)$' --output-on-failure
python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json > /tmp/k15b_dependency_after.json
python3 - <<'PY'
import json

before = json.load(open('/tmp/k15b_dependency_before.json'))
after = json.load(open('/tmp/k15b_dependency_after.json'))
for key in ('source_file_count', 'direct_edge_count', 'directed_pair_count',
            'unordered_pair_count'):
  if after['scan'][key] != before['scan'][key]:
    raise SystemExit(f'{key} changed')
if after['scan']['source_file_count'] != 655:
  raise SystemExit('final source count is not 655')
if before['fan_out']['app']['edge_count'] != 185 or after['fan_out']['app']['edge_count'] != 185:
  raise SystemExit('app fan-out changed')
if after['strongly_connected_components'] or not after['policy']['passed']:
  raise SystemExit('dependency policy failed')
print({'sources': 655, 'edges': 314, 'pairs': [16, 16], 'app': 185})
PY
python3 tools/check_branch_gate.py
git diff --exit-code HEAD -- CMakeLists.txt cmake src/app/iggy3d/automation/Automation.hpp src/app/iggy3d/automation/Automation.cpp src/app/iggy3d/automation/AutomationWorldSetup.cpp src/app/iggy3d/automation/AutomationDispatch.hpp src/app/iggy3d/automation/AutomationDispatch.cpp src/app/iggy3d/automation/AutomationGameplay.cpp src/app/iggy3d/automation/AutomationSaveBrowser.cpp src/app/iggy3d/automation/AutomationSystem.cpp src/app/iggy3d/automation/AutomationControl.hpp src/app/iggy3d/automation/AutomationControl.cpp src/app/iggy3d/automation/AutomationRoomEditing.hpp src/app/iggy3d/automation/AutomationRoomEditing.cpp tests/tools tools/check_branch_gate.py docs/architecture_dependency_policy.json src/runtime src/render src/projection src/runtime/save src/runtime/replay src/app/iggy3d/receipt tests/golden docs/file_specs docs/creative_mode/optimization
git diff --check
```

The focused set must pass 18/18. Rerun row/cardinality/scope checks after any
correction. No broad CTest or app/window launch.

## Single Git Transaction

After every unstaged gate passes, move this card to `done/`. Then make one
permission-bearing request containing:

```sh
git add -A -- docs/branch_gate_approvals.tsv docs/creative_mode/builder_tasks/done/K15b-automation-lookup-deduplication.md src/app/iggy3d/automation/AutomationMetadata.cpp tests/unit/product_automation_command_registry_tests.cpp
python3 tools/check_branch_gate.py --cached
git diff --cached --check
git commit -m "codex: deduplicate automation lookups (K15b)"
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

- K15a prerequisite/clean tree, eight-table, line/folder, focused test,
  dependency, branch, or whitespace premise fails.
- A local row struct or `rows` initializer must change.
- A valid mapping or one of the eight invalid fallbacks cannot remain exact.
- More than one helper/branch is needed, or a public header/API/CMake/other
  implementation must change.
- Lookup/helper/cardinality, exact scope, line, folder, or dependency metrics
  fail.
- All-target, 18/18 tests, row parity, branch gate, protected diff, or
  whitespace fails.
- A pre-existing warning would need cleanup or the Git transaction needs a
  retry.

## Completion Brief

Append the standard completion brief and report both K15 commit hashes, final
ownership/line/folder metrics, eight removed lookup tables, helper/call count,
row parity, all fallback pins, branch approvals, build and 18/18 results,
policy state, protected diff, and transaction result.

Then request aggregate K15 review. There is no next child.
