# K9a - Receipt Support Tail Condensation

## Status

READY v1.1, REISSUED AFTER BASELINE-RED TEST STOP. K9 batch step 1 of 1. Claim
only after K8 aggregate acceptance at `732ca6e5`. Authority:
`docs/creative_mode/builder_tasks/blocked/K9-receipt-support-tail-condensation-plan.md`.

Resume the preserved uncommitted K9 implementation; do not discard or recreate
it. The original pre-edit gates and post-edit all-target build are accepted
evidence. Apply the exact v1.1 fixture correction, then rerun the complete
post-edit gate. On green completion, commit this card, move it to `done/`, and
send Reviewer one aggregate K9 brief. A new STOP pauses the batch.

## v1.1 Stop Ruling

The first K9 attempt correctly stopped on `product_gameplay_tape_runner_tests`:
`FAIL: ai command logged`. Planner reproduced the identical failure in an
isolated clean checkout of accepted HEAD `732ca6e5`; K9 did not cause it.

The stale NPC tape fixture supplies no collision surfaces. Under landed R3.1,
an absent bake is `Unknown` and cannot fabricate sight or AI commands. Planner
validated the exact correction below in that isolated checkout; the CTest then
passed. Keep the test in the required set.

## Goal

Condense four receipt-support implementations into `ReceiptRecording.cpp` and
`ReceiptFields.cpp`, reducing the receipt folder from 17 files to 15 without
changing APIs, receipt truth, or behavior, and repair the one stale successful-
bake premise exposed by the required tape-runner CTest.

## Pre-Edit Gate

Before any edit:

```sh
test "$(find src/app/iggy3d/receipt -maxdepth 1 -type f | wc -l | tr -d ' ')" = 17
cmake -S . -B build
CCACHE_DISABLE=1 cmake --build build --target all
python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json > /tmp/k9_dependency_before.json
rg -n '^void recordProduct' src/app/iggy3d/ReceiptBuilder.cpp src/app/iggy3d/receipt/*.cpp
rg -n '^void appendProductStartupWorldBuildoutFields|^std::string floatReceiptValue' src/app/iggy3d/receipt/*.cpp
rg -n 'appendProductStartupWorldBuildoutFields' src/app/iggy3d/ReceiptBuilder.cpp src/app/iggy3d/receipt/ReceiptFields.hpp src/app/iggy3d/receipt/*.cpp
rg '^#include' src/app/iggy3d/receipt/CreativeReceiptRecording.cpp | sort > /tmp/k9_creative_includes.txt
rg '^#include' src/app/iggy3d/receipt/PhysicsReceiptRecording.cpp | sort > /tmp/k9_physics_includes.txt
comm -13 /tmp/k9_creative_includes.txt /tmp/k9_physics_includes.txt
rg -n 'CreativeReceiptRecording\.cpp|PhysicsReceiptRecording\.cpp|StartupWorldBuildoutFields\.cpp|ReceiptFields\.cpp' CMakeLists.txt cmake
git diff --check
```

The survey must show eleven `recordProduct*` definitions split 10/1 across the
Creative/physics files, one startup composer, one formatter, exactly one
startup caller in `ReceiptBuilder.cpp`, and exactly one CMake entry for each
current source. The `comm` command must print nothing, proving every physics
include already exists in the Creative source. Otherwise STOP without edits.

## Exact Files

Create by rename:

- `src/app/iggy3d/receipt/ReceiptRecording.cpp` from
  `src/app/iggy3d/receipt/CreativeReceiptRecording.cpp`.

Delete:

- `src/app/iggy3d/receipt/CreativeReceiptRecording.cpp`
- `src/app/iggy3d/receipt/PhysicsReceiptRecording.cpp`
- `src/app/iggy3d/receipt/StartupWorldBuildoutFields.cpp`

Update:

- `src/app/iggy3d/receipt/ReceiptFields.cpp`
- `CMakeLists.txt`
- `docs/branch_gate_approvals.tsv`
- `tests/unit/product_gameplay_tape_runner_tests.cpp` only as specified below.

Do not edit any other source/test/doc. ASCII and cartographer work remain inert
and unstaged.

## Recording Merge

1. `git mv` `CreativeReceiptRecording.cpp` to `ReceiptRecording.cpp`.
2. Keep the renamed file's include block unchanged; it already covers the
   physics recorder.
3. Move `setPhysicsMovementPlannerProof` into the existing anonymous namespace.
4. Move `recordProductPhysicsMovementPlannerTickProof` intact into the same TU.
5. Preserve all ten Creative recorder functions and private helpers unchanged.

The result owns exactly the eleven `recordProduct*` definitions declared in
`ReceiptBuilder.hpp`. Do not introduce a shared template, dispatcher, callback,
or renamed public function.

Update BG-1114's ledger path only to
`src/app/iggy3d/receipt/ReceiptRecording.cpp`; preserve its description/owner/
date and all three nearby source comments. The final commit must pass default
rename detection for the Creative file; do not add new gate IDs to compensate
for a reconstructed-file diff.

## Field Support Merge

Move `appendProductStartupWorldBuildoutFields` into `ReceiptFields.cpp` after
the unchanged `floatReceiptValue` definition. Preserve startup appender order:

1. startup probe;
2. world authoring;
3. active room.

Do not touch `ReceiptFields.hpp` or `ReceiptBuilder.cpp`. The external function
signature and sole call remain unchanged.

## Tape Fixture Correction

In `tapeWaitLetsNpcAttackPlayer` only:

1. After creating `session`, build
   `const iggy3d::SpatialSurfaceSet surfaces =
   iggy3d::buildSpatialSurfaceSet(room.roomAsset.room);`.
2. Change the existing tape-run call from
   `{&*session, &parsed.tape}` to `{&*session, &parsed.tape, &surfaces}`.

The required include and helper are already present in this test. Do not change
the 24 waits, any assertion, room/session setup, production tape runner, or
R3.1/null-bake behavior. This test edit repairs its successful-bake fixture; it
does not repin expected behavior.

## CMake And Mechanical Checkpoint

- CMake has exactly one `ReceiptRecording.cpp` and one `ReceiptFields.cpp`
  entry; all three deleted cpp basenames have zero CMake hit.
- Receipt folder is 17 -> 15 files.
- `ReceiptRecording.cpp` is 390-430 lines; `ReceiptFields.cpp` is 20-35.
- Eleven public recorders have one definition in `ReceiptRecording.cpp`.
- Startup composer and formatter each have one definition in
  `ReceiptFields.cpp`.
- `git diff HEAD --summary --find-renames=50%` identifies Creative ->
  `ReceiptRecording.cpp` as a rename.
- Against `/tmp/k9_dependency_before.json`: source files -2; direct edges 0;
  app fan-out 0; directed/unordered pairs 0/0; zero SCCs and policy violations.
- No header, caller, test, field-emitter, golden, save/hash/replay, or policy
  diff exists, except the exact tape-fixture correction above.

## Post-Edit Verification

```sh
cmake -S . -B build
CCACHE_DISABLE=1 cmake --build build --target all
ctest --test-dir build -R '^(product_receipt_key_order_tests|product_creative_ui_projection_receipt_tests|product_creative_ui_frame_tests|product_creative_ui_window_frame_tests|product_creative_ui_input_frame_tests|product_creative_ui_command_receipt_tests|product_creative_viewport_pick_frame_tests|product_creative_wireframe_frame_tests|product_creative_world_launch_tests|product_gameplay_controller_tests|product_gameplay_tape_runner_tests|dependency_direction_tests)$' --output-on-failure
python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json > /tmp/k9_dependency_after.json
python3 - <<'PY'
import json

with open('/tmp/k9_dependency_before.json') as stream:
  before = json.load(stream)
with open('/tmp/k9_dependency_after.json') as stream:
  after = json.load(stream)

checks = {
    'source_file_delta': (after['scan']['source_file_count'] - before['scan']['source_file_count'], -2),
    'direct_edge_delta': (after['scan']['direct_edge_count'] - before['scan']['direct_edge_count'], 0),
    'directed_pair_delta': (after['scan']['directed_pair_count'] - before['scan']['directed_pair_count'], 0),
    'unordered_pair_delta': (after['scan']['unordered_pair_count'] - before['scan']['unordered_pair_count'], 0),
    'app_fan_out_delta': (after['fan_out']['app']['edge_count'] - before['fan_out']['app']['edge_count'], 0),
}
for name, (actual, expected) in checks.items():
  if actual != expected:
    raise SystemExit(f'{name}: expected {expected}, got {actual}')
if after['strongly_connected_components']:
  raise SystemExit('K9 introduced an SCC')
if not after['policy']['passed'] or after['policy']['violations']:
  raise SystemExit('K9 violated dependency policy')
print(checks)
PY
test "$(find src/app/iggy3d/receipt -maxdepth 1 -type f | wc -l | tr -d ' ')" = 15
wc -l src/app/iggy3d/receipt/ReceiptRecording.cpp src/app/iggy3d/receipt/ReceiptFields.cpp
rg -n 'CreativeReceiptRecording\.cpp|PhysicsReceiptRecording\.cpp|StartupWorldBuildoutFields\.cpp' src apps tests CMakeLists.txt cmake
rg -n '^void recordProduct' src/app/iggy3d/receipt/ReceiptRecording.cpp
rg -n '^void appendProductStartupWorldBuildoutFields|^std::string floatReceiptValue' src/app/iggy3d/receipt/ReceiptFields.cpp
rg -n 'ReceiptRecording\.cpp|ReceiptFields\.cpp' CMakeLists.txt cmake
rg -n 'BG-1114' src/app/iggy3d/receipt/ReceiptRecording.cpp docs/branch_gate_approvals.tsv
git diff HEAD --summary --find-renames=50%
git diff HEAD -- tests/golden src/runtime/save src/runtime/replay docs/architecture_dependency_policy.json
git diff --check
git diff --cached --check
```

The stale-source grep returns no output. The CMake grep returns exactly the two
target sources. Golden/save/replay/policy diff is empty. Do not run broad CTest
or launch an app/window.

After committing, run:

```sh
python3 tools/check_branch_gate.py --diff HEAD^..HEAD
```

## Stop Conditions

- Pre-edit all-target build, dependency policy, folder count, owner survey,
  include-superset premise, or startup caller premise fails.
- Rename detection does not preserve Creative -> ReceiptRecording, or the
  merged source needs a new include/gate ID/generalization.
- Any public declaration, caller, test, field emitter, receipt expectation,
  formatter behavior, recorder assignment/status, or startup order changes,
  except the exact authorized tape fixture.
- The tape CTest still fails after supplying the authored room surfaces, or
  fixing it requires production changes, assertion changes, or another test.
- `ReceiptRecording.cpp` exceeds 430 lines or `ReceiptFields.cpp` exceeds 35.
- Post-edit all-target build, focused CTests, folder count, exact deltas, branch
  gate, SCC, policy, golden, or whitespace checks fail.

## Completion Brief

Append the rename/absorption map, final function ownership, unchanged public/
receipt behavior evidence, startup call-order proof, BG-1114 relocation, pre/
post all-target results, focused CTests, folder/line/delta metrics, rename
detection, protection diffs, clean-HEAD failure reproduction, and the exact
fixture correction. Flag the post-acceptance file-spec freshness order for
Planner; do not edit those docs.

Commit, move this card to `done/`, then request aggregate K9 review. There is no
next child.
