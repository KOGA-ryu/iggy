# E101: Kernel W4 - AabbGridIndex Broadphase For Standalone Picking

## Objective

Use the shipped `core/spatial/AabbGridIndex` as a broadphase before the
standalone object-picking narrow phase, while preserving exact pick results.

## Source Brief

- `docs/creative_mode/kernel_wiring_brief_v0_1.md`, section W4.
- Note: the source brief says the W4 recon was partially reconstructed. Treat
  this card as implementation with a strict correctness oracle.

## Scope

- `apps/iggy3d_creative/StandalonePicking.*`
- call sites in `apps/iggy3d_creative/main.cpp` or capture scenario only if
  required by the API shape.
- focused tests or deterministic capture proof.

## Required Behavior

- Build an `AabbGridIndex` from the current object visual pick candidates.
- Query the index with a conservative ray-segment AABB so the result is a
  superset of true hits.
- Run the existing exact narrow phase only over indexed candidates.
- Keep a brute-force oracle path in tests or a local debug/helper proof to assert
  the indexed pick returns the same object id as the full scan.

## Hazards

- A ray is not a box. The query AABB must be conservative enough to never drop a
  true hit.
- Result ordering matters. Preserve nearest-hit behavior exactly.
- If W1 has already landed, rotated candidates must still get the same OBB/AABB
  narrow phase as the brute-force path.

## Do Not

- Do not change visual proxy policy.
- Do not alter object ids, selection semantics, or capture script behavior.
- Do not add a persistent cache unless the small per-pick rebuild is proven
  insufficient in this slice.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Required Reads

- `docs/creative_mode/kernel_wiring_brief_v0_1.md`
- `src/core/spatial/AabbGridIndex.hpp`
- `apps/iggy3d_creative/StandalonePicking.hpp`
- `apps/iggy3d_creative/StandalonePicking.cpp`
- `apps/iggy3d_creative/StandalonePreviewProxies.*`

## Acceptance

- Indexed pick result equals brute-force pick result for a spread of deterministic
  rays over a many-object scene.
- Existing deterministic capture still selects and moves Point, Line, Path, and
  box-backed objects correctly.
- Final capture submit reason remains `package_room_meshes_presented`.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10
/Users/kogaryu/iggy3d/build/iggy3d_creative --capture /tmp/iggy3d_kernel_w4_final.png --frames 32 > /tmp/iggy3d_kernel_w4_final.log 2>&1
git -C /Users/kogaryu/iggy3d diff --check
```

Run the focused test target if a standalone picking/unit test exists or is added.

## Completion Brief

Append:

- Files changed:
- Broadphase query shape:
- Brute-force equality proof:
- Capture artifact:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief - E101

- Files changed:
  - `apps/iggy3d_creative/StandalonePicking.hpp`
  - `apps/iggy3d_creative/StandalonePicking.cpp`
  - `tests/unit/standalone_picking_tests.cpp`
  - builder queue bookkeeping files for claim/done movement
- Broadphase query shape:
  - `pickNearestVisualBoundsObject(...)` now rebuilds an
    `iggy3d::AabbGridIndex` from the current object visual pick candidate AABBs
    for each pick.
  - The ray is normalized once, then the broadphase builds a conservative
    finite ray-segment query AABB from ray origin to the farthest successfully
    indexed candidate corner projected in front of the ray, with 0.05 m padding.
  - Candidates rejected by the index because their bounds are invalid,
    non-finite, or outside the index's representable range are appended to the
    narrow-phase candidate id set as a fallback, so they are not silently lost.
  - The final narrow phase iterates the original candidate vector in original
    order and only skips candidates whose ids are absent from the indexed id set.
    This preserves nearest-hit tie behavior and keeps the existing AABB/OBB
    narrow phase unchanged.
- Brute-force equality proof:
  - Added `pickNearestVisualBoundsObjectBruteForce(...)` as an explicit oracle
    path for tests.
  - `standalone_picking_tests` now builds a many-object fixture with AABB
    candidates, one rotated OBB candidate, and off-ray clutter.
  - The test fires multiple deterministic rays and asserts indexed result
    matches brute-force object id, hit count, and nearest entry distance while
    testing no more candidates than brute force.
  - The fixture also asserts at least one ray reduces the tested candidate set.
- Capture artifact:
  - Not run. The card says not to launch a window, and the current workflow is
    avoiding the standalone capture/object-placement script.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative standalone_picking_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^standalone_picking_tests$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - `rg -n "[[:blank:]]$" /Users/kogaryu/iggy3d/apps/iggy3d_creative/StandalonePicking.hpp /Users/kogaryu/iggy3d/apps/iggy3d_creative/StandalonePicking.cpp /Users/kogaryu/iggy3d/tests/unit/standalone_picking_tests.cpp /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/claimed/E101-kernel-w4-aabb-grid-index-pick-broadphase.md /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/PRIORITY.md`
- Concerns/deferred:
  - The index is rebuilt per pick, not cached. That matches this slice's scope
    and avoids document-lifetime invalidation work until measurements show it is
    needed.
  - `ObjectVisualPickResult::testedCount` now represents exact narrow-phase
    candidate tests for the indexed path instead of full candidate count.
  - Capture proof remains deferred until capture/window runs are allowed again.

## Planner Review Repair

- Added a conservative fallback when the ray-segment query AABB cannot be
  represented by the same `AabbGridIndex` bounds policy. Without this, a camera
  ray starting outside the grid's representable query range could get an empty
  broadphase result and drop an otherwise valid narrow-phase hit.
- Added `broadphaseFallsBackWhenRayQueryExceedsGridRange()` to
  `standalone_picking_tests` to pin that false-negative case.
- Re-ran:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative standalone_picking_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^standalone_picking_tests$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over touched files
