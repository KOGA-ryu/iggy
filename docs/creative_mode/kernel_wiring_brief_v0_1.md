# Kernel Wiring Brief v0.1 — handing the six core kernels into the editor

**Author:** Claude (planner lane). **Date:** 2026-07-06. **Audience:** Codex (all wiring edits below
land in Codex-owned files — `apps/iggy3d_creative/*`, `src/app/iggy3d/creative/*`,
`adapters/RoomBake`). The core kernels are shipped and their signatures are **frozen**; this brief is
the work-order to make them *do something*.

## Context — six pure kernels shipped, all currently INERT

This session added six deterministic, pure kernels under `core/`, each wrapped-not-owned (creative
converts `CreativeBounds → Aabb3`). They compile, they're tested, and **nothing calls them yet.**

| Kernel | Header | Commit |
|---|---|---|
| `AabbGridIndex` | `core/spatial/AabbGridIndex.hpp` | `1f2574bd` |
| `OrientedBox` | `core/math/OrientedBox.hpp` | `25ba4b14` |
| `Snap` | `core/math/Snap.hpp` | `fd3c7ff6` |
| `Frustum` | `core/math/Frustum.hpp` | `c5c63739` |
| `greedyMeshGrid` | `core/grid/GreedyMesh.hpp` | `0181014b` |
| `floodFillReachability` | `core/grid/Reachability.hpp` | `ac4d21cc` |
| `generateStairs` | `core/geom/StairMesh.hpp` | `52aaa91e` |

(`generateStairs` is a content generator, not an editor-interaction kernel — it wires into a future
Stair/Ramp brush, not covered here.)

## Standing rules for the wiring

1. **Do not touch `creative/spatial/Snap` or `creative/document/DocumentSnap`.** They are the existing
   *double* 2D/3D UI-pointer snap and they work. Core `Snap` is *float* and *additive* — it only
   replaces the Move/Place position-finalization math, never the UI snap policy layer.
2. **Convert at the boundary.** `CreativeVec3`/`CreativeBounds` are `double`; core kernels are `float`.
   Convert on the way in, convert back on the way out. The float↔double round-trip is ~1e-7 — below
   any grid cell — but keep conversions explicit.
3. **Short-circuit the axis-aligned fast path.** OrientedBox and per-object cull cost is only worth
   paying for rotated / off-screen objects; keep the cheap path for the common case.
4. **FP determinism is already handled** globally (`-ffp-contract=off`, `528b4d8a`) — no per-site
   pragmas needed in the wiring.

## Recommended fire order

Ordered by *visible capability unlocked* first, then perf, then validation:

1. **W1 OrientedBox** — makes the (already-built) rotate tool actually render + pick. Highest user value.
2. **W2 Snap** — precise Move/Place. High value, small diff.
3. **W3 Frustum cull** — perf, invisible, contained (watch the const-scene hazard).
4. **W4 AabbGridIndex** — perf at object-count scale; only matters once scenes are large.
5. **W5 floodFillReachability** — the playability gate (new receipt + editor alert).
6. **W6 greedyMeshGrid** — bake draw-call collapse; most involved (identity/surface hazards).

---

## W1 — OrientedBox → rotated wireframe + rotated pick

**Current:** wireframe is built from axis-aligned bounds and picking tests the world ray against the
unrotated AABB — both ignore `object.transform.rotationEulerRadians`, so a rotated object renders and
picks as if unrotated. (This is why rotation was invisible: `Transform3::transformPoint` never applied
rotation either — OrientedBox is the first place it's honored.)

**Sites:**
- `apps/iggy3d_creative/StandalonePreviewProxies.cpp:139-153` — `visualBoundsForObject` builds the
  unrotated local AABB; feeds both wireframe and pick candidates.
- `apps/iggy3d_creative/StandalonePicking.cpp:222-233` — `pickNearestVisualBoundsObject` calls
  `rayEntryDistanceForAabb` for every candidate.

**Exact change:**
- In `visualBoundsForObject`: if `rotationEulerRadians != {0,0,0}`, build
  `makeOrientedBox(object.transform, aabbFromCreativeBounds(object.bounds))`, call
  `orientedBoxWorldAabb(...)`, return that enclosing AABB (so the wireframe box and the pick-candidate
  AABB both bound the rotated object). Else keep the current fast path verbatim.
- In `pickNearestVisualBoundsObject`: for rotated candidates, replace `rayEntryDistanceForAabb` with
  `iggy3d::intersectsRay(orientedBox, ray.origin, ray.direction, maxDist)` and use
  `hit.distanceMeters` as `entryDistance`. This needs the candidate to carry (or look up) the object's
  `Transform3 + local Aabb3`, not just the world AABB — extend `ObjectVisualPickBounds` with an
  optional OBB, or thread the `CreativeObject*`.

**Hazards:** `visualBoundsForObject` runs per-object per-frame — the rotation check must short-circuit
to the AABB path when unrotated. The pick loop needs document context to reconstruct the OBB; prefer
extending the candidate struct over a signature churn.

**Acceptance:** rotate a crate 45° about Y → its wireframe box visibly rotates; clicking the rotated
face selects it (clicking where the *unrotated* box would be but the rotated box isn't → no select).

**Lane:** Codex (`apps/iggy3d_creative/*`). Core signatures frozen.

---

## W2 — Snap → Move held-axis drag + Place cell-center

**Current:** Move finalizes position via `snapWorldAnchor` (creative `DocumentSnap`, double) sandwiched
between two `holdMoveAxis` calls (hold the dragged-locked axis before *and* after snap). Place uses a
local `snapGroundToCellCenter` doing `floor(x/cell)*cell + cell*0.5` by hand.

**Sites:**
- `src/app/iggy3d/creative/Facade.cpp:603-662` (Move resolve → snap → hold), helpers at `26-36`,`49-62`.
- `apps/iggy3d_creative/StandalonePlacement.cpp:15-20` (`snapGroundToCellCenter`).

**Exact change:**
- Move: build an `axisMask` from the held axis (**held X → `0x6`, held Y → `0x5`, held Z → `0x3`** —
  clear the held bit), convert the doc snap step/origin to `Vec3` float, call
  `snapVec3ToGrid(requested, steps, origins, axisMask)`, convert back to `CreativeVec3`. **Delete the
  second `holdMoveAxis` after snap** — the mask already held the axis. Keep the `holdMoveAxis` *before*
  snap (it sets the anchor for the held axis).
- Place (optional but recommended): replace the hand-rolled floor math with
  `snapVec3ToGrid(world, {1,1,1}, {0.5,0.5,0.5}, 0x5)` (X|Z), Y stays 0.

**Hazards:** get the mask inversion right (bit0=X, bit1=Y, bit2=Z). Core snap passes through on
non-positive step, so a disabled/zero doc-snap correctly leaves the position untouched.

**Acceptance:** Move with held Y from `(0,5,0)` to `(10,100,10)` → result `Y==5`, X/Z on the 1 m grid.
Existing `creative_tools_tests.cpp` stays green.

**Lane:** Codex (`creative/Facade.cpp`, `apps/iggy3d_creative`). **Do not** touch `creative/spatial/Snap`.

---

## W3 — Frustum → per-frame view cull before submit

**Current:** every baked mesh in `frame.projections.scene->room.meshes` is submitted; no cull.
`frame.camera.clipFromWorld` (Vulkan, z∈[0,1]) is already populated by `makeProductVulkanFrame`.

**Sites:**
- `apps/iggy3d_creative/main.cpp:~640` (frame built) → `:1577` (`backend->submitFrame(frame)`).
- `src/render/FrameInput.hpp` — `RenderCameraFrame.clipFromWorld`. **`projections.scene` is `const`.**

**Exact change:** once per frame, `frustumPlanesFromClip(frame.camera.clipFromWorld,
ClipDepthRange::ZeroToOne)`; for each mesh build its AABB from `position ± size*0.5` and keep it only
if `aabbInFrustum(planes, aabb)`.

**Hazards (important):** `projections.scene` is **const** in `FrameInput` — do **not** cast it away at
the frame-build site. Do the cull where the frame is *consumed* (the render loop / backend
`submitFrame`), or have the scene builder emit an already-culled mesh list. Non-finite/degenerate
mesh sizes → guard (the kernel already fail-safes a non-finite matrix to "cull nothing"). This is
AABB cull; rotated objects (post-W1) are conservatively over-kept — fine for a broadphase cull.

**Acceptance:** point the camera away from a large floor → its meshes drop out of the draw list
(instrument the submitted mesh count); nothing visible ever disappears while on-screen.

**Lane:** Codex (`apps/iggy3d_creative/main.cpp` + wherever the const-safe cull lands).

---

## W4 — AabbGridIndex → broadphase before the pick loop

**Current:** `pickNearestVisualBoundsObject` (StandalonePicking.cpp:222) runs the ray-vs-AABB slab
test over **all** candidates — O(n) every click.

**Exact change:** build an `AabbGridIndex` from the candidate bounds (id → world AABB;
`rebuildFrom(...)` on the candidate set, or maintain it incrementally off the document dirty flags).
For a pick, `query()` with the ray's traversal region (a fat AABB spanning the ray segment, or the
ray's screen-region bounds) to get the small candidate id set, then run the existing slab loop **only
over those**. The index is also the gather step for W3's cull-by-region and for snap-neighbourhood
later — build it once, reuse.

**Hazards:** a pick ray is a segment, not a box — bound it generously (near→far along the ray) so the
broadphase never drops a true hit (the index guarantees a superset only for the *query box* you give
it). Correctness oracle: the index-narrowed pick must return the same object as the full O(n) scan.

**Acceptance:** with the index, `pickNearest...` returns the identical `objectId` as the brute scan for
a spread of rays over a many-object scene (assert equality in a test).

**Lane:** Codex (`apps/iggy3d_creative/StandalonePicking.cpp`).

---

## W5 — floodFillReachability → post-bake playability gate

**Current:** the editor authors geometry but never proves it's *playable* — no check that all walkable
ground is reachable from spawns, no isolated-island detection. (The strategic gap in the plan.)

**Site:** `src/app/iggy3d/creative/adapters/RoomBake.cpp`, after object classification, before return.

**Exact change:** add `validateReachabilityFromDocument(doc, room, cellSize=1.0)` →
`CreativeReachabilityValidationReceipt { bool hasIslands; uint32_t strandedCellCount; string reasonCode; }`.
Project occupancy-bearing objects to a 2D XZ walkable grid (via `SpatialProjection`), take spawn/npc
anchors as seeds, call `floodFillReachability(grid, seeds, FourWay)`, receipt the
`strandedWalkableCount`. Append the receipt to the bake result and log OK / `N stranded cells`.

**Hazards:** cell size must match agent footprint (make it a `CreativeRoomBakeRequest` field). A seed on
a blocked cell is silently ignored by the kernel (by design) — pre-validate spawn placement or warn
separately. v1 is 2D (XZ) only; multi-level (stairs/links) needs Y-aware seeds — defer.

**Acceptance:** author two disconnected floor islands with one spawn → receipt reports
`strandedCellCount > 0`; connect them with a walkable path → `strandedCellCount == 0`.

**Lane:** Codex (`adapters/RoomBake`).

---

## W6 — greedyMeshGrid → bake draw-call collapse

**Current:** `buildRoomAssetFromCreativeDocument` emits one `RoomStaticMeshAsset` per object; a 3×3
floor = 9 meshes / 9 draw calls. No merge.

**Site:** `src/app/iggy3d/creative/adapters/RoomBake.cpp:547-622` (object → static-mesh emission).

**Exact change:** before emitting per-object meshes, group coplanar same-`(role, materialId,
occupancyKind)` objects, project each group onto its plane grid (Floor → XZ at constant Y; cell size
from `gridSettings.cellSizeMeters`), set `grid.keys[cell] = groupKey`, call `greedyMeshGrid(grid)`, and
emit **one mesh per `GreedyQuad`** instead of one per object. Start with the Structural-Floor plane
only.

**Hazards (why this is last):** (1) merging N objects → 1 quad **loses object identity** —
`mesh.id ↔ object.id` breaks (logging, undo, spatial queries). Preserve a quad→source-ids mapping
(extend `CreativeRoomBakeSpatialSurfaceSource`). (2) Walls need axis disambiguation (reuse the existing
`size.x >= size.z` rule). (3) Spatial surfaces are per-object today — decide merged-vs-per-object
before collapsing floors that carry walkable surfaces. (4) Only grid the bounding region of a group,
not the whole document, or a huge sparse floor wastes grid memory.

**Acceptance:** bake a 5×5 same-material floor → `staticMeshCount` drops from 25 to 1; the rendered
floor is visually identical; collision/walkable surface still covers the whole area.

**Lane:** mixed — kernel is mine (frozen), the bake grouping is Codex (`adapters/RoomBake`).

---

## Note

The `AabbGridIndex` seam recon agent hit a tool-retry cap during the recon workflow; W4 above is
reconstructed from the adjacent OrientedBox/Frustum recon plus a direct read of
`StandalonePicking.cpp:212-236`. If Codex wants the fuller machine-generated seam dossier for the other
five, it's in this session's `kernel-wiring-recon` workflow output.
