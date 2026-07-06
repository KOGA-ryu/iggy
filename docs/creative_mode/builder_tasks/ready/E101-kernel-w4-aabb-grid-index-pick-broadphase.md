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
