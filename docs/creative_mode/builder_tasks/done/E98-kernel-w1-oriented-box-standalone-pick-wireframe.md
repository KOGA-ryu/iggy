# E98: Kernel W1 - OrientedBox For Standalone Rotated Pick And Enclosing Visual Bounds

## Objective

Wire the shipped `core/math/OrientedBox` kernel into the standalone creative
app so rotated objects get truthful rotated picking and enclosing visual bounds.

## Source Brief

- `docs/creative_mode/kernel_wiring_brief_v0_1.md`, section W1.
- Core kernel signatures are frozen. Use the kernel; do not modify it unless a
  compile error proves the signature is unusable.

## Scope

- Standalone app only:
  - `apps/iggy3d_creative/StandalonePreviewProxies.*`
  - `apps/iggy3d_creative/StandalonePicking.*`
  - tests or capture-script code only if needed for a focused proof.
- In `visualBoundsForObject(...)`, preserve the current axis-aligned fast path
  for unrotated objects.
- For rotated objects, convert `CreativeBounds` and `CreativeTransform` at the
  boundary, build an `iggy3d::OrientedBox`, and use
  `orientedBoxWorldAabb(...)` as the visual/pick candidate bound.
- In the pick narrow phase, use `iggy3d::intersectsRay(...)` for rotated
  candidates and keep the current AABB slab test for unrotated candidates.
- Prefer extending `ObjectVisualPickBounds` with optional OBB data over repeated
  document lookups or wide signature churn.

## Do Not

- Do not change `CreativeDocument`, descriptors, RoomBake, product app input, or
  renderer/Vulkan.
- Do not make all picking pay the OBB cost. The unrotated path must remain the
  cheap path.
- Do not add per-kind rotate policy.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Required Reads

- `docs/creative_mode/kernel_wiring_brief_v0_1.md`
- `apps/iggy3d_creative/AGENTS.md`
- `docs/creative_mode/standalone_app_handoff.md`
- `apps/iggy3d_creative/StandalonePreviewProxies.hpp`
- `apps/iggy3d_creative/StandalonePreviewProxies.cpp`
- `apps/iggy3d_creative/StandalonePicking.hpp`
- `apps/iggy3d_creative/StandalonePicking.cpp`
- `src/core/math/OrientedBox.hpp`
- `src/core/math/Aabb3.hpp`
- `src/core/math/Transform3.hpp`

## Acceptance

- Existing capture scene still renders normally.
- A deterministic proof covers a rotated object:
  - the rotated object has a different OBB hit shape than its unrotated AABB;
  - clicking through a real rotated face selects it;
  - clicking an area that only the stale unrotated box would have covered does
    not select it.
- Capture final submit reason remains `package_room_meshes_presented`.
- Round-trip proof remains stable.
- The completion brief states whether the PNG hash changed and why.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10
/Users/kogaryu/iggy3d/build/iggy3d_creative --capture /tmp/iggy3d_kernel_w1_final.png --frames 32 > /tmp/iggy3d_kernel_w1_final.log 2>&1
git -C /Users/kogaryu/iggy3d diff --check
```

Also run the narrow standalone-focused/unit target if one is touched or added.

## Completion Brief

Append:

- Files changed:
- Behavior changed:
- Rotated-pick evidence:
- Capture artifact:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief - 2026-07-06

- Files changed:
  - `apps/iggy3d_creative/StandalonePreviewProxies.hpp`
  - `apps/iggy3d_creative/StandalonePreviewProxies.cpp`
  - `apps/iggy3d_creative/StandalonePicking.hpp`
  - `apps/iggy3d_creative/StandalonePicking.cpp`
  - `apps/iggy3d_creative/main.cpp`
  - `tests/unit/standalone_picking_tests.cpp`
  - `cmake/iggy3d_tests.cmake`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
- Behavior changed:
  - Unrotated standalone visual picking stays on the existing cheap AABB path.
  - Rotated, descriptor-bounds-backed objects now build an optional
    `iggy3d::OrientedBox` sidecar for exact pick narrow phase.
  - `visualBoundsForObject(...)` uses `orientedBoxWorldAabb(...)` for rotated
    visual/pick candidate bounds, while preserving point/path proxy handling and
    line proxy thickness.
  - `main.cpp` now builds pick candidates through `buildObjectVisualPickBounds`,
    so visual bounds, screen AABB, and optional OBB data stay in one helper.
- Rotated-pick evidence:
  - Added `standalone_picking_tests`.
  - Test pins that unrotated objects have no OBB sidecar.
  - Test pins that rotated visual bounds remain centered on the transform anchor
    and expand beyond the stale unrotated AABB.
  - Test pins a ray outside the stale unrotated AABB but through the real
    rotated face selects the object.
  - Test pins a ray through stale-only unrotated AABB space does not select the
    rotated object.
- Capture artifact:
  - Baseline-only capture was taken before implementation:
    `/tmp/iggy3d_kernel_w1_baseline.png`,
    SHA-256 `5641f645abd7c2152fb5c3af9e3d520c4e5cfe11a9af6d756c5ec6dd213355b6`.
  - Baseline receipt had `ROOM_BAKE final ... status='baked'
    reasonCode='creative_room_baked' ... standalonePreviewMeshes=3
    sceneMeshes=1690` and `ROUNDTRIP ... match=1`.
  - No final capture was run because the user explicitly cut out the script
    path that opens the standalone app and places objects. The rotated proof is
    covered by the focused unit target instead.
- Tests/checks run:
  - `cmake -S /Users/kogaryu/iggy3d -B /Users/kogaryu/iggy3d`
  - `cmake --build /Users/kogaryu/iggy3d/build --target standalone_picking_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^standalone_picking_tests$' --output-on-failure`
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10`
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d iggy3d_creative standalone_picking_tests -j10`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - `rg -n "[ \t]+$"` over touched files; no matches.
- Concerns/deferred:
  - This slice does not draw diagonal rotated wireframe corners; it uses the
    shipped OBB kernel for exact pick and world-AABB visual candidate bounds as
    requested by W1.
  - Existing capture-script placement proof remains untouched.

## Repair Brief - 2026-07-06

- Files changed:
  - `apps/iggy3d_creative/StandalonePicking.cpp`
  - `tests/unit/standalone_picking_tests.cpp`
  - `docs/creative_mode/builder_tasks/done/E98-kernel-w1-oriented-box-standalone-pick-wireframe.md`
- Behavior changed:
  - `pickNearestVisualBoundsObject(...)` now normalizes the ray direction once
    before testing candidates, so AABB `entryDistance` and OBB
    `distanceMeters` are compared in the same units.
- Repair evidence:
  - Added a mixed AABB + OBB regression test with a non-unit ray.
  - The test pins that the closer OBB wins over a farther AABB when both are
    hit, preventing W4 broadphase work from inheriting mismatched distance
    units.
- Wording clarification:
  - The E98 card title/objective now says "rotated pick and enclosing visual
    bounds"; true diagonal rotated debug wireframe remains deferred.
