<!-- Kernel Architecture v0.1 — 2026-07-06. Pure/generic/deterministic engines the repo HAS/NEEDS/WANTS, derived from docs/lane_function_contracts_v0_1.md. For review; not committed, not sent to Codex. -->

# iggy3d Kernel Architecture — the assembled down-layer

**Derived from** `docs/lane_function_contracts_v0_1.md` (51 functions across 5 lanes, grep-grounded). A KERNEL here is a pure, generic, deterministic computational engine that many features route through — the doctrine's "leverage lives in the READERS" made literal. Kernels take explicit inputs, return a value/receipt, touch NO session/global state, NO RNG, NO Date, and are testable headless. A new object KIND touches a kernel ONCE (a descriptor row / a data value), never forks it.

## Headline

51 functions collapse into **12 kernels**. The through-line is the **Descriptor table**: one constexpr row-per-kind (`describeObject`) is the spine that six down-lane kernels read instead of switching on kind. A new object KIND touches ONE row and inherits taxonomy, projection, occupancy, anchor semantics, bake role, palette membership, dirty-flags, spawn defaults, and capability bits everywhere at once — ~110 would-be switch ladders collapsed into rows.

The tier split is honest and front-loaded. **6 HAS** kernels already ship pure and test-pinned; the anchor sub-seam of the Creative→AI bridge is CLOSED, not broken. **3 NEEDS** kernels stand between here and the first playable slice, two of them extraction/wrapper over truths that already compute silently. **3 WANTS** kernels are reserved sockets that stay unbuilt until a measured or structural trigger fires.

The two real gaps the whole architecture pivots on: `buildReasoningGraph` has **NO production call site** (only tests fill `setReasoningGraph`), and the **ReconIntel notebook packet does not exist** in `src`. So the slice is unblocked at the head (`projectGuardRecon` reads durable `AiActorState` that already carries every field) but its floor-plan half waits on the L2 affordance wire-strings.

## Tier tally

| Tier | Count | Kernels |
|---|---|---|
| **HAS** | 6 | Descriptor, Mutation, Bake, Spatial-occupancy, Reasoning-graph, Persistence & Identity |
| **NEEDS** | 3 | validateDocumentPreBake, floodFillReachability, Recon spine (projectGuardRecon + captureReconIntel + buildAuthoringHandles) |
| **WANTS** | 3 | CreativeObjectAABBIndex, Visibility/LOS, transferReconIntel |

(The Recon spine and handle extraction are counted as the NEEDS work-cluster; the catalog below lists all twelve kernels individually.)

## Kernel catalog

### 1. Descriptor kernel — `describeObject(CreativeObjectKind)` — HAS
The exemplar read-table. `src/app/iggy3d/creative/document/ObjectDescriptor.cpp:1892`. A constexpr `std::array` (kDescriptors) concatenated from per-category sections; the only runtime work is a linear scan on `candidate.kind == kind`. Total function — an unmapped kind repairs to `kDescriptors.front()` (Unknown). Includes only `<array>`/`<cstddef>`; reads the KIND, never the object's data.
- **Purity boundary:** may NOT touch the live object, Document, selection, undo rings, dirty channels, renderer, save codec.
- **Absorbs:** taxonomy (categoryOf/profileOf), spatial policy (projectionProfile/occupancyKind), mutation legality+effect (descriptorAllowsMutation/dirtyFlagsForMutation — the TD-2 bounds-only-Move rule), creation dirty channels + spawn defaults, runtime export semantics (runtimeAnchorSemantic — FIRST link of the closed anchor seam), palette membership. Six subsystems, one row.
- **Invariant to harden:** there is NO `static_assert(kDescriptors.size() == count-of-kinds)` today. A new enum value with no row silently degrades to Unknown and vanishes from palette/bake/graph. **The totality test — `describeObject(k).kind==k` for every k — IS the missing guard.**
- **Touch-count:** new kind = 1 row (replaces ~10 switch sites).

### 2. Mutation kernel — `applyDocumentMutation` — HAS
The sole edit seam. Every verb (rotate/resize/setBounds/setTransform) and every tool routes through it; the Descriptor legality gates consume `categoryOf(mutationKind)/canMutate/isTransformMutation`. Document it as the one place the document changes, so no verb forks it. Prerequisite for handles and validation-repair.

### 3. Bake kernel — `buildRoomAssetFromCreativeDocument` — HAS
`src/app/iggy3d/creative/adapters/RoomBake.cpp:529`. Pure fold of a CreativeDocument into a RoomAsset + decision receipt: one classify-then-emit pass, descriptor-driven, one row per kind. Wired to two production call sites (`Operations.cpp:693`, `StandaloneRoomBakePreview.cpp:67`).
- **Anchor sub-seam CLOSED** at `:253`: `anchorKindForDescriptor` returns `toString(runtimeAnchorSemantic)`, cased downstream by `nodeKindForAnchor` (`ReasoningGraph.cpp:26`).
- **Two PARTIAL legs (sockets cut, unfilled):** link emission (NavLink/JumpLink/ClimbLink → RoomOpeningAsset + Opening surfaces) currently falls through to SkipUnsupportedShape — this is the live cross-lane seam break; and rotation-honoring geometry (`transform.rotation` is UNREAD — rotated objects bake as axis-aligned bounds).
- **Invariants:** determinism (byte-identical RoomAsset+receipt); conservation (considered == baked + Σ skips); no degenerate geometry escapes (validBakeBounds); anchor kind string == toString(semantic), no free-string.
- **Touch-count:** new kind = 1 descriptor row, 0 edits to RoomBake. New emit FAMILY (openings/links) = ~4 sites once (enum value + classify branch + emit helper + receipt counter).

### 4. Spatial-occupancy kernel — `projectObjectsToGrid` — HAS
`src/app/iggy3d/creative/spatial/SpatialProjection.cpp:749`. Stamps each object's world extent into occupancy cells `{gridIndex, coord, objectId, objectKind, occupancyKind}` and merges into one field + receipt. The 6 per-profile projectors (Point/Box/Volume/Line/Path/Link) collapse into ONE dispatch keyed by `descriptor.projectionProfile` — branched per SHAPE PROFILE, never per KIND. Two live callers (ViewportPickFrame, DocumentWireframe).
- **The ONE extraction gap:** `indexOccupancy(cells) -> CreativeOccupancyIndex` (gridIndex→occupant, O(1)) — the flood/pick readers need cell→occupant, which the flat list does not give. Must define occupancy conflict resolution (priority-by-occupancyKind or last-writer).
- **Touch-count:** new kind = 1 descriptor row (projectionProfile + occupancyKind fields).

### 5. Reasoning-graph kernel — `buildReasoningGraph` — HAS
`src/runtime/ai/ReasoningGraph.cpp:149`. Derives a sparse deterministic graph of meaningful positions (anchor+waypoint nodes, actor-blocking-clear walkable edges) as a pure function of one RoomAsset + explicit patrol waypoints, built once at room-load. `nodeKindForAnchor` is the single anchor-string→ReasoningNodeKind table. `reasoningSegmentBlocked` is the ONE occlusion discipline shared by vision and hearing.
- **THE GAP:** `setReasoningGraph` (`Session.cpp:1308`) has NO production caller — only tests fill it. The kernel + its L5 route reader (planRoute → GuardDecision → stepGuard) are LIVE; the fill wire at room-load is the single missing arrow. **This is the #1 slice unblocker.**
- **Append-only enums** (ReasoningNodeKind 14, ReasoningEdgeKind 7) — they serialize into the notebook later, never reorder.
- **Touch-count:** new affordance kind = 1 row in nodeKindForAnchor (0 if the enum value already exists).

### 6. Persistence & Identity kernel — `computeStateHash` / `encodeSaveEnvelope` / `installDocument` — HAS
One hash seam + one codec seam + one restore seam. `computeStateHash` (`StateHash.cpp:101`) is the sole determinism digest (float-quantized for platform stability); encode/decodeSaveEnvelope own all 14 sections; `installDocument` (`Facade.cpp:785`) is the sole validated whole-document swap; the snapshot ring (`pushCreativeUndoSnapshot`/`applyLastCreativeUndoSnapshot`) is the undo path.
- **The ONE real gap:** REDO IS MISSING — the ring is push + pop_back only; applyLast drops the snapshot. Copy the existing push/pop ring for the mirror.
- **Reserved socket:** a `SaveReconIntelSection` so the intel packet routes its durable form + fingerprint through THIS kernel, not a forked second persistence path.
- **Touch-count:** new hashable field = 1 addField line; new savable object property = 1 field + 1 encode/decode line (replaces 3-5x per-reader fanout).

### 7. validateDocumentPreBake + CreativeValidationDiagnostic — NEEDS (extraction)
`(const CreativeDocument&) -> CreativeValidationReceipt{passed, reasonCode, diagnostics[], counts}`. One pure validate→REPAIR-hint→receipt pass, ZERO mutation — emits `suggestedRepair` as data (a CreativeMutationRequest the caller may route through applyDocumentMutation), never applies it.
- **~4 of 6 checks are extraction/delegation:** checkDegenerateBounds (extract `validBakeBounds` — bake MUST keep calling the shared predicate so they never drift), checkAnchorsDropped (surface the silent SkipUnsupportedAnchor counter), checkStaleParentIds (DELEGATE to `validateCreativeObjectParentGraph`), checkFloating/checkOverlap (new readers over existing projectObjectsToGrid). Precedent to copy: AsciiRoomDiagnostic.
- **Why NEEDS:** the safety net at the authoring→runtime boundary — stops bake from SILENTLY deleting the guard's spawn/patrol anchors and degenerate geometry.
- **Invariant:** overlap/floating emit "warning" severity and never flip `passed` to false (intentional nesting must not block bake).
- **Touch-count:** new kind = 0 (generic over descriptor/bounds/occupancy); new CHECK = 1 predicate.

### 8. floodFillReachability — NEEDS (new-engine)
`(OccupancyGrid, seeds, cfg) -> ReachabilityReceipt{reached bitset, components, singleIsland, receipt}`. Pure 4/6-connected BFS; the caller projects first and hands in the finished grid. Grep-confirmed ABSENT (zero grid flood in src). Copy planRoute's determinism discipline — index-keyed vectors/bitsets, NEVER unordered_set (hash-iteration order breaks determinism).
- **Absorbs 4 would-be BFS reimplementations:** token-gen connectivity, navmesh island labeling, patrol-loop validation, parkour reach.
- **Invariants:** seeds ⊆ reached ⊆ walkable; reordering seeds yields the same reached set; FourWayXZ must be explicit about the ground plane so floors don't connect through vertical gaps.
- **Touch-count:** new kind = 0 (walkability flows through occupancyKind); new reader = 0 (supplies its own grid + seeds).

### 9. Recon-projection kernel — `projectGuardRecon` — NEEDS (new-engine, UNBLOCKED)
`(AiActorState, guardPos, AlertProfile, ReasoningGraph) -> GuardReconObservation`. Pure — the guard position is PASSED IN (never read off an Entity); the tick reported is copied verbatim. Head of the notebook chain. Copy `summarizeReasoningGraph`'s pure shape/purity/test style.
- **UNBLOCKED:** durable AiActorState already carries alertLevel, patrolWaypoints/Mode/TargetIndex, facingDirection, lastKnownTargetPosition, searchChosenNodeId. `alertBandIndex` + `aiBehaviorKindName` exist; the reasoningGraph is populated once rank-7 wires it.
- **The ONE missing piece:** `alertBandName` — the 0..5 → {idle,observant,suspicious,searching,alert,combat} row (only alertBandIndex exists today). Add it beside alertBandIndex; adding a band touches that one row, never a caller.
- **Invariant:** node-id resolution is a coincidence test only (position within epsilon of a graph node) — kNoNode when none, NEVER fabricates a node. Empty graph → all kNoNode.

### 10. captureReconIntel + hashReconIntel + serializeReconIntel — NEEDS (new-engine)
Aggregate N GuardReconObservations + RoomFacts into ONE hashable/serializable ReconIntel value. `hashReconIntel` folds over StableHasher (copy the AiActorState discipline); `serializeReconIntel` copies the SaveAiActorRecord byte-encoder (`SaveCodec.cpp:1142`). The PRODUCER that replaces the notebook test fixture `sampleReconPage`.
- **Graceful degradation is a design invariant:** the guard half ships from real state NOW; RoomFacts (floor-plan/garrison/patrols/hazards wire-strings) arrives EMPTY until the L2 affordance seam (contract rank 14) lands, so ReconIntel must render valid as a blank sketch.
- **Round-trip:** deserialize(serialize(x)) == x, hash(x) == hash(round-trip(x)). Order-sensitive fold (swapping two guards changes the hash — no set-semantics).
- **Touch-count:** new guard-fact kind = 1 field + 1 line each in fold/encode/decode, all co-located (mirrors SaveAiActorRecord).

### 11. buildAuthoringHandles / pickAuthoringHandle — NEEDS (extraction)
`(CreativeObject, clipFromWorld, w, h) -> vector<AuthoringHandleHit>` + `pickAuthoringHandle(...) -> bool`. Enumerate interaction handles (box corners/faces, line endpoints, path waypoints, rotate-ring/axis) as screen-space AABBs/segments with depth; dispatched on `shapeKindForObject`, never per concrete kind.
- **HAS as extraction:** the pure engine already ships battle-tested in `apps/iggy3d_creative/StandalonePicking.cpp` — but trapped in the `iggy3d_creative_app` binary namespace, and only the Path arm is fully realized. The rotate/axis arm is only the inline `pickGizmoAxis` lambda in main.cpp. Target home: `creative/spatial/AuthoringHandles.{hpp,cpp}` beside Snap.
- **Invariant:** slotIndex is stable and shape-defined so a reader can round-trip it back to the Mutation kernel; handles behind camera (clipW<=0) are invalid and never picked. Extraction MUST NOT change the corner-bit convention or the y-flip — the headless test pins exact pixels.
- **Touch-count:** new authorable shape kind = 1 case in the build switch (pick is already generic over HandleRole); replaces 6 today.

### 12. CreativeObjectAABBIndex — WANTS (extraction)
Coarse hash-grid over object AABBs + 4 query verbs (ray/overlap/frustum → id-sorted candidate set; caller runs the exact narrowphase). Copy `collectPhysicsBroadphasePairs` (`PhysicsBroadphase.hpp:50`) — ~80% exists.
- **Superset guarantee:** candidates ⊇ every true intersection — never drops a real hit, only admits false positives the narrowphase rejects. Results identical to brute-force, just faster.
- **Why WANTS:** at slice scale the brute-force E42 ray loop is correct and fast — NOT on the critical path.

### 13. Visibility / LOS kernel — WANTS (new-engine)
`buildVisibility(colliders, samplePoints, cfg) -> VisibilityReceipt` + `previewSightlineFan` + `cellPvs`. Pure symmetric point-to-point visibility, wrapping `reasoningSegmentBlocked`. Lift kEyeHeightMeters/kOcclusionMarginMeters into explicit VisibilityConfig. MUST NOT take a facing/cone — that is the runtime layer's `targetWithinVisionCone`; author-time visibility is facing-free exhaustive LOS.
- **Absorbs** cover-point detection, guard-sightline preview, cell-to-cell PVS — three products, one occlusion contract, one collider bake.
- **Near-slice arm:** `previewSightlineFan` (a designer places the guard blind without it). PVS is deferred.

### 14. transferReconIntel — WANTS (reserved-socket)
Fold the thief's scouted state into ONE hashed, role-addressed ReconIntel packet and deliver to a named roster slot. Pure (SessionState-in, receipt-out). Copy the reasoningGraph **transient-in-persistence slot** precedent (`SessionState.hpp:103-110`) — lives outside `transient`, absent from StateHash/SaveCodec, re-derived on load.
- **DEAD-END today:** zero priestess/knight/spectre in src; PlayerSlotKind is a control-transport enum (Local/Remote/Ai/Observer), NOT a game role → the duo role MUST be a distinct field. Status ConsumerRoleMissing until the socket is filled.
- **The notebook-fill half (transfer-to-self) is promoted into captureReconIntel NOW;** the cross-actor hand-off waits on the second actor.

## Dependency DAG (the load-bearing edges)

```
Descriptor  <──>  Mutation                         (mutual: table reads legality; verbs read the table)
Descriptor  ──>  Bake  ──>  Reasoning-graph  ──>  Recon-projection  ──>  captureReconIntel
Descriptor  ──>  Spatial-occupancy  ──>  floodFillReachability  ──>  validateDocumentPreBake
Descriptor + Mutation + Snap  ──>  buildAuthoringHandles
Mutation + validateDocumentPreBake  ──>  Persistence & Identity
captureReconIntel + Reasoning-graph + Persistence + PlayerRoster + duoRole(socket)  ──>  transferReconIntel
reasoningSegmentBlocked + collider-bake + Spatial-occupancy  ──>  Visibility/LOS
Spatial-occupancy + physics-broadphase(precedent) + Descriptor.bounds  ──>  CreativeObjectAABBIndex
```

Hard seam gate: **L2 affordance wire-strings ══> captureReconIntel.floorPlan** (capture degrades to empty until it lands).

## Stand-up order (first playable slice)

**Verify+document the HAS spine first (ranks 1-6):** Descriptor (add the totality static_assert), Mutation, Bake, Spatial-occupancy, Reasoning-graph (flag the missing fill wire), Persistence (document the redo gap).

**Then build the NEEDS for the slice (ranks 7-12):**
7. **Reasoning-graph fill wire** — the single missing arrow (room-load → setReasoningGraph). #1 unblocker, zero new engine.
8. **validateDocumentPreBake** — extraction; stops silent anchor/geometry deletion.
9. **projectGuardRecon** — UNBLOCKED; add alertBandName; head of the notebook chain.
10. **captureReconIntel** — the producer replacing the notebook fixture; graceful-degrading.
11. **floodFillReachability** — genuinely new but small; proves objective reachability + connectivity diagnostic.
12. **buildAuthoringHandles** — extraction out of the app namespace; last since the minimal slice can author via a fixture.

**Reserve the WANTS sockets (ranks 13-15):** Visibility/LOS (previewSightlineFan near-slice), CreativeObjectAABBIndex (behind the object-count instrument), transferReconIntel (behind the second actor).

## WANTS → NEEDS triggers

- **CreativeObjectAABBIndex:** object-count instrument reports ~100+ objects OR a second all-object query verb ships — the moment two readers each hand-roll an O(N) scan. Concrete: brute-force pick/select measured off-frame on a real room.
- **Visibility/LOS:** the first stealth-route authoring session — 'place the shipped guard and see its author-time sightlines' becomes active (guard is otherwise placed blind). previewSightlineFan promotes first, cover-points next, PVS last.
- **transferReconIntel:** STRUCTURAL — the second controllable co-op actor (a knight/priestess game-role on a PlayerSlot, distinct from PlayerSlotKind) comes into existence. The notebook-fill half needs no trigger (already in captureReconIntel). Secondary: L2 affordance wire-strings opening upgrades RoomFacts from empty-degraded to populated.

## Doctrine check

Every kernel is pure/generic/deterministic; leverage lives in the readers; a new object KIND touches one descriptor row and the 8 subsystems reading it change nothing. The validate→REPAIR→receipt DNA is carried by Bake (skip counters), validateDocumentPreBake (suggestedRepair hints), floodFillReachability (status receipts), Persistence (installDocument receipts), and the AABBIndex (degenerateBoundsCount). The minimums carry the full DNA: source (Descriptor) → generation (Bake, Reasoning-graph, projectGuardRecon) → validation (validateDocumentPreBake, floodFillReachability) → decision (captureReconIntel) → report (the receipts + the notebook page).

---

## Addendum — backfilled after the swarm (3 design agents hit the structured-output cap)

Three per-kernel design agents failed validation; two are already covered in the catalog above, one is added here.

- **Mutation kernel** — covered as catalog **#2** (`applyDocumentMutation`, the sole edit seam). Full contract: `applyDocumentMutation(CreativeDocument&, CreativeMutationRequest, options) -> CreativeDocumentMutationReceipt` (DocumentMutation.hpp:128). Pure over the document (no session/RNG/Date); validates per-kind legality via the Descriptor kernel (`descriptorAllowsMutation`/`canMutate`), writes the field, dirty-flags it, bumps revision only on real change. Every verb (rotate/resize/setBounds/rename/setVisible) and every tool commits through it. **Touch-count:** a new mutation kind = 1 dispatch case in `MutationApply`; a new object kind = 0. Status **HAS**, risk none.

- **Export-bridge** — folded into the **Bake kernel** (#3) per the assembly's `functionCollapse`. The two production bridges — `patrolRouteWaypointsFromDocument` (authored PatrolRoute `pathPoints` → the ordered `Vec3` span `buildReasoningGraph` accepts) and `traversalLinksFromDocument` (NavLink/JumpLink/ClimbLink → `RoomTraversalLink` edges) — are the two **PARTIAL legs** of Bake noted at #3. Status **NEEDS**, risk medium/high. **Open question first:** where do NavLink/JumpLink/ClimbLink store their two endpoints? (`hasBounds=false`, bake to nothing today — may be an authoring-storage gap, not a bake gap.)

### 15. Navmesh / traversal-reachability kernel — WANTS (new-engine)

**Charter:** baked walkable surfaces + traversal links → a navigable graph the AI *and* the parkour thief can be **proven** to inhabit (walkable nav-grid + jump/climb edges + island labeling).

- **Signature:** `buildNavGraph(const SpatialSurfaceSet& walkable, span<const RoomTraversalLink> links, NavConfig) -> NavGraph{nodes, edges(walk|jump|climb), islands} + NavReceipt`.
- **Purity:** pure over baked geometry + links; no session/RNG/Date. **Composes** the Reachability kernel (island labeling) + the Export-bridge (traversal links).
- **Absorbs:** the algorithm-map GAPS 1 & 2 (navmesh authoring/validation, parkour-connectivity) — lets the editor answer *"this platform is an unreachable island"* and *"the thief cannot reach the objective with the movement verbs"* at bake time, not via a stuck actor.
- **Readers:** the AI route planner (already consumes `ReasoningGraph` edges), the movement traversal system, and `validateDocumentPreBake` (reachability diagnostic).
- **Existing seam / precedent:** reuse `floodFillReachability` for islands; **the socket is literally reserved in the enum** — `ReasoningEdgeKind` already has `climb`/`hidden` values (ReasoningGraph.hpp) that `buildReasoningGraph` never emits (walkable-only at :201). So this kernel is "emit the reserved edge kinds," not invent a graph.
- **Invariants:** determinism (island labels stable under seed/link reorder); a link edge exists only if both endpoints resolve to nav nodes; jump/climb edges annotated with the gap they cross (gated later against movement's gravity=data + climb params).
- **Touch-count:** new traversal-link type = 1 `ReasoningEdgeKind` value (climb/hidden already reserved) + 1 emit branch. New object kind = 0.
- **Risk:** medium. **Depends on** the Export-bridge's endpoint-storage question being answered first.
- **HAS/NEEDS/WANTS:** HAS the reachability primitive + the reserved edge enum + the walkable-quad substrate; NEEDS nothing for slice 1 (flat-floor `floodFillReachability` suffices); WANTS it for the first *vertical* room where the thief/guard must jump or climb to progress.
- **WANTS → NEEDS trigger:** the first slice room that requires crossing a non-walkable gap (jump/climb) to reach the objective. Structural, not measured.

### Tier tally (corrected: 15 kernels)

| Tier | Count | Kernels |
|---|---|---|
| **HAS** | 6 | Descriptor · Mutation · Bake · Spatial-occupancy · Reasoning-graph · Persistence & Identity |
| **NEEDS** | 5 | validateDocumentPreBake · floodFillReachability · Recon-projection (`projectGuardRecon`) · Intel (`captureReconIntel`) · Authoring-handles |
| **WANTS** | 4 | CreativeObjectAABBIndex · Visibility/LOS · Navmesh/traversal · transferReconIntel |
