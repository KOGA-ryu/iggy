# K13a - Window Frame Presentation Split

## Status

READY v1.1. Sole K13 batch step. Claim only at accepted K12 repair HEAD
`3fdaa59ad3bd86a8ecb8cf7988ad88d0898de3f3`. Authority:
`docs/creative_mode/builder_tasks/blocked/K13-window-frame-presentation-split-plan.md`.

v1.1 accepts Builder's pre-edit/before-test STOP as a plan correction.
`elapsedMicroseconds` is intentionally file-local in both
`FramePresenter.cpp` and `Loop.cpp`; it is excluded from the cross-file unique
owner map and verified separately below. Resume the existing unstaged partial
K13 implementation. Do not revert, restart, stage, or edit `Loop.cpp`.

On green completion, move this card to `done/`, perform the single final Git
transaction below, and send Reviewer one aggregate K13 brief. A STOP pauses
the batch. Do not split the result into smaller commits.

## Goal

Split the 1,173-line frame presenter into backend orchestration, Vulkan
menu/overlay conversion, and Vulkan gameplay HUD assembly. Fold the two
one-consumer Vulkan state headers into their owning stores so the window and
dependency file counts remain flat. Preserve all behavior and the public
header byte-for-byte.

Metric promise:

- window `.hpp/.cpp` files remain 19;
- implementations 8 -> 10 and headers 11 -> 9;
- dependency source files remain 678;
- `FramePresenter.cpp` at most 300 lines;
- `VulkanOverlayFrames.cpp` at most 340 lines;
- `VulkanGameplayFrame.cpp` at most 710 lines;
- total window physical lines at most 4,300; and
- direct-edge/app-fan-out/directed-pair/unordered-pair deltas all zero.

Expected lower line ranges are not quotas. Never add spacing, comments,
wrapping, or neutral churn to hit a line estimate.

## Accepted Pre-Edit Baseline

Builder already passed this complete baseline before editing under v1.0. The
commands are retained as review evidence. **Do not rerun the source-clean,
8/11 file-count, 4,261-line, 1,173-line, or deleted-header existence checks
against the current partial worktree.** Use the v1.1 Resume Gate below.

```sh
test "$(git rev-parse HEAD)" = 3fdaa59ad3bd86a8ecb8cf7988ad88d0898de3f3
test -z "$(git status --short --untracked-files=no -- src apps tests tools CMakeLists.txt cmake docs/branch_gate_approvals.tsv)"
test "$(find src/app/iggy3d/window -maxdepth 1 -type f \( -name '*.hpp' -o -name '*.cpp' \) | wc -l | tr -d ' ')" = 19
test "$(find src/app/iggy3d/window -maxdepth 1 -type f -name '*.cpp' | wc -l | tr -d ' ')" = 8
test "$(find src/app/iggy3d/window -maxdepth 1 -type f -name '*.hpp' | wc -l | tr -d ' ')" = 11
test "$(find src/app/iggy3d/window -maxdepth 1 -type f \( -name '*.hpp' -o -name '*.cpp' \) -print0 | xargs -0 cat | wc -l | tr -d ' ')" = 4261
test "$(wc -l < src/app/iggy3d/window/FramePresenter.cpp | tr -d ' ')" = 1173
test "$(wc -l < src/app/iggy3d/window/FramePresenter.hpp | tr -d ' ')" = 121
test "$(wc -l < src/app/iggy3d/window/ProductVulkanMenuState.hpp | tr -d ' ')" = 27
test "$(wc -l < src/app/iggy3d/window/ProductVulkanRendererState.hpp | tr -d ' ')" = 13
python3 - <<'PY'
from pathlib import Path

root = Path('.')
checks = {
    'ProductVulkanMenuState.hpp': [
        'src/app/iggy3d/window/FrontendWindowShell.hpp',
    ],
    'ProductVulkanRendererState.hpp': [
        'src/app/iggy3d/window/PresentPathStore.hpp',
    ],
}
for basename, expected in checks.items():
  actual = []
  for path in list((root / 'src').rglob('*')) + list((root / 'apps').rglob('*')) + list((root / 'tests').rglob('*')):
    if path.is_file() and path.suffix in {'.hpp', '.cpp', '.h', '.cc', '.cxx'}:
      if basename in path.read_text(errors='ignore'):
        actual.append(str(path))
  if sorted(actual) != expected:
    raise SystemExit(f'{basename} consumers: {sorted(actual)}')
print(checks)
PY
cmake -S . -B build
CCACHE_DISABLE=1 cmake --build build --target all
ctest --test-dir build -R '^(branch_gate_tool_tests|dependency_direction_tests|product_window_renderer_lifecycle_tests|product_vulkan_room_frame_tests|product_vulkan_menu_frame_tests|product_receipt_key_order_tests|product_god_struct_ownership_coverage_tests|product_creative_ui_projection_receipt_tests|product_creative_ui_frame_tests|product_creative_ui_window_frame_tests|product_creative_wireframe_frame_tests|product_vulkan_pause_overlay_tests|product_vulkan_creative_ui_overlay_tests)$' --output-on-failure
python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json > /tmp/k13_dependency_before.json
python3 tools/check_branch_gate.py
git diff --check
```

The focused baseline must pass 13/13. The dependency JSON must report
678/314/16/16, app fan-out 185, and zero SCCs/violations. Existing compiler
warnings are baseline evidence, not permission to clean them up.

Before editing, retain the current ownership/branch/include survey:

```sh
rg -n '^(constexpr float kVirtualViewport|[A-Za-z_][^;]*\b(recordFirstVulkanSubmitMeasurement|drawableReady|scaleXFor|scaleYFor|scaledOffset|scaledExtent|renderRectFor|appendTextQuads|appendPrimitive|scaledHudOffset|scaledHudExtent|fixedHudFloat|appendGameplayHudText|appendHudPanel|appendInteractionModeHudUi|appendGameplayFeedbackUi|vulkanTopDownMapTitle|appendTopDownMapOverlayUi|appendMovementDebugHudUi|appendNpcBehaviorDebugHudUi|appendPhysicsDebugHudUi|appendRoomEditorHudUi|appendPositionHudUi|appendMovementTuningHudUi|appendDevToolsOverlayUi|appendMapMakerCubePreviewUi|appendMapMakerHudUi|starterMenuFrameInput|applyProductWindowProjectionMetrics|renderCreativeWireframeDebugLineFor|presentProductVulkanFrame|presentProductSdlFrame|buildProductVulkanStarterMenuFrame|refreshProductVulkanMenuFrameInput|buildProductVulkanGameplayFrame|refreshProductVulkanGameplayFrameInput|buildProductCreativeWireframeDebugRenderFrame|appendProductUiOverlay|appendCreativeUiOverlay|appendPauseMenuOverlay|presentProductWindowFrame)\s*\()' src/app/iggy3d/window/FramePresenter.cpp
rg -n '^std::uint64_t elapsedMicroseconds\(' src/app/iggy3d/window/FramePresenter.cpp src/app/iggy3d/window/Loop.cpp
rg -n '\belapsedMicroseconds\(' src/app/iggy3d/window/Loop.cpp
rg -n '^\s*(if|else if|switch)\s*\(|\?.*:' src/app/iggy3d/window/FramePresenter.cpp
rg -n '^#include "render/' src/app/iggy3d/window/FramePresenter.hpp src/app/iggy3d/window/FramePresenter.cpp
rg -n 'ProductVulkan(Menu|Renderer)State|ProductVulkan(Menu|Renderer)State\.hpp' src apps tests CMakeLists.txt cmake
```

All 41 uniquely named mapped definitions must currently be in
`FramePresenter.cpp`. Separately, `FramePresenter.cpp` and `Loop.cpp` must each
have exactly one anonymous-namespace `elapsedMicroseconds` definition, and
`Loop.cpp` must have exactly two calls to its local helper. The public header
must directly include `render/FrameInput.hpp`; the implementation must directly
include `render/FrameInput.hpp` and
`render/debug/DebugHudText.hpp`; and the two old state headers must each have
the sole owner already checked above. A contradiction is a STOP.

## v1.1 Resume Gate

Run this against the preserved partial worktree before resuming. It replaces
the source-clean portion of the accepted baseline; it does not replace final
mechanical/build/test verification.

```sh
test "$(git rev-parse HEAD)" = 3fdaa59ad3bd86a8ecb8cf7988ad88d0898de3f3
test -z "$(git diff --cached --name-only)"
test -f src/app/iggy3d/window/VulkanOverlayFrames.cpp
test -f src/app/iggy3d/window/VulkanGameplayFrame.cpp
git diff --exit-code HEAD -- src/app/iggy3d/window/Loop.cpp src/app/iggy3d/window/FramePresenter.hpp tests cmake/iggy3d_tests.cmake tools/check_branch_gate.py tests/tools/branch_gate_tests.py
python3 - <<'PY'
import subprocess

expected_tracked = {
  'CMakeLists.txt',
  'docs/branch_gate_approvals.tsv',
  'src/app/iggy3d/window/FramePresenter.cpp',
  'src/app/iggy3d/window/FrontendWindowShell.hpp',
  'src/app/iggy3d/window/PresentPathStore.hpp',
  'src/app/iggy3d/window/ProductVulkanMenuState.hpp',
  'src/app/iggy3d/window/ProductVulkanRendererState.hpp',
}
expected_untracked = {
  'src/app/iggy3d/window/VulkanGameplayFrame.cpp',
  'src/app/iggy3d/window/VulkanOverlayFrames.cpp',
}
tracked = set(subprocess.run(
    ['git', 'diff', '--name-only', 'HEAD', '--', 'CMakeLists.txt',
     'docs/branch_gate_approvals.tsv', 'src', 'apps', 'tests', 'cmake'],
    check=True, capture_output=True, text=True).stdout.splitlines())
untracked = set(subprocess.run(
    ['git', 'ls-files', '--others', '--exclude-standard', '--', 'CMakeLists.txt',
     'docs/branch_gate_approvals.tsv', 'src', 'apps', 'tests', 'cmake'],
    check=True, capture_output=True, text=True).stdout.splitlines())
if tracked != expected_tracked or untracked != expected_untracked:
  raise SystemExit(
      f'K13 resume scope differs: tracked={sorted(tracked)}, '
      f'untracked={sorted(untracked)}')
print({'tracked': sorted(tracked), 'untracked': sorted(untracked)})
PY
git diff --check
```

Any staged K13 file, missing partial destination, changed `Loop.cpp`/public
header/test/tool surface, or different production file set is a STOP. Do not
discard the partial implementation to recreate the v1.0 clean baseline.

This gate proves the partial worktree's provenance, not implementation
completion. Continue the exact Steps 1-5 before running final mechanical
gates. In particular, Step 3 still requires both virtual-viewport constants to
move into `VulkanGameplayFrame.cpp`; do not weaken their parity check merely
because the stopped partial tree is not final.

## Exact Files

Create:

- `src/app/iggy3d/window/VulkanOverlayFrames.cpp`;
- `src/app/iggy3d/window/VulkanGameplayFrame.cpp`.

Delete:

- `src/app/iggy3d/window/ProductVulkanMenuState.hpp`;
- `src/app/iggy3d/window/ProductVulkanRendererState.hpp`.

Update only:

- `CMakeLists.txt`;
- `docs/branch_gate_approvals.tsv`;
- `src/app/iggy3d/window/FramePresenter.cpp`;
- `src/app/iggy3d/window/FrontendWindowShell.hpp`;
- `src/app/iggy3d/window/PresentPathStore.hpp`; and
- this card when moved to `done/`.

`FramePresenter.hpp` is protected and must remain byte-for-byte unchanged. No
test, CMake test-registration file, forwarding header, alias, internal header,
planner/spec/optimization document, or unrelated file may change.

## Step 1 - Fold The Two Sole-Owner States

In `FrontendWindowShell.hpp`:

1. remove the `ProductVulkanMenuState.hpp` include;
2. move the complete `ProductVulkanMenuState` definition and its ownership
   comment immediately inside `namespace iggy3d`, before
   `FrontendWindowShell`; and
3. leave the `FrontendWindowShell` definition byte-equivalent, including the
   position of `productVulkanMenu`.

In `PresentPathStore.hpp`:

1. remove the `ProductVulkanRendererState.hpp` include;
2. move the complete `ProductVulkanRendererState` definition and its ownership
   comment immediately inside `namespace iggy3d`, before `PresentPathStore`;
   and
3. leave the `PresentPathStore` definition byte-equivalent, with
   `productVulkanRenderer` first.

Delete both old headers. Do not create forwarding files or touch consumers.

## Step 2 - Create `VulkanOverlayFrames.cpp`

Include `FramePresenter.hpp` as the owner contract. Add exactly this marker
outside function bodies:

```cpp
// branch-gate-relocation: BG-1229 from=src/app/iggy3d/window/FramePresenter.cpp
```

Move these private definitions, complete and source-equivalent, into one
anonymous namespace:

- `drawableReady`;
- `scaleXFor`;
- `scaleYFor`;
- `scaledOffset`;
- `scaledExtent`;
- `renderRectFor`;
- `appendTextQuads`;
- `appendPrimitive`;
- `starterMenuFrameInput`; and
- `renderCreativeWireframeDebugLineFor`.

Move these existing public definitions complete and source-equivalent outside
the anonymous namespace:

- `buildProductVulkanStarterMenuFrame`;
- `refreshProductVulkanMenuFrameInput`;
- `buildProductCreativeWireframeDebugRenderFrame`;
- `appendProductUiOverlay`;
- `appendCreativeUiOverlay`; and
- `appendPauseMenuOverlay`.

Move existing nearby branch-gate comments with their statements. Preserve all
theme, scale, clamp, status, ordering, wireframe conversion, readiness, count,
append, and pointer-refresh truth.

Directly include `render/debug/DebugHudText.hpp` because this owner calls its
layout API. Do not directly include `render/FrameInput.hpp`; the unchanged,
self-contained owner header already supplies that contract.

## Step 3 - Create `VulkanGameplayFrame.cpp`

Include `FramePresenter.hpp` as the owner contract. Add exactly this marker
outside function bodies:

```cpp
// branch-gate-relocation: BG-1230 from=src/app/iggy3d/window/FramePresenter.cpp
```

Move these private constants/definitions, complete and source-equivalent,
into one anonymous namespace:

- `kVirtualViewportWidth` and `kVirtualViewportHeight`;
- `scaledHudOffset` and `scaledHudExtent`;
- `fixedHudFloat`;
- `appendGameplayHudText`;
- `appendHudPanel`;
- `appendInteractionModeHudUi`;
- `appendGameplayFeedbackUi`;
- `vulkanTopDownMapTitle`;
- `appendTopDownMapOverlayUi`;
- `appendMovementDebugHudUi`;
- `appendNpcBehaviorDebugHudUi`;
- `appendPhysicsDebugHudUi`;
- `appendRoomEditorHudUi`;
- `appendPositionHudUi`;
- `appendMovementTuningHudUi`;
- `appendDevToolsOverlayUi`;
- `appendMapMakerCubePreviewUi`; and
- `appendMapMakerHudUi`.

Move these existing public definitions complete and source-equivalent outside
the anonymous namespace:

- `buildProductVulkanGameplayFrame`; and
- `refreshProductVulkanGameplayFrameInput`.

Move existing nearby branch-gate comments with their statements. Preserve
every virtual coordinate, extent, color, text, row cap, branch, append order,
Creative-overlay suppression rule, count, and pointer refresh.

Directly include `render/debug/DebugHudText.hpp`; do not directly include
`render/FrameInput.hpp`.

## Step 4 - Reduce `FramePresenter.cpp`

Leave exactly these five uniquely named definitions in this implementation:

- `recordFirstVulkanSubmitMeasurement`;
- `applyProductWindowProjectionMetrics`;
- `presentProductVulkanFrame`;
- `presentProductSdlFrame`; and
- `presentProductWindowFrame`.

The first four remain private in the anonymous namespace; the last remains the
public definition. In addition, retain FramePresenter's file-local
`elapsedMicroseconds` helper exactly once and source-equivalently, but exclude
that common local name from the cross-file unique ownership map. Keep every
function source-equivalent and preserve the
Vulkan, starter-menu fallback, SDL, projection metric, overlay, submit,
receipt, and top-level dispatch ordering.

Do not edit or rename the separate anonymous-namespace
`elapsedMicroseconds` in `Loop.cpp`. That file must remain byte-for-byte equal
to accepted HEAD, with one local definition and its existing two calls.

Remove includes only after their last direct use moves. In particular,
`FramePresenter.cpp` must no longer directly include either
`render/FrameInput.hpp` or `render/debug/DebugHudText.hpp`.

Do not move backend submit logic into either frame-builder concern. Do not
introduce a request adapter, shared internal header, helper duplication, or a
call from one new concern implementation into the other beyond existing public
functions declared by `FramePresenter.hpp`.

## Step 5 - CMake And Branch Ledger

In `CMakeLists.txt`, place exactly one entry for each new implementation next
to `FramePresenter.cpp`:

- `src/app/iggy3d/window/VulkanOverlayFrames.cpp`;
- `src/app/iggy3d/window/VulkanGameplayFrame.cpp`.

Do not change target membership or `cmake/iggy3d_tests.cmake`.

Append exactly these approvals to `docs/branch_gate_approvals.tsv` using the
existing TSV columns:

- BG-1229, destination `src/app/iggy3d/window/VulkanOverlayFrames.cpp`, exact
  Vulkan menu/overlay/wireframe branch-set relocation from
  `FramePresenter.cpp`;
- BG-1230, destination `src/app/iggy3d/window/VulkanGameplayFrame.cpp`, exact
  Vulkan gameplay HUD branch-set relocation from `FramePresenter.cpp`.

Do not edit `tools/check_branch_gate.py` or its oracle.

## Mechanical Ownership And Parity Gates

Run all of the following before staging:

```sh
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

root = Path('src/app/iggy3d/window')

def at_head(path: str) -> str:
  return subprocess.run(
      ['git', 'show', f'HEAD:{path}'], check=True,
      capture_output=True, text=True).stdout

def definition_block(text: str, name: str) -> str:
  matches = list(re.finditer(
      rf'(?m)^[A-Za-z_][^\n]*\b{re.escape(name)}\s*\(', text))
  if len(matches) != 1:
    raise SystemExit(f'{name}: expected one definition start, got {len(matches)}')
  start = matches[0].start()
  brace = text.find('{', matches[0].end())
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
          return text[start:i + 1].strip()
    i += 1
  raise SystemExit(f'{name}: closing brace not found')

old = at_head('src/app/iggy3d/window/FramePresenter.cpp')
owners = {
  'src/app/iggy3d/window/FramePresenter.cpp': '''
recordFirstVulkanSubmitMeasurement applyProductWindowProjectionMetrics
presentProductVulkanFrame
presentProductSdlFrame presentProductWindowFrame
'''.split(),
  'src/app/iggy3d/window/VulkanOverlayFrames.cpp': '''
drawableReady scaleXFor scaleYFor scaledOffset scaledExtent renderRectFor
appendTextQuads appendPrimitive starterMenuFrameInput
renderCreativeWireframeDebugLineFor buildProductVulkanStarterMenuFrame
refreshProductVulkanMenuFrameInput buildProductCreativeWireframeDebugRenderFrame
appendProductUiOverlay appendCreativeUiOverlay appendPauseMenuOverlay
'''.split(),
  'src/app/iggy3d/window/VulkanGameplayFrame.cpp': '''
scaledHudOffset scaledHudExtent fixedHudFloat appendGameplayHudText appendHudPanel
appendInteractionModeHudUi appendGameplayFeedbackUi vulkanTopDownMapTitle
appendTopDownMapOverlayUi appendMovementDebugHudUi appendNpcBehaviorDebugHudUi
appendPhysicsDebugHudUi appendRoomEditorHudUi appendPositionHudUi
appendMovementTuningHudUi appendDevToolsOverlayUi appendMapMakerCubePreviewUi
appendMapMakerHudUi buildProductVulkanGameplayFrame
refreshProductVulkanGameplayFrameInput
'''.split(),
}

all_names = [name for names in owners.values() for name in names]
if len(all_names) != len(set(all_names)):
  raise SystemExit('duplicate symbol in K13 owner map')
for owner, names in owners.items():
  current = Path(owner).read_text()
  for name in names:
    if definition_block(current, name) != definition_block(old, name):
      raise SystemExit(f'{name}: source changed while moving to {owner}')

definition_owners = {name: [] for name in all_names}
for path in root.glob('*.cpp'):
  text = path.read_text()
  for name in all_names:
    if re.search(rf'(?m)^[A-Za-z_][^\n]*\b{re.escape(name)}\s*\(', text):
      definition_owners[name].append(str(path))
expected = {
    name: [owner]
    for owner, names in owners.items()
    for name in names
}
if definition_owners != expected:
  raise SystemExit(f'definition owners differ: {definition_owners}')

present_path = 'src/app/iggy3d/window/FramePresenter.cpp'
present = Path(present_path).read_text()
if definition_block(present, 'elapsedMicroseconds') != definition_block(
    old, 'elapsedMicroseconds'):
  raise SystemExit('FramePresenter elapsedMicroseconds changed')
if len(re.findall(
    r'(?m)^std::uint64_t elapsedMicroseconds\s*\(', present)) != 1:
  raise SystemExit('FramePresenter must retain one local elapsedMicroseconds definition')

loop_path = 'src/app/iggy3d/window/Loop.cpp'
loop = Path(loop_path).read_text()
old_loop = at_head(loop_path)
if loop != old_loop:
  raise SystemExit('Loop.cpp changed during K13')
if len(re.findall(
    r'(?m)^std::uint64_t elapsedMicroseconds\s*\(', loop)) != 1:
  raise SystemExit('Loop.cpp must retain one local elapsedMicroseconds definition')
if len(re.findall(r'\belapsedMicroseconds\s*\(', loop)) != 3:
  raise SystemExit('Loop.cpp must retain one local definition plus two calls')

gameplay = Path('src/app/iggy3d/window/VulkanGameplayFrame.cpp').read_text()
for declaration in (
    'constexpr float kVirtualViewportWidth = 1280.0F;',
    'constexpr float kVirtualViewportHeight = 720.0F;',
):
  if old.count(declaration) != 1 or gameplay.count(declaration) != 1:
    raise SystemExit(f'constant moved incorrectly: {declaration}')
print(f'{len(all_names)} uniquely named source-equivalent definitions with exact owners')
print('two independent local elapsedMicroseconds helpers preserved')
PY
python3 - <<'PY'
from pathlib import Path
import subprocess

def at_head(path: str) -> str:
  return subprocess.run(
      ['git', 'show', f'HEAD:{path}'], check=True,
      capture_output=True, text=True).stdout

def struct_block(text: str, name: str) -> str:
  start = text.index(f'struct {name}')
  end = text.index('};', start) + 2
  return text[start:end]

checks = [
  ('ProductVulkanMenuState',
   'src/app/iggy3d/window/ProductVulkanMenuState.hpp',
   'src/app/iggy3d/window/FrontendWindowShell.hpp'),
  ('ProductVulkanRendererState',
   'src/app/iggy3d/window/ProductVulkanRendererState.hpp',
   'src/app/iggy3d/window/PresentPathStore.hpp'),
  ('FrontendWindowShell',
   'src/app/iggy3d/window/FrontendWindowShell.hpp',
   'src/app/iggy3d/window/FrontendWindowShell.hpp'),
  ('PresentPathStore',
   'src/app/iggy3d/window/PresentPathStore.hpp',
   'src/app/iggy3d/window/PresentPathStore.hpp'),
]
for name, old_path, new_path in checks:
  old = struct_block(at_head(old_path), name)
  new = struct_block(Path(new_path).read_text(), name)
  if old != new:
    raise SystemExit(f'{name}: struct body/order/defaults changed')
print('four state/store structs are byte-equivalent')
PY
git diff --exit-code HEAD -- src/app/iggy3d/window/FramePresenter.hpp
test "$(find src/app/iggy3d/window -maxdepth 1 -type f \( -name '*.hpp' -o -name '*.cpp' \) | wc -l | tr -d ' ')" = 19
test "$(find src/app/iggy3d/window -maxdepth 1 -type f -name '*.cpp' | wc -l | tr -d ' ')" = 10
test "$(find src/app/iggy3d/window -maxdepth 1 -type f -name '*.hpp' | wc -l | tr -d ' ')" = 9
test "$(find src/app/iggy3d/window -maxdepth 1 -type f \( -name '*.hpp' -o -name '*.cpp' \) -print0 | xargs -0 cat | wc -l | tr -d ' ')" -le 4300
wc -l src/app/iggy3d/window/FramePresenter.cpp src/app/iggy3d/window/VulkanOverlayFrames.cpp src/app/iggy3d/window/VulkanGameplayFrame.cpp
test "$(wc -l < src/app/iggy3d/window/FramePresenter.cpp | tr -d ' ')" -le 300
test "$(wc -l < src/app/iggy3d/window/VulkanOverlayFrames.cpp | tr -d ' ')" -le 340
test "$(wc -l < src/app/iggy3d/window/VulkanGameplayFrame.cpp | tr -d ' ')" -le 710
rg -n 'ProductVulkan(Menu|Renderer)State\.hpp' src apps tests CMakeLists.txt cmake; test $? -eq 1
test ! -e src/app/iggy3d/window/ProductVulkanMenuState.hpp
test ! -e src/app/iggy3d/window/ProductVulkanRendererState.hpp
test "$(rg -c 'src/app/iggy3d/window/FramePresenter.cpp' CMakeLists.txt)" = 1
test "$(rg -c 'src/app/iggy3d/window/VulkanOverlayFrames.cpp' CMakeLists.txt)" = 1
test "$(rg -c 'src/app/iggy3d/window/VulkanGameplayFrame.cpp' CMakeLists.txt)" = 1
rg -n 'branch-gate-relocation: BG-1229 from=src/app/iggy3d/window/FramePresenter.cpp' src/app/iggy3d/window/VulkanOverlayFrames.cpp
rg -n 'branch-gate-relocation: BG-1230 from=src/app/iggy3d/window/FramePresenter.cpp' src/app/iggy3d/window/VulkanGameplayFrame.cpp
test "$(rg -c '^BG-12(29|30)\t' docs/branch_gate_approvals.tsv)" = 2
python3 - <<'PY'
from pathlib import Path
import subprocess

expected = {
  'CMakeLists.txt',
  'docs/branch_gate_approvals.tsv',
  'src/app/iggy3d/window/FramePresenter.cpp',
  'src/app/iggy3d/window/FrontendWindowShell.hpp',
  'src/app/iggy3d/window/PresentPathStore.hpp',
  'src/app/iggy3d/window/ProductVulkanMenuState.hpp',
  'src/app/iggy3d/window/ProductVulkanRendererState.hpp',
  'src/app/iggy3d/window/VulkanOverlayFrames.cpp',
  'src/app/iggy3d/window/VulkanGameplayFrame.cpp',
}
tracked = set(subprocess.run(
    ['git', 'diff', '--name-only', 'HEAD', '--', 'CMakeLists.txt', 'cmake',
     'docs/branch_gate_approvals.tsv', 'src', 'apps', 'tests'],
    check=True, capture_output=True, text=True).stdout.splitlines())
untracked = set(subprocess.run(
    ['git', 'ls-files', '--others', '--exclude-standard', '--', 'CMakeLists.txt',
     'cmake', 'docs/branch_gate_approvals.tsv', 'src', 'apps', 'tests'],
    check=True, capture_output=True, text=True).stdout.splitlines())
actual = tracked | untracked
if actual != expected:
  raise SystemExit(f'K13 production scope: expected {sorted(expected)}, got {sorted(actual)}')
print(sorted(actual))
PY
python3 - <<'PY'
from pathlib import Path
import subprocess
import tools.check_branch_gate as gate

diff = subprocess.run(
    ['git', 'diff', '--unified=0'], check=True,
    capture_output=True, text=True).stdout
for path in (
    'src/app/iggy3d/window/VulkanOverlayFrames.cpp',
    'src/app/iggy3d/window/VulkanGameplayFrame.cpp',
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
    print(f'{finding.path}:{finding.new_line}: {finding.statement}: {finding.reason}')
  raise SystemExit('combined unstaged branch gate failed')
print('combined unstaged branch gate: ok')
PY
```

Do not pad a file to satisfy an estimate. If natural source exceeds a hard
upper bound, STOP and report the ownership shape.

## Post-Edit Verification

```sh
cmake -S . -B build
CCACHE_DISABLE=1 cmake --build build --target all
ctest --test-dir build -R '^(branch_gate_tool_tests|dependency_direction_tests|product_window_renderer_lifecycle_tests|product_vulkan_room_frame_tests|product_vulkan_menu_frame_tests|product_receipt_key_order_tests|product_god_struct_ownership_coverage_tests|product_creative_ui_projection_receipt_tests|product_creative_ui_frame_tests|product_creative_ui_window_frame_tests|product_creative_wireframe_frame_tests|product_vulkan_pause_overlay_tests|product_vulkan_creative_ui_overlay_tests)$' --output-on-failure
python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json > /tmp/k13_dependency_after.json
python3 - <<'PY'
import json

with open('/tmp/k13_dependency_before.json') as stream:
  before = json.load(stream)
with open('/tmp/k13_dependency_after.json') as stream:
  after = json.load(stream)

checks = {
    'source_file_delta': (after['scan']['source_file_count'] - before['scan']['source_file_count'], 0),
    'direct_edge_delta': (after['scan']['direct_edge_count'] - before['scan']['direct_edge_count'], 0),
    'directed_pair_delta': (after['scan']['directed_pair_count'] - before['scan']['directed_pair_count'], 0),
    'unordered_pair_delta': (after['scan']['unordered_pair_count'] - before['scan']['unordered_pair_count'], 0),
    'app_fan_out_delta': (after['fan_out']['app']['edge_count'] - before['fan_out']['app']['edge_count'], 0),
}
for name, (actual, expected) in checks.items():
  if actual != expected:
    raise SystemExit(f'{name}: expected {expected}, got {actual}')
if after['scan']['source_file_count'] != 678:
  raise SystemExit(f"source count: {after['scan']['source_file_count']}")
if after['scan']['direct_edge_count'] != 314:
  raise SystemExit(f"edge count: {after['scan']['direct_edge_count']}")
if after['scan']['directed_pair_count'] != 16 or after['scan']['unordered_pair_count'] != 16:
  raise SystemExit('pair counts changed')
if after['fan_out']['app']['edge_count'] != 185:
  raise SystemExit(f"app fan-out: {after['fan_out']['app']['edge_count']}")
if after['strongly_connected_components']:
  raise SystemExit('K13 introduced an SCC')
if not after['policy']['passed'] or after['policy']['violations']:
  raise SystemExit('K13 violated dependency policy')
print(checks)
PY
python3 - <<'PY'
from pathlib import Path

expected = {
  'src/app/iggy3d/window/FramePresenter.hpp': {'render/FrameInput.hpp'},
  'src/app/iggy3d/window/FramePresenter.cpp': set(),
  'src/app/iggy3d/window/VulkanOverlayFrames.cpp': {'render/debug/DebugHudText.hpp'},
  'src/app/iggy3d/window/VulkanGameplayFrame.cpp': {'render/debug/DebugHudText.hpp'},
}
for path, wanted in expected.items():
  includes = {
      line.removeprefix('#include "').removesuffix('"')
      for line in Path(path).read_text().splitlines()
      if line.startswith('#include "render/')
  }
  if includes != wanted:
    raise SystemExit(f'{path} render includes: expected {wanted}, got {includes}')
print(expected)
PY
git diff --exit-code HEAD -- tests cmake/iggy3d_tests.cmake tools/check_branch_gate.py tests/tools/branch_gate_tests.py src/render src/projection src/runtime src/app/iggy3d/receipt src/runtime/save src/runtime/replay docs/architecture_dependency_policy.json docs/file_specs docs/creative_mode/optimization
git diff --check
```

The focused set must pass 13/13. Protected-surface output must be empty. Do not
run broad CTest or launch an app/window.

## Single Git Transaction

After every unstaged gate passes, move this card to `done/` with an ordinary
worktree move. Then make exactly one permission-bearing command request
containing all four lines:

```sh
git add -A -- CMakeLists.txt docs/branch_gate_approvals.tsv docs/creative_mode/builder_tasks/ready/K13a-window-frame-presentation-split.md docs/creative_mode/builder_tasks/done/K13a-window-frame-presentation-split.md src/app/iggy3d/window/FramePresenter.cpp src/app/iggy3d/window/FrontendWindowShell.hpp src/app/iggy3d/window/PresentPathStore.hpp src/app/iggy3d/window/ProductVulkanMenuState.hpp src/app/iggy3d/window/ProductVulkanRendererState.hpp src/app/iggy3d/window/VulkanOverlayFrames.cpp src/app/iggy3d/window/VulkanGameplayFrame.cpp
python3 tools/check_branch_gate.py --cached
git diff --cached --check
git commit -m "codex: split window frame presentation (K13)"
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

- Any accepted baseline or v1.1 resume-gate premise, file/line/consumer,
  all-target, 13-test, policy, branch-gate, or whitespace check fails.
- A state header has another consumer, a moved definition cannot remain
  source-equivalent, or a symbol does not fit the exact owner map.
- The FramePresenter-local timing helper changes, `Loop.cpp` changes, or its
  separate local helper no longer has exactly two callers.
- The work needs a new header, public API change, duplicate helper, changed
  branch/statement order, UI/render behavior re-pin, or test change.
- `FramePresenter.hpp`, a protected concern, architecture policy, branch-gate
  tool/oracle, or unrelated file must change.
- Any old state basename remains, exact file/line/owner counts fail, total
  window LOC exceeds 4,300, or a target implementation exceeds its hard bound.
- Dependency source/direct-edge/pair/fan-out deltas differ from zero, or an
  SCC/policy violation appears.
- Post-edit all-target, 13/13 CTest, source/state parity, branch relocation,
  protected diff, or whitespace check fails.
- A pre-existing compiler warning would need cleanup.
- The final Git transaction needs a second approval/retry.

## Completion Brief

Report:

- commit hash;
- exact old -> new ownership for all 41 uniquely named definitions and two
  constants, plus separate parity for both file-local timing helpers;
- preserved Vulkan/gameplay/menu/overlay/SDL submit and append ordering;
- byte-unchanged `FramePresenter.hpp` proof;
- exact state-header folds and unchanged struct/member/default proof;
- final line counts and window 19 / 10 cpp / 9 hpp / total LOC facts;
- BG-1229/BG-1230 exact relocation results and unchanged branch-tool oracle;
- pre/post all-target and 13/13 focused CTest results;
- exact dependency deltas and final 678/314/16/16/app-185 policy state;
- protected diff and one Git-transaction result; and
- post-acceptance file-spec/Cartographer freshness items for Planner.

Commit, then request aggregate K13 review. There is no next child.
