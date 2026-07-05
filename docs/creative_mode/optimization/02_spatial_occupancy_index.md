# 02 — Spatial Occupancy Index

> ↔ research `02_spatial_bounds_index.md`. Lane: **[C]**. This is the plan's
> **L4 `CreativeOccupancyIndex`** (name reserved in the foundation plan; W8a).
> The first implementation packet of the program — everything else queries it.
>
> Relationship to the existing `SpatialProjection`: `projectObjectsToGrid`
> (`creative/SpatialProjection.hpp:127-147`) stays the PURE per-object
> projection function. The index is the INCREMENTAL CONTAINER built from the
> same projection semantics, updated by dirty drain instead of recomputed per
> query. Today the pick path re-projects the whole document on every click
> (`window/Loop.cpp:66-74`, grid {64,64,8}, cellSize 1.0 per call) — that full
> scan is what this packet retires.

## Entry criteria

- W2 landed: document-owned `CreativeGridSettings` (cellSize on the document,
  never per call — D3/LAW-18) and the L0 world-math header (AABB helpers).
- W1 landed: D8 semantics in mutations (Move translates bounds — LAW-5). The
  index would otherwise faithfully index lies.
- Doc 01 S1 landed: `boundsRule` + `bakeDomains` columns exist.
- W1w landed or in flight: pick coordinate law (LAW-1) — the DDA pick slice
  (S5) plugs into the REPAIRED click path, not the broken one.

## Frozen interfaces

```cpp
// creative/OccupancyIndex.hpp (new; the reserved L4 name)

struct CreativeSpatialProxy {
    CreativeObjectId id = 0;                 // u64 — LAW-19
    CreativeObjectKind kind = CreativeObjectKind::Unknown;
    CreativeAabb3 trueBounds;                // from boundsRule (doc 01)
    CreativeAabb3 fatBounds;                 // trueBounds inflated kFatBoundsCells * cellSize (LAW-8)
    CreativeBakeDomainMask domains = 0;      // capability filter (doc 01)
    std::uint32_t lastVisitStamp = 0;        // generation dedup (LAW-7)
};

struct CreativeOccupancyIndexConfig {
    double cellSize = 1.0;                   // COPIED from CreativeGridSettings at rebuild;
                                             // receipt carries the value. Never a parameter
                                             // on query calls.
};

class CreativeOccupancyIndex {
public:
    // Mutation surface — called ONLY from the dirty drain (doc 05) and rebuild.
    CreativeOccupancyUpdateReceipt insertOrUpdate(const CreativeObject&,
                                                  const CreativeObjectDescriptor&);
    CreativeOccupancyUpdateReceipt remove(CreativeObjectId);
    CreativeOccupancyRebuildReceipt rebuildFrom(const CreativeDocument&);

    // Query surface — const, allocation-free after warmup, any caller.
    void queryAabb(const CreativeAabb3&, CreativeBakeDomainMask required,
                   std::vector<CreativeObjectId>& out) const;
    void queryPoint(CreativeVec3, CreativeBakeDomainMask required,
                    std::vector<CreativeObjectId>& out) const;
    CreativeRayPickResult rayPick(const CreativeRay&, CreativeBakeDomainMask required,
                                  double maxDistance) const;

    const CreativeOccupancyStats& stats() const;
};
```

Preconditions / postconditions:

- `queryAabb`: input bounds must be normalized (min ≤ max per axis). Non-finite
  or inverted input → empty result + stats counter `queriesRejectedInvalid`
  (repair is the CALLER's job at the tool boundary; the index never guesses —
  LAW-16 applied as reject-with-receipt because this is an internal API, not an
  authoring surface).
- Output is sorted ascending by object id (stable order for tests and diffing).
- `insertOrUpdate` with an object whose descriptor lacks `Spatial` domain →
  no-op receipt `skipped_not_indexed` (NOT an error — editor flows call it
  uniformly).
- All query methods are `const` and thread-agnostic (no interior mutability
  except the visit stamp, which lives on proxies owned by the mutation surface;
  see §Dedup for the const-correct arrangement).

Receipts (the packet's report DNA):

```cpp
struct CreativeOccupancyUpdateReceipt {
    CreativeOccupancyUpdateStatus status;    // Inserted, Updated, Removed,
                                             // SkippedNotIndexed, SkippedUnchanged,
                                             // RoutedLargeObject, RejectedInvalidBounds
    CreativeObjectId id = 0;
    std::uint32_t cellsBefore = 0;
    std::uint32_t cellsAfter = 0;
    bool fatBoundsHeld = false;              // true => zero cell churn (LAW-8 proof)
};
```

## Storage spec

Sparse hash grid (LAW-6). No dense array, no minCoord/extent world bound.

```cpp
// key: packed signed coords, 21 bits per axis, bias 2^20
[[nodiscard]] constexpr std::uint64_t packCellKey(std::int32_t x, std::int32_t y,
                                                  std::int32_t z);
// valid range per axis: [-1048576, 1048575]; outside => RejectedInvalidBounds
// receipt at insert (an object 4 million cells from origin is authored garbage;
// repair belongs upstream at the mutation boundary, the index refuses politely).
```

- `std::unordered_map<std::uint64_t, CreativeCellBucket> cells_` where
  `CreativeCellBucket { std::vector<CreativeObjectId> ids; }` — ids kept
  sorted-insert (buckets are small; sorted keeps query output ordering cheap).
- `std::unordered_map<CreativeObjectId, CreativeSpatialProxy> proxies_`.
- `std::unordered_map<CreativeObjectId, std::vector<std::uint64_t>> memberships_`
  — the reverse map. Non-negotiable (research 02 is right): removing without it
  means scanning every cell.
- `std::vector<CreativeObjectId> largeObjects_` — LAW-9 side list, membership
  threshold `kMaxCellsPerObject = 128`, checked linearly by every query, with
  `stats_.largeObjectChecks` counting the cost so the threshold can be tuned
  from evidence.
- `mutable std::uint32_t queryStamp_` + stamps on proxies for dedup (LAW-7).
  Const-correctness: stamp fields are `mutable std::uint32_t lastVisitStamp`
  on the proxy — the ONE sanctioned mutable in this type; documented there.

Cell coverage: LAW-3. `cellOf(v) = (int)std::floor(v / cellSize)` per axis —
`floor`, never `int()` truncation (negative coords). Coverage of an AABB is the
inclusive coord box `cellOf(min) .. cellOf(max)`. Boundary rule: `max` exactly
on a boundary stays in its floor cell (arithmetic consequence of floor — no
epsilon anywhere in coverage math; epsilons live only in `boundsRule` repair).

## Algorithm specs

### insertOrUpdate

```
1. descriptor.bakeDomains lacks Spatial        -> SkippedNotIndexed
2. bounds := boundsFor(object, descriptor)      // boundsRule column, doc 01;
                                                // degenerate -> repaired + receipt upstream
3. bounds invalid/non-finite                    -> RejectedInvalidBounds
4. existing proxy && bounds inside fatBounds    -> update trueBounds only,
                                                   SkippedUnchanged (fatBoundsHeld=true)
5. coverage := cells(fat(bounds))               // fat at insert, LAW-8
   |coverage| > kMaxCellsPerObject              -> remove grid memberships,
                                                   move to largeObjects_, RoutedLargeObject
6. remove old memberships (reverse map), insert new, update proxy+reverse map
   -> Inserted / Updated with cellsBefore/After
```

Complexity: O(cells touched). Memory: proxies are SoA-hostile as a map — fine
at first; OP7-class revisit only if `proxies_` lookup shows up in a measured
profile (LAW-6 discipline: no speculative SoA).

### queryAabb

```
1. validate; ++queryStamp_ (wrap: if 0 after ++, clear all stamps once — the
   4-billion-query wrap is a real bug in stamp schemes; handle it, test it)
2. coord box := cells(bounds)                   // TRUE query bounds, not fat
3. for each cell key in box: for each id in bucket:
     proxy.lastVisitStamp == queryStamp_ ? skip : stamp it
     (proxy.domains & required) != required ? stats_.maskRejects++ : 
     !overlaps(proxy.trueBounds, bounds) ? stats_.exactRejects++ :
     out.push_back(id)
4. largeObjects_ pass: same filter chain, stats_.largeObjectChecks++
5. sort(out)   // buckets sorted makes this near-sorted; std::sort is fine
```

Broad phase only — callers do narrow checks (LAW; research 02's bouncer line
stands). The index never answers "does it collide", only "who is close".

### rayPick — 3D DDA (LAW-2; replaces the z-plane)

Amanatides–Woo traversal:

```
1. cell := cellOf(origin); tMax[axis] := t to first boundary per axis;
   tDelta[axis] := cellSize / |dir[axis]| (INF for zero components)
2. loop until t > maxDistance or maxSteps (kMaxPickSteps = 4096, receipted):
   a. candidates := bucket(cell) + largeObjects_ (stamped, mask-filtered)
   b. for each: slab-test ray vs proxy.trueBounds -> hit t
   c. hits found in this cell with t <= t_cellExit: return nearest t;
      tie -> lowest id (LAW-2 determinism)
   d. advance along smallest tMax axis
3. no hit -> Miss receipt with cellsVisited count
```

The per-cell cutoff (2c: only accept hits with t ≤ cell exit t) is what makes
DDA correct — an object registered in a later cell must not win against a
nearer object in the current cell. State this in the code comment; it is the
one subtle line in the whole packet.

Narrow phase here is ray-vs-AABB slab only. Ray-vs-actual-shape stays with
future narrow systems; the pick UX needs cells + AABBs, nothing more yet.

## Golden cases (verbatim into tests)

cellSize 1.0 unless stated. Coords are (x,y,z).

1. **Coverage basic:** bounds [1.2,0.0,3.9]→[5.1,2.0,4.0] → coord box
   (1,0,3)→(5,2,4): 5×3×2 = **30 cells**.
2. **Boundary-exact max:** bounds [0,0,0]→[2.0,1.0,1.0] → coord box
   (0,0,0)→(2,1,1) — max 2.0 floors to cell 2: **3×2×2 = 12 cells**. (LAW-3:
   floor of the max, inclusive; 2.0/1.0 → cell 2.)
3. **Negative floor:** point (-0.25, 0.5, -3.0) → cell (-1, 0, -3). NOT (0,0,-3)
   — truncation is the bug this case exists to catch.
4. **Fat-bounds hold:** cellSize 1, kFatBoundsCells 0.5. Insert crate bounds
   [0.2,0,0.2]→[0.8,1,0.8] (fat [-0.3,-0.5,-0.3]→[1.3,1.5,1.3]); move by
   (+0.3,0,0) → new true bounds inside fat → receipt SkippedUnchanged,
   fatBoundsHeld=true, cellsBefore==cellsAfter. Move by (+2,0,0) → reinsert.
5. **Dedup:** one object spanning 4 cells, queryAabb covering all 4 → id appears
   **once**; stats duplicateSkips == 3.
6. **Mask filter:** Crate (Spatial|Render|Collision) and Note (Spatial) in the
   same cell; queryAabb(required=Collision) returns Crate only; maskRejects==1.
7. **DDA order:** objects A bounds [2,0,0]→[3,1,1] and B [5,0,0]→[6,1,1]; ray
   origin (0,0.5,0.5) dir (+1,0,0) → hit A at t=2.0, never B. Reverse the ray
   from (8,0.5,0.5) dir (-1,0,0) → hit B at t=2.0.
8. **DDA cell-exit cutoff:** A [0.9,0,0]→[1.1,1,1] (spans cells 0 and 1 in x),
   B [0.4,0,0.4]→[0.6,1,0.6]; ray (0,0.5,0.5)→(+1,0,0): B (t=0.4) wins over A
   (t=0.9) even though A was stamped while visiting cell 0.
9. **Out-of-range insert:** bounds min.x = -3.0e6 → RejectedInvalidBounds
   receipt; index unchanged; stats counter bumps. (LAW-4: no relocation.)
10. **Large object:** room bounds spanning 200 cells with threshold 128 →
    RoutedLargeObject; queryAabb anywhere inside still returns it (side-list
    pass); stats.largeObjectChecks > 0.

## Repair rules

| Condition | Action | Receipt |
| --- | --- | --- |
| degenerate bounds (min==max on an axis) | repaired UPSTREAM by boundsRule (handle extent) before reaching the index | `bounds_repaired_degenerate` (doc 01) |
| inverted bounds (min>max) | index rejects; mutation boundary owns the repair (normalize + receipt) — W1 territory | `RejectedInvalidBounds` |
| coord out of packed range | reject, count | `RejectedInvalidBounds` |
| stamp wraparound | clear stamps, continue | stats `stampResets` |

## Slices

- **S1 — L0 math.** AABB normalize/overlap/expand + `cellOf`/coverage + packCellKey
  in the W2 world-math header. Golden cases 1–3, 9 (math half). Files: the L0
  header + its test. Forbidden: everything else.
- **S2 — proxy builder.** `boundsFor(object, descriptor)` via `boundsRule`
  column + proxy construction. Golden: per-rule bounds cases incl. thin epsilon
  + handle extent. Needs doc 01 S1.
- **S3 — storage.** Buckets, reverse map, insertOrUpdate/remove/rebuildFrom with
  receipts, fat bounds, large-object routing. Golden 4, 9, 10.
- **S4 — queries.** queryAabb/queryPoint + stamps + wrap handling. Golden 5, 6.
- **S5 — DDA rayPick + wiring.** rayPick per spec; then the pick frame consumes
  the index instead of per-click projection: `CreativeViewportPickFrame`
  swaps `projectObjectsToGrid` + `pickCreativeViewportCell` for
  `index.rayPick(rayFromNormalizedClick(...))`. Depends on W1w's coordinate
  repair (LAW-1) — the ray construction consumes the NORMALIZED click. The old
  z-plane path is deleted, not gated. Golden 7, 8 + a live-entry-point pick test
  at 1x and 2x (shares W1w's gate fixture).
- **S6 — dirty wiring + metrics.** Index updates driven by the W4 Spatial
  channel drain (doc 05); full `stats()` surface receipted into the existing
  stats pattern. Until W4 lands, `rebuildFrom(document)` on revision change is
  the sanctioned interim (receipted `rebuild_reason=revision_poll`) — remove it
  in this slice.

Each slice: full ctest green; tests named `creative_occupancy_index_*`;
one lane [C]; forbidden files listed per slice above.

## Metrics (stats surface, all receipted)

objectCount, indexedProxyCount, largeObjectCount, avgCellsPerObject,
maxCellsPerObject, queryCount, candidatesPerQuery (avg), maskRejects,
exactRejects, duplicateSkips, fatBoundsHolds vs reinserts, largeObjectChecks,
stampResets, pick cellsVisited (avg/max), queriesRejectedInvalid.

The fatBoundsHolds:reinserts ratio during a drag is THE stability metric —
if it is not overwhelmingly holds, kFatBoundsCells is wrong; tune with evidence.

## Non-goals

Exact collision, navmesh, frustum culling, trigger dispatch, rendering,
persistence of the index (it is derived data — NEVER serialized; rebuilt from
the document on load), BVH/octree (OP7, metric-gated).

## RECON items

- RECON-02a: confirm `CreativeGridSettings` final shape as W2 lands (cellSize
  type double? per-axis? this doc assumes cubic per D3) — `observed`.
- RECON-02b: exact current pick call chain at HEAD post-W1w (line anchors for
  S5's swap) — `observed`.
- RECON-02c: does `CreativeRay` exist anywhere, or does S5 define it (origin +
  unit dir, doubles)? Check map_maker fly camera for an existing ray type first
  — do not create a duplicate.
