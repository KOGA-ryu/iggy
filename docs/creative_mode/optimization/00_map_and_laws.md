# 00 — Map And Laws

> The reconciliation layer. Research taxonomy on one side, iggy3d reality on the
> other, and the laws every packet obeys in between. All file:line anchors are at
> trunk `cd6b9f4e` (2026-07-05) unless marked; they drift — workers re-verify and
> receipt the drift, they do not silently adapt.

## 1. Reconciliation with the foundation plan

The research set was written against a blank engine. iggy3d is not blank. The
mapping of research concepts onto existing foundation-plan coordinates:

| Research concept | Foundation coordinate | Status |
| --- | --- | --- |
| capability matrix / profiles | D6 descriptor-column law + W3 (columns migrate in) | table EXISTS, columns pending |
| uniform spatial grid | L4 `CreativeOccupancyIndex` + W8 | name reserved, unbuilt |
| dirty domains + tracker | L4 dirty channels + W4 accumulator | `dirtyFlagsForMutation` exists, accumulator unbuilt |
| bake outputs | `adapters/{Draw,RoomEd,ObjCat}` reserved sockets + plan L5 bake bridge | 0-byte sockets |
| mutation spine | the ~70-verb `CreativeMutationKind` + `applyDocumentMutation` receipts | EXISTS (W1 repairs in flight) |
| room graph / portals | `docs/multiroom_connectivity_design.md` (different stream) | do NOT rebuild here |
| render instancing / LOD | far-future sockets — creative draws wireframe boxes first (plan L5) | contract only |

**Sequencing law:** no OP packet starts before its foundation dependency lands.
Current blockers, in order: gate red (plan §8 F16), W1 mutation repairs, W1w
live-wiring repair, W2 world math (document-owned grid settings + ids). The
spatial index (02) depends on W2's `CreativeGridSettings`; the dirty/bake doc
(05) depends on W4; bridges (04) depend on W5/W6 persistence + W3 creation.

## 2. Anchors master table (research name → iggy3d reality)

| Research type | iggy3d reality | Anchor |
| --- | --- | --- |
| `CreativeObjectKind` | exists, ~107 kinds + Unknown | `src/app/iggy3d/creative/Object.hpp:35-162` |
| `CreativeObject` | exists: `id`(u64), `kind`, `name`, `transform{position,rotation,scale}` (double `CreativeVec3`), `bounds{min,max}`, `layerId`, `visible`, `locked`, `tags`, `parentId` | `Object.hpp:164-178` |
| `CreativeObjectId` | exists, `uint64` (`Object.hpp:12`). WARNING: `creative::Id` is `uint32` (`Core.hpp:7`) — K4 seam. **New structures use the u64 id, always.** | both |
| `CreativeObjectDescriptor` | exists, ~108 rows | `src/app/iggy3d/creative/ObjectDescriptor.cpp` |
| `CreativeOptimizationProfile` | becomes descriptor COLUMNS, not a new parallel enum: projection profile (point/box/volume/line/link — today a switch in `SpatialProjection.cpp`, W3 migrates it) + new `bakeDomains` mask column (doc 01) | doc 01 |
| `CreativeBakeMask` / `CreativeBakeDomain` | reconcile with existing `CreativeDirtyFlags` produced by `dirtyFlagsForMutation` (Identity/Preview/Serialization/Transform/Navigation/Gameplay/Geometry/Collision…) — doc 01 defines the union, ONE flag family, no duplicate vocabulary | `ObjectDescriptor.cpp` (~:356) |
| `CreativeAabb3` | exists as the bounds struct on `CreativeObject`; helper math (normalize/overlap/expand) lands in W2's L0 world-math header | `Object.hpp` + W2 |
| `CreativeSpatialGrid` | **`CreativeOccupancyIndex`** (use the plan's reserved name) | doc 02, W8 |
| `CreativeSpatialProxy` | new, defined in doc 02 | `creative/OccupancyIndex.hpp` (new) |
| `CreativeMutation` | exists as mutation grammar + receipts (`DocumentMutation.hpp:128-205`, `MutationApply.cpp`) | exists |
| grid coord / cell | exists: `CreativeSpatialCell{index,coord,objectId,objectKind,occupancyKind}` in `SpatialProjection.hpp`; per-call `cellSize` is a DISEASE W2 cures (document-owned `CreativeGridSettings`) | `SpatialProjection.hpp:78-147` |
| runtime collision target | `RoomAsset.spatialSurfaces` — `SpatialSurfaceSet`, roles Walkable/Blocker/ProjectileBlocker + collision masks | `src/content/assets/RoomAsset.hpp:54-67` |
| runtime nav target | reasoning graph (`runtime/ai`): nodes/edges, `markerToReasoningNode` string mapping, per-kind travel costs | A7 contract, `docs/affordance_vocabulary_v0_1.md` |
| runtime spawn/marker target | `entityFromAnchor` routing (npc/pickup/door/exit/inert) | `src/app/iggy3d/world/PackageSessionSeed.cpp:156-175` |
| renderer | `FramePresenter`: Vulkan path appends overlays (`:762-768`), SDL path does NOT (`:844-884`, F4); virtual→drawable scale at `:28-42` | `src/app/iggy3d/window/FramePresenter.cpp` |

Types the research assumed that do NOT exist in iggy3d and are NOT to be
invented ad hoc: `Transform3` (use `CreativeObject.transform`), `MeshResourceId`
/ `MaterialResourceId` (no creative mesh identity exists yet — doc 03 defers),
`Vec3` (use `CreativeVec3`; do not add a fifth vector type).

## 3. The algorithm laws

Numbered so packets can cite them as `LAW-n`.

1. **One coordinate space.** Any click/viewport pairing is normalized at the
   boundary into ONE declared unit space; every consumer states which space it
   is in. Tests run at 1x AND 2x (drawable ≠ window). (From confirmed findings
   F1/F2 — the DPI seam that broke picking on the dev Mac.)
2. **Ray pick is DDA, not a z-plane.** Viewport picking walks cells along the
   ray (Amanatides–Woo 3D DDA), nearest-t first, tie → lowest object id. The
   `z=0` literal (F3) is banned. Doc 02 owns the spec.
3. **Cell coverage law.** An AABB covers cells `floor(min/cell) .. floor(max/cell)`
   inclusive per axis, with negative coordinates flooring (never truncating), and
   a stated boundary rule: a max lying exactly on a cell boundary does NOT enter
   the next cell (`floor` on max with an epsilon subtraction is banned — the rule
   is arithmetic, not epsilon). Golden cases in doc 02 pin all three.
4. **Occupancy is never fabricated.** Out-of-range content projects/indexes as
   empty (intersection semantics), uniformly across profiles. Relocation-clamp
   (F10) is banned. (D8 corollary.)
5. **Placement mutations move what the profile projects.** (D8. F9 is the
   disease; W1 repairs it; every new bake/index consumes post-D8 semantics.)
6. **Sparse before dense, hash before tree.** The index is a sparse hash grid
   keyed by packed coords. Dense arrays (bounded world) and BVH/octrees are
   metric-gated upgrades (doc 06, OP7), not defaults.
7. **Generation-stamp dedup.** Multi-cell query dedup uses a per-proxy visited
   stamp (u32 query counter), not a set. Zero allocation on the query path once
   the result vector is warm.
8. **Fat bounds for edit stability.** Proxies store true bounds + fat bounds
   (inflated 0.5 cell). Reinsert only when true bounds escape fat bounds.
   Dragging an object must not churn cells every frame.
9. **Large-object side list.** Objects spanning > `kMaxCellsPerObject` (first
   value: 128) go to a side list checked linearly alongside grid hits, with a
   metric. No clever solution before the metric proves need.
10. **Mutations are the only dirty writer.** Dirty channels derive from
    descriptor columns via the L4 map; systems DRAIN channels, they never poll
    the document. (W4; doc 05.)
11. **Bake DAG order is law.** spatial → room assignment → nav/collision →
    light/audio lists. A baker may only read outputs of earlier stages. Doc 05
    states the full DAG; violating it = building light lists off a stale room
    graph.
12. **Double-buffered bake outputs.** Build fresh, swap atomically. No consumer
    ever observes a half-rebuilt structure.
13. **Rebuild policy is explicit.** Per domain: incremental patch below
    threshold, full rebuild above (first threshold: dirty objects > 30% of
    domain population). The threshold is a named constant with a metric.
14. **Renderer parity.** A surface that consumes input MUST be drawn by every
    renderer that routes that input (F4; W1w law). Applies to every future
    creative draw layer.
15. **LOD hysteresis.** Distance bands carry ±epsilon overlap so boundary
    objects don't flicker. Band validation permits exactly this overlap and
    nothing else. (Doc 03; far-future but the law is cheap to state now.)
16. **Receipts + repair.** Every operation validates → REPAIRS where safe →
    receipts what it did (status, counts, repair codes). Rejection without a
    receipt code is banned. Repair without a receipt is worse.
17. **Wire strings for gameplay bridges.** Creative→runtime gameplay vocabulary
    travels as append-only anchor-kind strings; unknown kinds are
    ignore-and-continue, never an error. (`docs/affordance_vocabulary_v0_1.md`
    law, inherited by doc 04.)
18. **THE grid is the only creative spatial truth.** (D3.) The index, the draw
    adapter, and every bake read document-owned `CreativeGridSettings`. The other
    three grid universes (ascii centered / room_editor lround / map_maker
    lattice) stay theirs; conversions live in named bridge files only.
19. **u64 ids everywhere new.** No new structure narrows `CreativeObjectId`.
    The `creative::Id` u32 seam (K4) is quarantined, not propagated.
20. **Metrics are part of done.** Each packet's metric table ships with the
    packet, wired into the existing stats/receipt pattern (`Facade` stats_,
    receipt fields) — not a TODO.

## 4. Packet DAG (detail in doc 06)

```
foundation: [gate green] -> W1 -> W1w -> W2 ----------------+
                                                            |
OP1 descriptor columns (doc 01, rides W3) -> OP2 occupancy index (doc 02, W8a)
        |                                          |
        v                                          v
OP3 dirty/bake pipeline (doc 05, rides W4) -> OP4 draw adapter (doc 03, W8b)
                                                   |
W5/W6 persistence + W3 creation ------------------>+-> OP5 gameplay bridges (doc 04)
                                                   |
metrics over threshold --------------------------->+-> OP6 render scaling (doc 03 §LOD/instancing)
                                                   +-> OP7 BVH research (doc 06)
```

## 5. House packet format

Every packet in docs 01–05 carries: **Lanes** ([C]/[P] + files), **Entry
criteria** (which foundation orders + which anchors verified), **Frozen
interfaces** (exact signatures + pre/postconditions + receipt struct),
**Algorithm spec** (pseudocode + invariants + complexity + memory layout),
**Golden cases** (numeric, test-ready), **Repair rules**, **Slices** (each with
files, forbidden files, tests, gate command), **Metrics**, **Non-goals**,
**RECON items** (open questions a worker answers BEFORE building, with evidence
labels).

Gate for every slice: `ctest --test-dir build` fully green on the Mac
(structural, never a hard-coded count), box GCC/Vulkan-OFF parity as the fleet
post-check. One lane per commit.
