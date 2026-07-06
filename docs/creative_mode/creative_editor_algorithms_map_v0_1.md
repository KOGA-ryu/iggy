# Creative Editor — Algorithm Map (v0.1)

Grounded in live recon of the `creative::` kernel + `apps/iggy3d_creative/main.cpp` and an
8-agent research workflow (2026-07-06). Every recommendation is anchored to real `file:line`
seams. **Cut algorithm slices from this map**; version-bump on corrections.

Doctrine held against every entry: generic systems never per-kind; target-first (smallest
algorithm that unlocks the target); leverage lives in the READERS; validate → REPAIR → receipt.

---

## 0. The through-line (read this first)

The kernel already funnels **every** edit through one validated mutation path
(`applyDocumentMutation` → `applyMutation`, DocumentMutation.hpp:128 / MutationApply.hpp:89)
that returns a **dirty-flag receipt** (Transform | Bounds | …) and bumps a monotonic
`revision_`, and it already keeps a **full-document snapshot ring** for undo.

So the editor's job now is **not to add per-kind machinery** — it is to add generic **READERS**
of two things: `CreativeBounds` and the mutation receipt. Pick, snap, cull, overlap-check,
undo/redo, and bake **all reduce to reading the same bounds + occupancy data through different
queries.** That is the doctrine ("leverage in the readers") made literal.

**The single highest-leverage move:** build ONE persistent `CreativeObjectAABBIndex` — a coarse
(8 m) hash-grid keyed by `objectId + CreativeBounds`, updated incrementally off the
`Transform|Bounds` dirty flags every receipt already emits, rebuilt wholesale on undo/redo
snapshot boundaries. It is the shared substrate under **four** otherwise-separate lanes at once:
ray-pick candidate gather, snap-candidate neighborhood search, overlap-at-placement validation,
and per-frame frustum cull.

---

## 1. Reach-NOW algorithms (immediately worthwhile)

Slotting into the user's stated sequence: **A** = descriptor cleanup (a refactor, not an
algorithm — still do it first); the algorithms below land in **B** (delete + pick), **C**
(undo/redo), and a snap slice after.

### Picking (slice B)
- **World-space ray-vs-AABB slab pick** — replaces the projected-center-depth tie-break at
  `main.cpp:2361` that causes the crate-over-floor mis-pick. Nearest ray-entry `tmin` over the
  `visualBoundsForObject` AABBs the loop already computes. Emits one `TargetRef` → existing
  Select path unchanged. Effort **S**.
- **`clipFromWorld` 4×4 inverse** (pixel→world ray) — the one new capability the slab needs
  (app only does world→clip today). Build it first; the frustum marquee later reuses it. **S**.
- **Hover highlight** — same slab pick per-frame (no click), written to the *already-wired*
  `CreativeSelectionState.candidateTarget` via `updateSelectionCandidate`. Nearly free once the
  pick is a reusable `pickClosest(pixel)->TargetRef` helper. **S**.

### Spatial (slice B)
- **`CreativeObjectAABBIndex`** (coarse-super-cell hash grid) — the shared structure above.
  Reuses `worldBoundsToGridBounds`/`toGridIndex`, fed by receipt dirty flags, `rebuildFrom(span)`
  on snapshot boundaries. **M**. Bucket at 8 m (NOT the 1 m occupancy resolution) or a 75×45
  floor bloats the multimap.
- **Keep the O(n) full scan as the correctness oracle** — assert the index returns identical
  pick results before deleting the scan. **S**.

### Undo/redo (slice C)
- **Unify onto one snapshot ring** — collapse the two diverging rings to one owner (the app's
  push-before / discard-on-failure pattern). Kind-agnostic by construction. **S**.
- **Add a redo mirror ring** — the biggest gap: capture `facade.document()` into redo *before*
  `installDocument` overwrites it; clear redo on any fresh push. **S**.
- **Generic `withUndo` wrapper keyed off `Document.revision()` delta** — consolidates the 4×
  copy-pasted helpers (`main.cpp:1032/1058/1352/1382`) so every current + future command inherits
  undo/redo for free. **M**.
- Coalescing is **already free**: Preview mutates only the ghost; the document changes once at
  Commit. One snapshot per commit is automatic — do NOT build command-merge.

### Snapping (slice after B/C)
- **3D-widen the snap primitives** (`CreativeSnapPoint2`→3, add `stepZ/originZ`) — additive,
  z defaults off. Blocks everything vertical. **S**.
- **Surface snap / drop-to-floor** — the dominant box-stacking verb. Reuses
  `pickCreativeViewportCell`'s `HighestZFirst` mode over already-projected occupancy cells; a
  single-column query, NOT the 3×3×3 neighborhood vertex snap needs. Reads only `CreativeBounds`
  → every kind drops-to-floor generically. **M**. Trap: world-down vs grid-stacking-axis adapter
  (test with a two-box fixture).
- **Incremental/relative snap** — snap the delta not the position; `snapScalar` already does
  origin-relative rounding, you only thread the grab-origin. Lands dark. **S**.
- **Pivot snap** = base-of-AABB alignment parameter on surface snap (drop feet-first). **S**.

### Token → ascii → 3D (independent lane — the "minecraft" vision)
The minimum DNA loop (source → generation → validation → decision → report) is **three Small
algorithms**, all riding shipped `ascii_room/` seams:
- **Deterministic token-grammar parser + seeded PRNG** (FNV-1a seed, splitmix64) — the one
  missing arrow: token string → `AsciiRoomCanvas` draw-calls. Establishes the no-global-RNG law
  (app has zero RNG today). **S**.
- **Rectangle/border room shell** — already implemented as `drawAsciiRoom(canvas,0,0,W,H,'.','#')`.
  Zero new geometry. **S**.
- **4-connected flood-fill connectivity validator** — BFS from spawn proves every walkable cell
  is reachable (catches sealed pockets `buildAsciiRoomGrid` does NOT check). REPAIR = knock a
  door glyph, re-parse. **S**.

---

## 2. Dependency-ordered build order

| # | Build | Unlocks |
|---|-------|---------|
| 1 | `clipFromWorld` inverse (pixel→ray) | slab pick + frustum marquee later |
| 2 | World-space slab pick → `pickClosest()` helper + hover | correct single-hit pick (B), free hover |
| 3 | `CreativeObjectAABBIndex` (fed by receipt, `rebuildFrom` on snapshot) | pick gather + overlap + cull + snap-neighborhood — **4 queries, 1 structure** |
| 4 | Undo/redo unify + redo ring + `withUndo` wrapper | robust undo AND redo; the commit template rotate/scale clone |
| 5 | 3D-widen snap → surface + pivot + incremental snap | vertical box-stacking; off-grid-preserving drags |
| 6 | Rotate tool = single-axis screen-angle ring cloning Move-drag + angle snap | rotation authoring (validation/receipt/undo inherited free) |
| 7 | Oriented-box render (8 corners from center+Euler) + ray-vs-OBB pick | rotation becomes **visible**; rotated objects pick true footprint |
| 8 | Scale tool cloning the lifecycle via `resizeDocumentObject`/`SetWidth` (NOT `ScaleMutation`) | visible, bakeable resize |
| 9 | Multi-select kernel seam (`SelectionState` → set + `SetSelectedSet`, update all readers) | marquee + cluster move/delete |
| 10 | 2D projected-rect marquee (candidates from the index) | first region grab |
| 11 | Token→ascii→3D minimum loop (parser → `drawAsciiRoom` → flood-fill → bake) | reproducible token→walkable room round-trip |
| 12 | Greedy meshing + hidden-face culling (Structural-Floor-plane first, quadsBefore/After receipt) | draw-call collapse for big rooms; collision for BoxVolume kinds |

Steps 1–4 are the spine; 5–8 ride the seams they unlock; 9–10 are region-select; 11–12 are
independent lanes (token-gen, bake) that can interleave once the interactive editor stabilizes.

---

## 3. Cross-cutting investments (the highest-leverage pieces)

- **`clipFromWorld` inverse** — one pixel→ray primitive under picking, region-select, and
  vertical snap drag.
- **`CreativeObjectAABBIndex`** — one index serves pick + overlap + cull + snap-neighborhood.
  Same math as the occupancy grid, inverse direction (cell→objects). Does NOT need BVH/octree/k-d.
- **`CreativeDocumentMutationReceipt.dirtyFlags`** — the universal change signal: drives index
  maintenance, undo change-detection (revision delta), and snap-cache invalidation. One receipt,
  three consumers.
- **Begin/Preview/Commit lifecycle + the Move-drag commit branch** — the reusable command
  template. Rotate and Scale clone it wholesale; only the `applyXxxMutation` call at the commit
  seam changes. Undo snapshot, receipt, revision bump, lock refusal all inherited.
- **`snapScalar(value, step, origin)`** — one rounding function serves grid, incremental, size
  (Resize), and angle snap. No new snap primitive per tool.
- **The occupancy grid / `SpatialProjection`** — one voxelization feeds surface-snap columns,
  feature-snap neighborhoods, greedy-mesh bake, and (ray-marched) the eventual broadphase.
- **`AsciiRoomCanvas` draw-call seam** — the generic interface for ALL generation: rect-fill
  today, WFC/BSP later plug into the identical seam.
- **`CreativeBounds` as the universal unit** — pick, index keying, snap features, and bake role
  all read the same AABB, so a new descriptor row inherits picking + snapping + indexing +
  optimized baking with zero per-kind code.

---

## 4. Overkill ledger — what NOT to build (yet), and why

- **Persistent-immutable / structural-sharing document (COW/HAMT)** for "near-free undo" — a full
  document storage rewrite fighting 293 pinning tests, for payoff only past FromSoft-room scale.
  A 32-deep ring of a few hundred flat objects is a few MB. Revisit only if profiling proves it.
- **Inverse-op / command-log** — violates "undo for free": inverse synthesis is per-mutation-kind
  (each of ~20 `applyXxxMutation` needs an inverse; create/delete touch internals the receipt
  doesn't expose). The snapshot ring is already kind-agnostic.
- **BVH / octree / loose-grid / k-d tree / sweep-and-prune** — the coarse hash grid answers pick,
  overlap, cull, and snap-neighborhood at tens-to-low-hundreds of objects (naive break ≈ 2–5k).
  BVH refit degrades under constant editing. k-d snap can brute-force the index's candidate set;
  SAP is the wrong shape for single-selection editing.
- **Frustum-vs-AABB marquee (6-plane sub-frustum)** — returns ~identical results to the cheap 2D
  projected-rect test on this near-top-down axis-aligned editor. Build 2D first; escalate only if
  users see it grab objects behind a wall.
- **Möller-Trumbore ray-vs-triangle** — no live per-object triangle mesh exists (visualBounds are
  proxies). AABB picking IS the correct precision for descriptor-box primitives. Defer until
  MeshProxy carries real geometry.
- **Quaternion / axis-angle / arcball rotation** — gimbal lock only bites when *composing*
  multi-axis Euler rotations; the single-axis ring overwrites one component and never composes, so
  it structurally cannot gimbal-lock. Quats would force a new type through
  Object/Serialization/undo/bake/snap for a problem the target doesn't have. Arcball also hides
  which axis rotated — fights the provable-receipted-edit doctrine.
- **Scale via `ScaleMutation`/`transform.scale`** — nothing reads it (renderer + projection read
  bounds). Invisible, un-baked. Use `Resize`/`SetWidth` bounds verbs.
- **Wave Function Collapse for "fromsoftleveldesign"** — the bait facet. Needs a constraint
  surface + backtracking + per-style rules nothing downstream requires, and forces you to freeze
  the token's meaning before one token has round-tripped. The bake path is identical whether a
  1-line `drawAsciiRoom` or 400-line WFC produced the ascii, so adopting it later is a contained
  swap behind the canvas seam. Build the boring rect-fill loop first; watch what real tokens ask.
- **BSP / cellular-automata cave gen** — blocked, not deferred: the ascii module is
  rooms-not-dungeon with no room-linking. They'd generate ascii the bake path can't connect.
- **Marching-squares / decimation / LOD / real CSG** — CSG is free from the 1 m grid (overlapping
  axis-aligned boxes light the same cells = union at zero robustness cost). Decimation/LOD have no
  creative mesh identity (`MeshResourceId` doesn't exist) and no LOD draw path. Marching-squares
  is a render-at-scale bake optimization misfiled as generation.

---

## 5. GAPS — families the seven lanes missed (the strategic section)

The editor authors **geometry** but never proves the world is **playable for the game that
already ships**. These are the cross-lane seams (creative ↔ AI ↔ movement) — the "map affordance
vocabulary" (stream A7) made concrete:

1. **Navmesh authoring & validation** — the shipped stealth AI (LOS/patrol/last-known) and the
   parkour thief all need a walkable surface graph; the editor has no way to derive or validate
   one. Derive a nav-grid from baked floor surfaces (greedy-meshed walkable quads are the natural
   input) + a reachability pass (connected-components flood-fill) so the author is told "this
   platform is an unreachable island" **at bake time**, not by a stuck NPC. *Biggest gap.*
2. **Parkour / traversal-connectivity graph** — the thief's reachability is a jump/climb/vault
   graph between ledges, not the NPC walkable graph. Validate "can the thief get spawn→objective
   using the movement verbs," edges gated against the M&A lane's gravity=data + climbing-guard
   params. A room can be geometrically valid but parkour-impossible.
3. **Line-of-sight / visibility precompute for authoring** — cover-point detection (where can the
   thief hide from a cone?), sightline preview (what does a guard see from this waypoint?), coarse
   cell-to-cell PVS for designing ambush sightlines. The occupancy grid already gives the raycast
   substrate.
4. **Patrol-route & waypoint authoring + validation** — the `Path` shape kind exists; the gap is
   the validator proving a patrol path lies on walkable cells and forms a legal loop (same
   reachability machinery as navmesh).
5. **Intel-packet / notebook extraction** — the game's core interface is the thief's notebook
   transferring intel to the knight+priestess duo. Missing: a query/serialization pass turning
   authored guard cones + patrol routes + cover points into the notebook data structure. This is
   the asymmetric-co-op seam.
6. **Multi-room connectivity / portal graph** — room-graph + door-linking + cross-room
   reachability. Blocks BSP/CA gen; own roadmap stream.
7. **Overlap / validation audit as a first-class pre-bake gate** — document-wide: intersecting
   structural volumes, floating (non-grounded) objects, degenerate zero-size bounds, orphaned
   markers → repairable receipts. The natural home for the validate→REPAIR→receipt doctrine.
8. **Diff / merge of documents** — generate a room, hand-tune it, regenerate: no way to re-apply
   hand edits over a regenerated base. Lower priority, real once gen + manual authoring coexist.

**The reframe:** items 1–5 mean the editor's real job isn't "place boxes" — it's *author a world
the shipped AI and movement systems can be proven to inhabit.* The greedy-meshed walkable quads
(step 12) are the input to the navmesh (gap 1); flood-fill reachability is the shared validator
across nav, patrol, and parkour. That flood-fill you build for token-gen connectivity (step 11)
is the same algorithm — build it once, reuse it four ways.

---

## 6. Model corrections (what recon changed)

- **Undo is already half-built**: TWO diverging snapshot rings (kernel
  `CreativeDocumentUndoStack` pops-on-apply + forbids redo; app `StandaloneUndoStack`), NO redo.
  Slice C = unify + add redo, not build-from-scratch.
- **Rotate/Scale kernel is DONE**: `CreativeMutationKind::Rotate/Scale`, `applyRotate/ScaleMutation`
  (absolute-set), `rotate/resizeDocumentObject`, `descriptorAllowsMutation` per-kind gate — all
  built + tested. Gap is only the app gizmo. And `transform.rotation`/`.scale` are **unread** →
  scale via `resizeDocumentObject`, rotate needs oriented-box render to be visible.
- **The ascii→3D pipeline ships** (`src/app/iggy3d/ascii_room/`): `AsciiRoomCanvas` (draw
  primitives) → `parseAsciiRoomCanvas` → `buildAsciiRoomGrid` (validator + receipt) →
  `AsciiRoomToRoomAsset` (bake). Token-gen = one arrow (token → canvas draw-calls).
- **The pick has a real bug**: center-depth tie-break steals clicks (crate-over-floor). Fix =
  world-space slab (needs the `clipFromWorld` inverse).

---

*Appendix (per-family algorithm tables with effort/reach/gotcha) available in the workflow
result; distilled above.*
