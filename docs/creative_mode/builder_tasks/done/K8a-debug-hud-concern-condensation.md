# K8a - Debug HUD Concern Condensation

## Status

READY v1.3, REISSUED AFTER PRE-EDIT ENVIRONMENT STOP. K8 batch step 1 of 1.
Claim only after K7 aggregate acceptance at `fcf2154a`. Authority:
`docs/creative_mode/builder_tasks/blocked/K8-debug-hud-concern-condensation-plan.md`.

On green completion, commit this card, move it to `done/`, and send Reviewer one
aggregate K8 brief. A STOP pauses the batch.

## Premise Corrections

- v1.1 authorized include-only receipt consumers, including
  `SaveStateFields.cpp`, despite the general receipt-body firewall.
- v1.2 adds the lower `DebugHudState.hpp` boundary, keeps builder APIs out of
  store include closures, authorizes the required direct builder include in
  `ProjectionRefresh.cpp`, restores pre/post all-target builds, uses dependency
  deltas from a fresh baseline, and records post-acceptance doc re-anchors.
- v1.3 authorizes `CCACHE_DISABLE=1` for the pre/post all-target commands after
  the default ccache launcher failed to write its sandbox-external temporary
  directory. Planner ran the exact override successfully through 100%; caching
  is the only changed execution property.

No K8 source/test edit existed at reissue.

## Goal

Replace three tiny transient HUD pairs and three one-includer state headers with
one state header plus one builder pair, preserving every public symbol and all
behavior.

## Pre-Edit Gate

Before any edit:

```sh
test "$(find src/app/iggy3d/debug -maxdepth 1 -type f | wc -l | tr -d ' ')" = 16
cmake -S . -B build
CCACHE_DISABLE=1 cmake --build build --target all
python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json > /tmp/k8_dependency_before.json
rg -l '#include "app/iggy3d/debug/(InteractionModeHud|NpcBehaviorDebugHud|PhysicsDebugHud)\.hpp"' src apps tests | sort
rg -n 'buildInteractionModeHud|buildNpcBehaviorDebugHud|buildPhysicsDebugHud' src apps tests
```

The include survey must match the parent plan's 23-hit classification and
production builder calls must remain confined to `ProjectionRefresh.cpp`.
Otherwise STOP without edits.

## Exact Files

Create:

- `src/app/iggy3d/debug/DebugHudState.hpp`
- `src/app/iggy3d/debug/DebugHudPanels.hpp`
- `src/app/iggy3d/debug/DebugHudPanels.cpp`

Delete:

- `src/app/iggy3d/debug/InteractionModeHud.hpp`
- `src/app/iggy3d/debug/InteractionModeHud.cpp`
- `src/app/iggy3d/debug/NpcBehaviorDebugHud.hpp`
- `src/app/iggy3d/debug/NpcBehaviorDebugHud.cpp`
- `src/app/iggy3d/debug/PhysicsDebugHud.hpp`
- `src/app/iggy3d/debug/PhysicsDebugHud.cpp`
- `src/app/iggy3d/debug/DevCollisionOverlayState.hpp`
- `src/app/iggy3d/debug/NpcBehaviorDebugHudState.hpp`
- `src/app/iggy3d/debug/TopDownMapState.hpp`

Update ownership/build/gate wiring:

- `src/app/iggy3d/debug/DebugHudStore.hpp`
- `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
- `CMakeLists.txt`
- `docs/branch_gate_approvals.tsv`

Replace old HUD-header includes with `DebugHudState.hpp` only in:

- `src/app/iggy3d/ReceiptBuilder.cpp`
- `src/app/iggy3d/gameplay/ProjectionRefresh.hpp`
- `src/app/iggy3d/input/InputDeviceStore.hpp`
- `src/app/iggy3d/receipt/CreativePickWireframeFields.cpp`
- `src/app/iggy3d/receipt/CreativeReceiptRecording.cpp`
- `src/app/iggy3d/receipt/CreativeUiFields.cpp`
- `src/app/iggy3d/receipt/DebugHudFields.cpp`
- `src/app/iggy3d/receipt/FeedbackSurfaceAutomationVulkanFields.cpp`
- `src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp`
- `src/app/iggy3d/receipt/GameplayRuntimeMovementFields.cpp`
- `src/app/iggy3d/receipt/GameplaySceneStateFields.cpp`
- `src/app/iggy3d/receipt/PhysicsReceiptRecording.cpp`
- `src/app/iggy3d/receipt/ReceiptFields.hpp`
- `src/app/iggy3d/receipt/SaveStateFields.cpp`
- `src/app/iggy3d/receipt/TailFields.cpp`
- `src/app/iggy3d/view/DebugHudView.cpp`

Replace old HUD-header includes with `DebugHudPanels.hpp` only in:

- `tests/unit/product_interaction_mode_hud_tests.cpp`
- `tests/unit/product_npc_behavior_debug_hud_tests.cpp`
- `tests/unit/product_physics_debug_hud_tests.cpp`

No other source/test/doc is allowed. ASCII and cartographer work remain inert
and unstaged.

## Required State Boundary

`DebugHudState.hpp` contains, unchanged:

1. `InteractionModeHud`;
2. `NpcBehaviorDebugHudLine` and `NpcBehaviorDebugHud`;
3. `PhysicsDebugHudLine` and `PhysicsDebugHud`;
4. `ProductTopDownMapState`;
5. `ProductDevCollisionOverlayState`;
6. `ProductNpcBehaviorDebugHudState`.

It includes only required standard headers plus `GameplayFeedback.hpp`. It has
no builder declaration, `DebugProjection`, `ProductInteractionMode`, request
type, or store composition.

`DebugHudStore.hpp` includes `DebugHudState.hpp` plus `PositionHud.hpp` and
retains exactly its current five members/order/defaults. It must not include
`DebugHudPanels.hpp`.

`InputDeviceStore.hpp` replaces `InteractionModeHud.hpp` with
`DebugHudState.hpp`; it must not include `DebugHudPanels.hpp` or
`DebugHudStore.hpp`.

## Required Builder Boundary

`DebugHudPanels.hpp` includes `DebugHudState.hpp` and `InteractionMode.hpp`. It
owns only `InteractionModeHudRequest`, a forward declaration of
`DebugProjectionResult`, and the three unchanged builder declarations.

`DebugHudPanels.cpp` owns the three existing implementations in interaction,
NPC, physics order. It includes `DebugProjection.hpp` once and uses one
anonymous namespace. Rename private colliding helpers by concern; no public or
output behavior changes. Preserve emitted HUD line order, not literal source
line positions.

`ProjectionRefresh.cpp` adds one direct `DebugHudPanels.hpp` include because it
is the sole production builder caller. No body line may change.

Movement, position, and top-down stay separate and byte-unchanged because they
are already right-sized and have distinct proof/projection/overlay inputs.

## Consumer And CMake Migration

Every listed state-only consumer replaces one or both deleted HUD headers with
one `DebugHudState.hpp` include. Every listed direct test includes
`DebugHudPanels.hpp`; assertions remain byte-identical.

Every listed receipt path, including `SaveStateFields.cpp`, is authorized for
include replacement only. No receipt body line changes.

Replace the three old CMake implementation entries with exactly one
`src/app/iggy3d/debug/DebugHudPanels.cpp`. Do not change test declarations or
other target membership.

## Branch-Gate Relocation

- Change ledger paths for `BG-1064` and `BG-1108` to
  `src/app/iggy3d/debug/DebugHudPanels.cpp`.
- Add `BG-1223` there with description
  `relocated NPC behavior debug HUD availability and visibility policy`, owner
  `codex`, date `2026-07-09`.
- Keep `BG-1064`/`BG-1108` adjacent to their moved branches and place BG-1223
  within the required window of every moved NPC builder branch.
- Add or remove no branch.

BG-1223 exists because the diff-based checker treats relocated existing NPC
branches as additions. One ID intentionally covers that one builder's coherent
availability/visibility cascade; it is not a behavior-policy expansion.

## Mechanical Checkpoint

- Debug folder is 16 -> 10 files.
- `DebugHudState.hpp` is 120-180 lines; `DebugHudPanels.hpp` is 25-50;
  `DebugHudPanels.cpp` is 200-300; `DebugHudStore.hpp` is 15-35.
- State/store headers contain no `DebugHudPanels` include; state contains no
  projection/request/builder surface.
- The three public builders and six state groups have one owner each.
- Nine deleted paths and their basenames have no live source/test/CMake hit.
- CMake has one `DebugHudPanels.cpp` entry and no old implementation entry.
- Against `/tmp/k8_dependency_before.json`: source files -6, direct edges -1,
  app fan-out edges -1, directed/unordered pairs unchanged, zero SCCs and policy
  violations.
- Listed receipt/test consumers have include-only diffs; protected HUD owners,
  receipt bodies, save/hash/replay, goldens, and dependency policy are unchanged.

## Post-Edit Verification

```sh
cmake -S . -B build
CCACHE_DISABLE=1 cmake --build build --target all
ctest --test-dir build -R '^(product_interaction_mode_hud_tests|product_npc_behavior_debug_hud_tests|product_physics_debug_hud_tests|product_movement_debug_hud_tests|product_position_hud_tests|product_top_down_map_overlay_tests|product_menu_transitions_tests|product_vulkan_room_frame_tests|product_receipt_key_order_tests|product_god_struct_ownership_coverage_tests|dependency_direction_tests)$' --output-on-failure
python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json > /tmp/k8_dependency_after.json
python3 - <<'PY'
import json

with open('/tmp/k8_dependency_before.json') as stream:
  before = json.load(stream)
with open('/tmp/k8_dependency_after.json') as stream:
  after = json.load(stream)

checks = {
    'source_file_delta': (after['scan']['source_file_count'] - before['scan']['source_file_count'], -6),
    'direct_edge_delta': (after['scan']['direct_edge_count'] - before['scan']['direct_edge_count'], -1),
    'directed_pair_delta': (after['scan']['directed_pair_count'] - before['scan']['directed_pair_count'], 0),
    'unordered_pair_delta': (after['scan']['unordered_pair_count'] - before['scan']['unordered_pair_count'], 0),
    'app_fan_out_delta': (after['fan_out']['app']['edge_count'] - before['fan_out']['app']['edge_count'], -1),
}
for name, (actual, expected) in checks.items():
  if actual != expected:
    raise SystemExit(f'{name}: expected {expected}, got {actual}')
if after['strongly_connected_components']:
  raise SystemExit('K8 introduced an SCC')
if not after['policy']['passed'] or after['policy']['violations']:
  raise SystemExit('K8 violated dependency policy')
print(checks)
PY
test "$(find src/app/iggy3d/debug -maxdepth 1 -type f | wc -l | tr -d ' ')" = 10
wc -l src/app/iggy3d/debug/DebugHudState.hpp src/app/iggy3d/debug/DebugHudPanels.hpp src/app/iggy3d/debug/DebugHudPanels.cpp src/app/iggy3d/debug/DebugHudStore.hpp
rg -n 'InteractionModeHud\.hpp|NpcBehaviorDebugHud\.hpp|PhysicsDebugHud\.hpp|DevCollisionOverlayState\.hpp|NpcBehaviorDebugHudState\.hpp|TopDownMapState\.hpp|InteractionModeHud\.cpp|NpcBehaviorDebugHud\.cpp|PhysicsDebugHud\.cpp' src apps tests CMakeLists.txt cmake
rg -n 'DebugHudPanels' src/app/iggy3d/debug/DebugHudState.hpp src/app/iggy3d/debug/DebugHudStore.hpp src/app/iggy3d/input/InputDeviceStore.hpp
rg -n 'DebugProjection|build(InteractionMode|NpcBehavior|Physics)DebugHud|InteractionModeHudRequest' src/app/iggy3d/debug/DebugHudState.hpp
rg -n 'DebugHudPanels\.cpp' CMakeLists.txt cmake
rg -n 'BG-1064|BG-1108|BG-1223' src/app/iggy3d/debug/DebugHudPanels.cpp docs/branch_gate_approvals.tsv
git diff -- tests/golden src/runtime/save src/runtime/replay docs/architecture_dependency_policy.json
git diff --check
```

The two forbidden-layer greps return no output. The stale-path grep returns no
output. The CMake grep returns one source entry. Golden/save/replay/policy diff
is empty. `CCACHE_DISABLE=1` changes only cache enablement; do not alter CMake's
launcher, compiler, flags, or target graph. Do not run broad CTest or launch an
app/window.

After committing, run:

```sh
python3 tools/check_branch_gate.py --diff HEAD^..HEAD
```

## Stop Conditions

- Pre-edit cache-disabled all-target build, dependency policy, 16-file premise,
  include survey, or sole production-builder-caller premise fails.
- A store/state-only consumer needs `DebugHudPanels.hpp`, or state needs a
  builder/projection/request dependency.
- A consumer needs more than its exact include migration, except the allowed
  `ProjectionRefresh.cpp` include addition.
- A public type/function/default/status/tone/emitted-line behavior or test
  assertion changes; a forwarding alias/header or duplicate owner appears.
- `DebugHudPanels.cpp` exceeds 300 lines or absorbs another HUD concern.
- Branch-gate relocation needs behavior changes.
- Post-edit cache-disabled all-target build, focused CTests, 16->10 folder
  count, exact deltas, SCC, or policy checks fail.

## Completion Brief

Append created/deleted files, state/builder layering proof, exact include-only
consumer list, unchanged API/behavior evidence, branch-gate rationale, pre/post
all-target results, focused CTests, folder/line/delta metrics, and protection
diffs. Explicitly flag two post-acceptance duties: Planner re-anchors
`perception-3d-maximum.md` sections 8/15 to `DebugHudState.hpp`; the milestone
file-spec/cartographer references to deleted files enter their freshness queue.

Commit, move this card to `done/`, then request aggregate K8 review. There is no
next child.
