# E73: RoomBake Blocker Surface Normals

## Objective

Make RoomBake emit truthful spatial-surface normals for baked walls and
structural blockers instead of hardcoding every blocker to `+Z`.

## Problem

RoomBake currently sets all non-floor blocker normals to `{0, 0, 1}`:

- actor blocker surfaces use `surface.normal = {0.0F, 0.0F, 1.0F}`;
- projectile blocker surfaces use the same normal.

That keeps active-room/collision counts green, but it is semantically wrong for
walls or generated room-shell sides that run along Z and should face +/-X.
Existing tests assert mesh counts, surface ids, blocker roles, and collision
readiness, but they do not assert blocker normal direction.

This matters because `SpatialSurfaceSet` normalizes and exposes
`CollisionSurfaceView::normal`; downstream movement, wall interaction, or
reasoning code can consume that normal even when AABB counts are correct.

## Required Reads

- `src/app/iggy3d/creative/adapters/RoomBake.cpp`
- `src/content/authoring/EditableRoomDocument.cpp`
- `src/runtime/collision/SpatialSurfaceSet.cpp`
- `tests/unit/creative_document_room_bake_tests.cpp`
- `tests/unit/product_creative_world_launch_tests.cpp`

## Scope

- Derive blocker/projectile surface normals from baked geometry.
- For wall role / wall-segment meshes, use the thin horizontal axis to choose
  an axis-aligned normal consistent with the wall orientation.
- For generic prop boxes where no face is selected, keep a documented default
  or add a small policy helper; do not pretend all surfaces are oriented walls.
- Keep mesh ids, surface ids, counts, and RoomAsset role strings stable.

## Acceptance

- A wall whose thin axis is Z gets a Z normal.
- A wall whose thin axis is X gets an X normal.
- Generated room-shell east/west and north/south walls no longer all produce
  identical blocker normals.
- Tests assert normals for at least two perpendicular wall orientations.
- Active-room collision still builds ready query surfaces.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_document_room_bake_tests product_creative_world_launch_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_document_room_bake_tests|product_creative_world_launch_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not change RoomBake inclusion policy.
- Do not change surface counts or source sidecar ids.
- Do not add renderer/Vulkan changes.
- Do not broaden into full collision-response redesign.

## Completion Brief

- Status: done.
- Files modified:
  - `src/app/iggy3d/creative/adapters/RoomBake.cpp`
  - `tests/unit/creative_document_room_bake_tests.cpp`
- Implementation:
  - Added `wallBlockerNormal(...)` and `blockerNormalForRole(...)` in RoomBake.
  - Wall blocker/projectile surfaces now derive normals from baked wall
    orientation:
    - walls running along X use `{0, 0, 1}`;
    - walls running along Z use `{1, 0, 0}`.
  - Generic prop/blocker boxes keep the documented legacy `{0, 0, 1}` default
    because they do not identify a selected face yet.
  - Mesh ids, surface ids, source sidecars, counts, and RoomBake inclusion policy
    were unchanged.
- Tests:
  - Added direct perpendicular wall coverage proving an X-running wall emits +Z
    actor/projectile normals and a Z-running wall emits +X actor/projectile
    normals.
  - The same test proves active-room collision remains ready and preserves those
    normals in query surfaces.
  - Extended generated room-shell bake coverage to assert both east/west and
    north/south wall blocker normals are present instead of all walls sharing one
    normal.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_document_room_bake_tests product_creative_world_launch_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_document_room_bake_tests|product_creative_world_launch_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over touched files
- Result: all passed.
