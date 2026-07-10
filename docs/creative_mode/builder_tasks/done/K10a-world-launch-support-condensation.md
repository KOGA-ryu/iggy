# K10a - World Launch Support Condensation

## Status

READY. K10 batch step 1 of 1. Claim only after K9 aggregate acceptance at
`6820f464`. Authority:
`docs/creative_mode/builder_tasks/blocked/K10-world-launch-support-condensation-plan.md`.

On green completion, move this card to `done/`, perform the single final Git
transaction specified below, and send Reviewer one aggregate K10 brief. A STOP
pauses the batch.

## Goal

Reduce `src/app/iggy3d/world/` from 24 files to 17 by creating one launch
cluster header over two right-sized launch TUs, one world-template pair, and
folding two one-owner state records into `CreativeAuthoringStore.hpp`, with no
behavior or public-symbol change.

## Pre-Edit Gate

Before any edit:

```sh
test "$(find src/app/iggy3d/world -maxdepth 1 -type f | wc -l | tr -d ' ')" = 24
cmake -S . -B build
CCACHE_DISABLE=1 cmake --build build --target all
ctest --test-dir build -R '^(product_god_struct_ownership_coverage_tests|product_world_creation_tests|product_creative_world_launch_tests|product_new_world_menu_action_tests|product_starter_menu_action_tests|product_window_input_frame_tests|product_frontend_router_tests|product_menu_transitions_tests|product_save_delete_executor_tests|product_save_catalog_tests|product_package_session_seed_tests|product_receipt_key_order_tests|dependency_direction_tests)$' --output-on-failure
python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json > /tmp/k10_dependency_before.json
rg -n '#include "app/iggy3d/world/(DefaultWorldTemplate|ProductWorldTemplateOperations|ProductSessionLaunch|ProductNewWorldLaunch|ProductLaunchState|WorldCreationState|WorldSetupState)\.hpp"' src apps tests
rg -n 'defaultProductWorldTemplate|devOverrideProductWorldTemplate|productPackagePathFromOptions|productWorldTemplateFromOptions|clearProductGameplayLaunchState|createProductSessionFromPackage|createProductSession\(|launchProductContinueSave|launchProductLoadSaveSelection|launchProductNewWorld' src apps tests
rg -n 'BG-1137|BG-1224' src/app/iggy3d/world docs/branch_gate_approvals.tsv
git diff --check
```

The focused baseline must pass 13/13. The surveys must match the governing
plan: 24 files, exact old owners, two state headers included only by
`CreativeAuthoringStore.hpp`, BG-1137 on the selected-dungeon branch, and no
BG-1224 yet. Otherwise STOP without edits.

## Exact Final Files And Ownership

Create by unstaged worktree rename, not `git mv`:

- `src/app/iggy3d/world/Launch.hpp` from `ProductSessionLaunch.hpp`;
- `src/app/iggy3d/world/SessionLaunch.cpp` from `ProductSessionLaunch.cpp`;
- `src/app/iggy3d/world/NewWorldLaunch.cpp` from `ProductNewWorldLaunch.cpp`;
- `src/app/iggy3d/world/WorldTemplate.cpp` from
  `ProductWorldTemplateOperations.cpp`;
- `src/app/iggy3d/world/WorldTemplate.hpp` from
  `DefaultWorldTemplate.hpp`.

Delete after absorption:

- `ProductNewWorldLaunch.hpp`;
- `ProductLaunchState.hpp/.cpp`;
- `ProductWorldTemplateOperations.hpp`;
- `DefaultWorldTemplate.cpp`;
- `WorldCreationState.hpp`;
- `WorldSetupState.hpp`.

Final ownership:

- `Launch.hpp`: exactly six reset/session/continue/load/new-world declarations;
- `SessionLaunch.cpp`: reset plus session creation and continue/load-save launch;
- `NewWorldLaunch.cpp`: new-world preparation/launch only;
- `WorldTemplate.hpp/.cpp`: template type plus all four template/path functions;
- `CreativeAuthoringStore.hpp`: unchanged setup/creation state types and store.

No forwarding header, compatibility alias, wrapper, or new public symbol.

## Launch Cluster

`Launch.hpp` includes only `<optional>` and
`runtime/session/Session.hpp`; forward-declare `FrontendState`,
`PackageLoadResult`, `ProductAppOptions`, `ProductAppWindowState`,
`ProductSaveBridgeResult`, `ProductWorldTemplate`, and `WorldSetupDraft`.

Merge the old three header declarations without changing signatures. Move
`clearProductGameplayLaunchState` intact into `SessionLaunch.cpp`; its required
active-room/product-room includes already exist there. Preserve every other
function body and its relative statement order.

`SessionLaunch.cpp` must explicitly include `SaveBridge.hpp`,
`WorldTemplate.hpp`, and `content/PackageLoader.hpp`. `NewWorldLaunch.cpp` must
explicitly include the concrete frontend state, world-setup model, options, and
`WorldTemplate.hpp` headers formerly supplied transitively. Replace internal
old-launch includes with `Launch.hpp`; do not change logic.

## World Template Owner

`WorldTemplate.hpp` contains the unchanged `ProductWorldTemplate` definition,
the two default/override declarations, the two option/path declarations, and a
forward declaration of `ProductAppOptions`. Preserve the needed standard
includes.

`WorldTemplate.cpp` keeps the existing operations implementation and absorbs
both default/override function bodies unchanged. Add
`// branch-gate: BG-1224` immediately before each of the two moved ternaries.
Add exactly one BG-1224 ledger row with the governing plan's text. No other
branch annotation or decision change.

Update BG-1137's ledger path only to
`src/app/iggy3d/world/NewWorldLaunch.cpp`; preserve its description, owner,
date, and source comment.

## State Fold

Move `ProductWorldSetupState` and `ProductWorldCreationState` byte-equivalently
into `CreativeAuthoringStore.hpp` immediately before the store. Remove only the
two old state includes. Preserve type comments, field order/defaults, and the
store's `worldSetup` then `worldCreation` member order.

## Exact Consumer Surface

Only include-path changes are allowed in these production consumers:

- `src/app/iggy3d/AppKernel.cpp`;
- `src/app/iggy3d/ProductAppWindowState.hpp`;
- `src/app/iggy3d/creative/CreativeWorldOperations.cpp`;
- `src/app/iggy3d/menu/ActionHandlers.cpp`;
- `src/app/iggy3d/save/SaveSlotOperations.cpp`;
- `src/app/iggy3d/view/MenuPanelsView.cpp`;
- `src/app/iggy3d/window/FramePresenter.hpp`;
- `src/app/iggy3d/window/Loop.hpp`;
- `src/app/iggy3d/world/Creation.hpp`.

Only include-path changes are allowed in:

- `tests/unit/ProductReceiptTestSupport.hpp`;
- `tests/unit/product_creative_ui_command_receipt_tests.cpp`;
- `tests/unit/product_creative_ui_frame_tests.cpp`;
- `tests/unit/product_creative_ui_projection_receipt_tests.cpp`;
- `tests/unit/product_creative_world_launch_tests.cpp`;
- `tests/unit/product_interaction_mode_state_tests.cpp`;
- `tests/unit/product_mouse_capture_policy_tests.cpp`;
- `tests/unit/product_movement_debug_hud_tests.cpp`;
- `tests/unit/product_receipt_key_order_tests.cpp`;
- `tests/unit/product_starter_menu_action_tests.cpp`;
- `tests/unit/product_top_down_map_overlay_tests.cpp`;
- `tests/unit/product_window_input_frame_tests.cpp`.

In `tests/unit/product_save_delete_executor_tests.cpp`, update only the stale
`ProductSessionLaunch.cpp` owner-name comment to `SessionLaunch.cpp`. No test
assertion/body change is allowed.

## CMake And Mechanical Checkpoint

Replace the five old source entries with exactly:

- `src/app/iggy3d/world/WorldTemplate.cpp`;
- `src/app/iggy3d/world/SessionLaunch.cpp`;
- `src/app/iggy3d/world/NewWorldLaunch.cpp`.

Then prove:

- world folder 24 -> 17;
- `SessionLaunch.cpp` 315-350 lines, `NewWorldLaunch.cpp` 210-240,
  `WorldTemplate.cpp` 50-80, `Launch.hpp` 35-65, `WorldTemplate.hpp` 25-45,
  and `CreativeAuthoringStore.hpp` 155-185;
- all ten existing public functions plus both state types each have one owner;
- all seven old basenames have zero live source/CMake/test references;
- `git diff HEAD --summary --find-renames=40%` recognizes the five worktree
  renames;
- dependency deltas are source `-7`, direct edges `-2`, app fan-out `-2`,
  pair deltas `0/0`, with zero SCCs/violations;
- no retained world pair, behavior body, assertion, golden, save/hash/replay,
  policy, or unrelated file differs.

## Post-Edit Verification

```sh
cmake -S . -B build
CCACHE_DISABLE=1 cmake --build build --target all
ctest --test-dir build -R '^(product_god_struct_ownership_coverage_tests|product_world_creation_tests|product_creative_world_launch_tests|product_new_world_menu_action_tests|product_starter_menu_action_tests|product_window_input_frame_tests|product_frontend_router_tests|product_menu_transitions_tests|product_save_delete_executor_tests|product_save_catalog_tests|product_package_session_seed_tests|product_receipt_key_order_tests|dependency_direction_tests)$' --output-on-failure
python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json > /tmp/k10_dependency_after.json
python3 - <<'PY'
import json

with open('/tmp/k10_dependency_before.json') as stream:
  before = json.load(stream)
with open('/tmp/k10_dependency_after.json') as stream:
  after = json.load(stream)

checks = {
    'source_file_delta': (after['scan']['source_file_count'] - before['scan']['source_file_count'], -7),
    'direct_edge_delta': (after['scan']['direct_edge_count'] - before['scan']['direct_edge_count'], -2),
    'directed_pair_delta': (after['scan']['directed_pair_count'] - before['scan']['directed_pair_count'], 0),
    'unordered_pair_delta': (after['scan']['unordered_pair_count'] - before['scan']['unordered_pair_count'], 0),
    'app_fan_out_delta': (after['fan_out']['app']['edge_count'] - before['fan_out']['app']['edge_count'], -2),
}
for name, (actual, expected) in checks.items():
  if actual != expected:
    raise SystemExit(f'{name}: expected {expected}, got {actual}')
if after['strongly_connected_components']:
  raise SystemExit('K10 introduced an SCC')
if not after['policy']['passed'] or after['policy']['violations']:
  raise SystemExit('K10 violated dependency policy')
print(checks)
PY
test "$(find src/app/iggy3d/world -maxdepth 1 -type f | wc -l | tr -d ' ')" = 17
wc -l src/app/iggy3d/world/Launch.hpp src/app/iggy3d/world/SessionLaunch.cpp src/app/iggy3d/world/NewWorldLaunch.cpp src/app/iggy3d/world/WorldTemplate.hpp src/app/iggy3d/world/WorldTemplate.cpp src/app/iggy3d/creative/CreativeAuthoringStore.hpp
rg -n 'DefaultWorldTemplate|ProductWorldTemplateOperations|ProductSessionLaunch|ProductNewWorldLaunch|ProductLaunchState|WorldCreationState|WorldSetupState' src apps tests CMakeLists.txt cmake
rg -n 'BG-1137|BG-1224' src/app/iggy3d/world docs/branch_gate_approvals.tsv
git diff HEAD --summary --find-renames=40%
git diff HEAD -- tests/golden src/runtime/save src/runtime/replay docs/architecture_dependency_policy.json
python3 tools/check_branch_gate.py
git diff --check
```

The stale-basename and protected-surface commands return no output. The branch
gate passes with only BG-1137 relocation and two BG-1224 comments. Do not run
broad CTest or launch an app/window.

## Single Git Transaction

After every unstaged gate passes, move this card to `done/` using a worktree
operation. Then make exactly one permission-bearing command request containing:

```sh
git add -A -- CMakeLists.txt docs/branch_gate_approvals.tsv docs/creative_mode/builder_tasks/done/K10a-world-launch-support-condensation.md src/app/iggy3d/world src/app/iggy3d/creative/CreativeAuthoringStore.hpp src/app/iggy3d/AppKernel.cpp src/app/iggy3d/ProductAppWindowState.hpp src/app/iggy3d/creative/CreativeWorldOperations.cpp src/app/iggy3d/menu/ActionHandlers.cpp src/app/iggy3d/save/SaveSlotOperations.cpp src/app/iggy3d/view/MenuPanelsView.cpp src/app/iggy3d/window/FramePresenter.hpp src/app/iggy3d/window/Loop.hpp tests/unit/ProductReceiptTestSupport.hpp tests/unit/product_creative_ui_command_receipt_tests.cpp tests/unit/product_creative_ui_frame_tests.cpp tests/unit/product_creative_ui_projection_receipt_tests.cpp tests/unit/product_creative_world_launch_tests.cpp tests/unit/product_interaction_mode_state_tests.cpp tests/unit/product_mouse_capture_policy_tests.cpp tests/unit/product_movement_debug_hud_tests.cpp tests/unit/product_receipt_key_order_tests.cpp tests/unit/product_save_delete_executor_tests.cpp tests/unit/product_starter_menu_action_tests.cpp tests/unit/product_top_down_map_overlay_tests.cpp tests/unit/product_window_input_frame_tests.cpp
python3 tools/check_branch_gate.py --cached
git diff --cached --check
git commit -m "codex: condense world launch support (K10)"
```

Run those four lines in one approved shell transaction. Do not request separate
approval for staging or commit. If any line fails, STOP without an automatic
retry and report the exact staged/unstaged state.

After commit, read-only verification may run without another Git mutation:

```sh
python3 tools/check_branch_gate.py --diff HEAD^..HEAD
git diff --check HEAD^ HEAD
```

## Stop Conditions

- Any pre-edit premise or 13-test baseline fails.
- A retained world pair, caller body, test assertion, public symbol, state
  field/default/order, behavior, receipt, save/hash/replay, or golden must
  change.
- More than the two BG-1224 comments or any branch behavior change is needed.
- Any final file exceeds its bound, old basename remains, rename detection
  fails at 40%, or exact folder/dependency deltas differ.
- Post-edit all-target, 13/13 CTest, branch gate, policy, protection, or
  whitespace check fails.
- The final Git transaction needs a second approval or retry.

## Completion Brief

Append the old->new file map, final symbol/state ownership, preserved launch and
template behavior evidence, state-order proof, BG-1137/BG-1224 handling,
pre/post all-target and 13/13 CTest results, 24->17 folder/line metrics, exact
dependency deltas, rename summary, protected diffs, and single Git-transaction
result. Flag post-acceptance file-spec freshness for Planner; do not edit those
docs.

Commit, then request aggregate K10 review. There is no next child.
