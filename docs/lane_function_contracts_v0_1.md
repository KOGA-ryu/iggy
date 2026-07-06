# Lane Function Contracts (v0.1) — creative · export · AI/recon · validation · scale

Grounded 2026-07-06 by a 5-lane + assembly workflow that greps every claim against the real code. Function contracts, not code. Status: EXISTS / PARTIAL / FIX / NEW / DEFER. **Honesty rule: wrappers and extractions are marked as such — do not mistake them for new engine work.**

---

# Assembled Spec — Creative -> Export -> AI/Notebook -> Validation -> Scale

## Headline: how little is actually new

47 contracted functions. **Only 15 are NEW, and ~6 of those are wrappers/extractions**, not engine work. The verb engine, the bake spine, save/load/undo, and — verified against the code — the **anchor half of the Creative->AI seam are all already DONE**. The genuine new-work core for the first playable slice is about a dozen functions: interactive gizmo/handle affordances (tool-layer), two production bridges into the reasoning graph, the L4 validate->REPAIR->receipt umbrella + grid flood primitive, and the L3 recon data spine. An entire lane (L5 scale) and the duo half of L3 should build nothing yet.

### Honesty flags confirmed by grep (not taken on faith)
| Claim under test | Verdict | Evidence |
|---|---|---|
| L2 "anchor emits generic occupancy strings reader drops" (brief's seam-break premise) | **STALE — anchor seam is CLOSED** | `anchorKindForDescriptor` returns `toString(descriptor.runtimeAnchorSemantic)` (RoomBake.cpp:253-256); `nodeKindForAnchor` already cases on exit/treasure/key/spawn/npc/monster/chokepoint/high_ground/hiding_spot/cover/patrol_post (ReasoningGraph.cpp:26+) |
| `buildReasoningGraph` has no production call site | **CONFIRMED** | called only from its own .cpp/.hpp + 4 test files; every src consumer hand-feeds nothing — tests hand-feed literal Vec3 vectors |
| Notebook page has no producer | **CONFIRMED** | `ProductNotebookReconPage` is only ever filled by the test fixture `sampleReconPage()` (product_notebook_ui_draw_list_tests.cpp:39) |
| Redo does not exist | **CONFIRMED** | zero `redo` matches in CreativeAppState.hpp / StandaloneUndo.hpp |
| Duo consumer does not exist | **CONFIRMED** | zero src files mention priestess/knight/spectre; PlayerRoster is slot+EntityId only |
| Duplicate verb does not exist | **CONFIRMED** | only `DuplicateObjectId` restore-error enum matches (DocumentSection.cpp) |
| id->object is already O(1) | **CONFIRMED** | `objectIndex_` unordered_map, Document.hpp:233 |
| Stale-parent/cycle detection already done | **CONFIRMED** | `validateCreativeObjectParentGraph` (Document.cpp:298), wired into every mutation |
| Degenerate-bounds detection already done (silent) | **CONFIRMED** | `validBakeBounds` (RoomBake.cpp:71-84), used as silent SkipNoBounds |

## Tally by status
| Status | Count | Meaning |
|---|---|---|
| EXISTS | 12 | built + working; surface only, do not rebuild |
| PARTIAL | 8 | started / half-built / in Codex E-queue |
| FIX | 4 | exists but wrong (mostly silent-skip -> named diagnostic, or walkable-only edges) |
| NEW | 15 | genuinely absent (~6 are wrapperOrExtraction=true) |
| DEFER | 8 | real only at measured scale / when a second actor exists |
| **slice-critical** | **19** | on the first-playable-slice critical path |

## Cross-lane DAG (the load-bearing edges)
```
L1 applyDocumentMutation ──┬─> L1 rotate/resize/setBounds ──> L1 Tool::Rotate/Scale
                           ├─> L1 dispatchToolInput ─────────> L1 Line/box handle drag
                           └─> (every verb)
L1 describeObject/allDescriptors ──> L2 buildRoomAsset ──┬─> L2 anchorKind+nodeKind (CLOSED seam)
                                                         ├─> L2 patrolRouteWaypointsFromDocument
                                                         └─> L2 traversalLinksFromDocument ──> L2 buildReasoningGraph(link edges)
L2 append anchor-semantic enum values  ══(HARD SEAM GATE)══>  L3 captureReconIntel.floorPlan
L3 alertBandName ──> L3 projectGuardRecon ──> L3 captureReconIntel ──┬─> L3 buildNotebookReconPage ──> (shipped draw-list builder)
                                                                     ├─> L3 hashReconIntel/serializeReconIntel
                                                                     └─> L3 transferReconIntel ══(needs 2nd actor — DOES NOT EXIST)══> L3 duoConsumeReconIntel
L5 projectObjectsToGrid ──> L4 floodFillReachability ──> L4 checkFloating/checkOverlap
L4 CreativeValidationDiagnostic ──> L4 validateDocumentPreBake ──(delegates)──> L4 checkStaleParentIds (EXISTS)
L5 object-count instrument ══(measured trigger)══> L5 CreativeObjectAABBIndex (all queries)
```

## The ordered build sequence (one sequence across all lanes)
Prerequisites + slice-critical first; convenience mid; deferred last.

| # | Function | Lane | Status | Class | Note |
|---|---|---|---|---|---|
| 1 | applyDocumentMutation | L1 | EXISTS | prerequisite | the one edit seam |
| 2 | describeObject/allObjectDescriptors | L1 | EXISTS | prerequisite | the READER table |
| 3 | dispatchToolInput/selectionState | L1 | EXISTS | prerequisite | tool spine |
| 4 | findObject (objectIndex_) | L5 | EXISTS | prerequisite | id->obj already O(1) — do NOT build id index |
| 5 | checkStaleParentIds (validateCreativeObjectParentGraph) | L4 | EXISTS | prerequisite | already wired every mutation |
| 6 | save/load/installDocument | L1 | EXISTS | prerequisite | install = undo/redo primitive too |
| 7 | buildRoomAssetFromCreativeDocument | L2 | EXISTS | prerequisite | bake spine |
| 8 | roleForObject/appendSpatialSurfaces | L2 | EXISTS | prerequisite | physical->reasoning bridge |
| 9 | anchorKindForDescriptor + nodeKindForAnchor | L2 | EXISTS | slice-critical | **anchor seam already CLOSED** |
| 10 | pushRedoAndWithUndo | L1 | PARTIAL | slice-critical | symmetric 2nd vector; collapses 5+ call sites |
| 11 | CreativeValidationDiagnostic (+repair) | L4 | NEW | slice-critical | the one genuinely-new type; suggestedRepair field |
| 12 | rotate/resize/setBoundsDocumentObject | L1 | EXISTS | slice-critical | kernel the new tools commit through |
| 13 | validateDocumentPreBake (umbrella) | L4 | NEW | slice-critical | silent skips -> named repairable receipt |
| 14 | runtimeAnchorSemanticForKind + append enum values | L2 | PARTIAL | slice-critical | **closes the live seam gate; overlaps E67** |
| 15 | checkDegenerateBounds | L4 | FIX | validation | extract validBakeBounds; share predicate |
| 16 | checkAnchorsDropped | L4 | FIX | slice-critical | extract SkipUnsupportedAnchor; guards spawns |
| 17 | floodFillReachability | L4 | PARTIAL | slice-critical | grid flood over projectObjectsToGrid |
| 18 | Tool::Rotate/Scale + drag lifecycle | L1 | PARTIAL | slice-critical | tool-layer, mirror Move |
| 19 | patrolRouteWaypointsFromDocument | L2 | NEW | slice-critical | single most-real new bridge |
| 20 | Line endpoint drag + moveLineEndpointWithUndo | L1 | NEW | slice-critical | copy path-handle pattern |
| 21 | box bounds-handle drag + resizeBoxBoundsWithUndo | L1 | NEW | slice-critical | reuse projectBoxToScreen |
| 22 | checkFloatingObjects | L4 | NEW | validation | no support-below detection exists |
| 23 | checkOverlappingStructuralVolumes | L4 | PARTIAL | validation | NEW reader over EXISTING occupancy grid |
| 24 | traversalLinksFromDocument + RoomTraversalLink | L2 | NEW | slice-critical | **first verify link endpoint storage** |
| 25 | buildReasoningGraph (link edges) | L2 | FIX | slice-critical | walkable-only today; keep determinism |
| 26 | alertBandName | L3 | NEW | prerequisite | 6-arm switch wrapper |
| 27 | projectGuardRecon/GuardReconObservation | L3 | NEW | slice-critical | pure projector; source of all recon |
| 28 | ReconIntel + captureReconIntel | L3 | NEW | slice-critical | **degrades gracefully at seam gate** |
| 29 | buildNotebookReconPage | L3 | NEW | slice-critical | the missing PRODUCER |
| 30 | hashReconIntel | L3 | NEW | validation | reuse StableHasher |
| 31 | serializeReconIntel | L3 | NEW | validation | reuse SaveAiActorRecord codec |
| 32 | runtimeAnchorSemanticForKind accessor | L2 | PARTIAL | convenience | overlaps E67 |
| 33 | duplicateDocumentObject | L1 | NEW | convenience | thin wrapper over createDocumentObject |
| 34 | path waypoint insert/remove/reorder | L1 | PARTIAL | convenience | extends PathPointsMutation |
| 35 | checkDescriptorCoverage (runtime) | L4 | PARTIAL | validation | overlaps E59; keep runtime scope to object |
| 36 | entityFromAnchor | L2 | EXISTS | validation | 3rd reader; flags entity-count hash shift |
| 37 | object-count/query-time instrument | L5 | PARTIAL | validation | **build this NOW so DEFERs are data** |
| 38 | checkPatrolRouteConnectivity | L4 | NEW | deferred | blocked on authored-waypoint doc rep |
| 39 | projectObjectsToGrid (scan pressure) | L5 | EXISTS | deferred | grid substrate + un-guarded scan |
| 40 | pick candidate gather (scan) | L5 | EXISTS | deferred | O(n); also the oracle |
| 41 | CreativeObjectAABBIndex + rebuildFrom | L5 | DEFER | deferred | speculative/uncarded (map:29) |
| 42 | AABBIndex insert/remove/update | L5 | DEFER | deferred | rides dirtyFlags |
| 43 | AABBIndex::queryOverlap | L5 | NEW | deferred | near-term pull; do brute-force first |
| 44 | AABBIndex::queryRay | L5 | DEFER | deferred | exact test exists; only broadphase new |
| 45 | AABBIndex::queryFrustum | L5 | DEFER | deferred | deepest defer |
| 46 | transferReconIntel | L3 | DEFER | deferred | no consumer exists |
| 47 | duoConsumeReconIntel | L3 | DEFER | deferred | knight/priestess side has no home |

## The prerequisite gate (do not start L3's floor plan / L2's links prematurely)
- **SOFT (L3 can start now):** projectGuardRecon needs session.reasoningGraph, which is populated at activation (Session.cpp:1311). The guard-observation half reads durable AiActorState the game already saves — start immediately.
- **HARD SEAM GATE:** captureReconIntel's floorPlan/garrison/hazard strings need affordance wire-strings that NEITHER bake path emits yet (the one live seam break). The concrete L2 prerequisite is rank 14 (append CreativeRuntimeAnchorSemantic values so authored objects EMIT chokepoint/cover/patrol_post). Until then captureReconIntel ships its guard half and DEGRADES to empty floorPlan; buildNotebookReconPage must render a valid page with a blank sketch. **Close rank 14 EARLY — it is the single highest-leverage unblock for the slice.**
- **STORAGE QUESTION before ranks 24-25:** NavLink/JumpLink/ClimbLink have hasBounds=false and bake to nothing — verify WHERE they store their two endpoints before baking a connector; endpoints may be absent entirely (an authoring gap, not a bake gap).
- **DEAD-END (do not start):** transferReconIntel + duoConsumeReconIntel depend on a controllable second actor that does not exist. DEFER.

---

## Full function contracts (all fields, grounded per lane)


### Lane 1 — Creative authoring

> **Existing surface:** Roughly 80% real, 20% genuine new work. The mutation KERNEL is fully built and generic: Rename/SetVisible/SetLocked/Move/Rotate/Scale/SetTransform/Resize/Stretch/SetBounds all validate against per-kind rules (canMutate/allowedMutations), write the object field, dirty-flag it, and return receipts — MutationApply.cpp:136-205 dispatches them; makeRotate/Scale/Resize/Bounds payload factories all exist (Mutation.cpp:817-838). The document-level bridge (applyDocumentMutation + the convenience wrappers moveDocumentObject/rotateDocumentObject/resizeDocumentObject/setDocumentObjectBounds/renameDocumentObject/setDocumentObjectVisible|Locked) is complete (DocumentMutation.hpp:128-205). Create/remove is complete (Facade::createDocumentObject/removeDocumentObject → Document::createObject/removeDocumentObject; CreativeDocumentCreateRequest carries transform/bounds/path/tag overrides). Selection + Move-drag TOOL is complete end-to-end: Tool enum {Select,Move,Measure,Navigate}, dispatchToolInput with the BeginMove/PreviewMove/CommitMove/CancelMove lifecycle, snap, and a full CreativeFacadeMoveDragReceipt. Save/load is complete via saveCreativeWorld/openCreativeWorld/installDocument. Undo is a working snapshot ring (CreativeDocumentUndoStack, push/apply/discard) but SNAPSHOT-ONLY with NO redo. Descriptor reads are exhaustive (describeObject/shapeKindForObject/categoryOf/allObjectDescriptors + dirtyFlagsForMutation). Standalone helpers are rich: placeBrushObjectWithUndo, dispatchMoveReleaseWithUndo, movePathObjectWithUndo, movePathPointWithUndo, buildPathPointHandleHits/pickPathPointHandle, projectBoxToScreen/pickNearestVisualBoundsObject, a Move-only 3-axis gizmo. Bake hook buildRoomAssetFromCreativeDocument emits meshes+anchors+spatial-surfaces. The GREENFIELD is narrow: (1) redo; (2) Duplicate (absent everywhere — only DocumentSection's DuplicateObjectId error enum matches the grep); (3) interactive Rotate/Scale/Place TOOLS + their gizmo drag (kernel exists, tool+gizmo don't); (4) Line endpoint drag + box bounds-handle drag (path-waypoint MOVE exists as the proven pattern; endpoint/box-handle geometry+drag don't); (5) path waypoint insert/remove/reorder (move exists); (6) the bake affordance wire-string emission (the master-plan seam break) and rotation/scale being unread by bake.


> **Lane summary:** Do NOT rebuild the verb engine — it is the most finished part of the codebase. Every one of the 8 "generic verbs" already exists as a MUTATION: Select+Move are wired as TOOLS with a full drag lifecycle; Rotate/Scale/Resize/SetBounds exist as validated kernel mutations reachable through applyDocumentMutation but have NO interactive tool or gizmo; Place exists as placeBrushObjectWithUndo (a two-step palette+ground drop, not a live tool); Delete exists as removeDocumentObject/deleteSelectedObject; Measure exists as a tool; Duplicate does NOT exist in any form. The ACTUAL new work, smallest-first: (A) redoStack + a withUndo wrapper that captures pre-image and pushes a redo entry — the undo ring is snapshot-based so redo is a symmetric second std::vector<CreativeDocument> plus reordering the existing apply helper; LOW risk, pure extension of CreativeAppState. (B) duplicateDocumentObject — a THIN wrapper: read the object, build a CreativeDocumentCreateRequest from its kind+transform+bounds+pathPoints+tags (buildBrushCreateRequest is the template), offset it, call createDocumentObject; NEW but wrapperOrExtraction=true, LOW risk. (C) Rotate/Scale interactive tools: the kernel + payloads + document wrappers already exist and are verified to write transform.rotation/transform.scale; the new work is a Tool::Rotate/Tool::Scale enum value, the tool-core drag lifecycle intents (mirror BeginMove/PreviewMove/CommitMove), and gizmo hit-geometry — MEDIUM risk, mostly in the tool core + standalone gizmo, NOT the engine. (D) Line endpoint drag + box bounds-handle drag: movePathPointWithUndo + buildPathPointHandleHits/pickPathPointHandle are the EXACT proven pattern to copy — build handle AABBs at the two Line endpoints / the box corners, pick them, and commit via setDocumentObjectBounds/SetBounds; MEDIUM risk, standalone-app affordance work. (E) path waypoint insert/remove/reorder: extends the existing PathPointsMutation/SetPatrolRoute path — LOW-MEDIUM. (F) the cross-lane bake seam: rotation/scale are WRITTEN by the kernel but UNREAD by RoomBake and DocumentWireframe (bake reads bounds+anchorKind only), and RoomBake emits NO affordance wire-strings — this is Lane 2's contract but Lane 1 must surface the authored rotation/scale + affordance-tag data for it to consume. Biggest risk sits in (C)/(D): getting screen-space drag math right, not engine correctness. Note one latent smell: path MOVE reuses CreativeMutationKind::SetPatrolRoute as the carrier for geometry edits (main.cpp:508) — fine today but worth a dedicated SetPathPoints kind if path editing grows.

#### `applyDocumentMutation` — **EXISTS** · _prerequisite_ · risk: none
- **Owner:** `src/app/iggy3d/creative/document/DocumentMutation.*`
- **Exists as:** applyDocumentMutation (DocumentMutation.hpp:128, .hpp:133 overload)
- **Caller:** movePathObjectWithUndo/movePathPointWithUndo (main.cpp:504,588), any tool committing a non-move mutation, Facade internals
- **Callees / deps:** MutationApply::applyMutation, CreativeDocument::findObject/incrementRevision, describeObject/dirtyFlagsForMutation
- **Inputs:** CreativeDocument&, CreativeMutationRequest (or objectId+kind+payload), CreativeDocumentMutationOptions
- **Output / receipt:** CreativeDocumentMutationReceipt (status/revisionBefore/After/dirtyFlags/changed/allowed/objectReceipt)
- **Invariants:** only increments revision if object actually changed; per-kind rule validation via canMutate; lock state respected
- **Failure modes:** MissingObject, InvalidRequest, Rejected (rule/lock), ApplyFailed, NoChange
- **Test shape:** apply Rotate to a rotatable kind → Applied+changed+revision++; apply to locked → Rejected; apply no-op → NoChange
- **Why:** the single generic document-edit seam every verb routes through
- **Touch-count reduction:** one bridge for all 60+ mutation kinds — adding a kind never adds a document-level function

#### `rotateDocumentObject / resizeDocumentObject / setDocumentObjectBounds` — **EXISTS** · _slice-critical_ · risk: none  · **wrapper/extraction**
- **Owner:** `src/app/iggy3d/creative/document/DocumentMutation.*`
- **Exists as:** rotateDocumentObject (DocumentMutation.hpp:159), resizeDocumentObject (:165), setDocumentObjectBounds (:171); kernel at MutationApply.cpp:136/148/160
- **Caller:** currently only tests; the NEW Rotate/Scale tools + box bounds-handle drag will call these on commit
- **Callees / deps:** applyDocumentMutation → applyRotate/Resize/SetBoundsMutation which write transform.rotation/bounds
- **Inputs:** CreativeDocument&, CreativeObjectId, CreativeVec3 rotation/size or CreativeBounds
- **Output / receipt:** CreativeDocumentMutationReceipt
- **Invariants:** writes transform.rotation/transform.scale/bounds; sameVec3 short-circuits to NoChange; validated by descriptor rules
- **Failure modes:** kind does not allow the mutation → Rejected; missing object → MissingObject
- **Test shape:** rotateDocumentObject on a Wall → transform.rotation updated, Transform dirty flag set; verify NoChange on identical value
- **Why:** the verbs the new Rotate/Scale gizmo and box-handle drag will commit through — proves the engine already supports them
- **Touch-count reduction:** n/a

#### `pushRedoAndWithUndo (redo ring + withUndo wrapper)` — **PARTIAL** · _slice-critical_ · risk: low  · **wrapper/extraction**
- **Owner:** `src/app/iggy3d/creative/CreativeAppState.hpp + apps/iggy3d_creative/StandaloneUndo.hpp`
- **Exists as:** undo half exists: CreativeDocumentUndoStack (CreativeAppState.hpp:59), pushCreativeUndoSnapshot (:97), applyLastCreativeUndoSnapshot (:126); NO redo stack, NO withUndo wrapper — each call site hand-rolls push+discard (main.cpp:437-444)
- **Caller:** placeBrushObjectWithUndo, dispatchMoveReleaseWithUndo, deleteSelectedObject, movePath*WithUndo, and the redo keybind
- **Callees / deps:** installDocument, pushCreativeUndoSnapshot, a symmetric redo std::vector<CreativeDocument>
- **Inputs:** CreativeAppState&, the mutating lambda, source string
- **Output / receipt:** receipt of the wrapped op + updated undo/redo depths
- **Invariants:** a committed edit pushes undo + clears redo; undo pops to redo; redo pops to undo; capped at maxDepth
- **Failure modes:** empty stack → no-op receipt; install rejected → snapshot discarded (already handled for undo, must mirror for redo)
- **Test shape:** place→undo→redo restores object; new edit after undo clears redo; depth caps at 32
- **Why:** redo is the single most-cited missing piece; the undo ring is snapshot-based so redo is a symmetric second vector, not new engine work
- **Touch-count reduction:** a withUndo wrapper collapses the ~15-line hand-rolled push/discard block duplicated at 5+ call sites into one

#### `duplicateDocumentObject` — **NEW** · _convenience_ · risk: low  · **wrapper/extraction**
- **Owner:** `src/app/iggy3d/creative/document/DocumentMutation.* (new) or apps standalone helper`
- **Exists as:** —
- **Caller:** a Duplicate keybind/menu action in the standalone app and product app
- **Callees / deps:** findObject, buildBrushCreateRequest-style request assembly, Facade::createDocumentObject
- **Inputs:** CreativeDocument&/Facade&, CreativeObjectId source, CreativeVec3 offset
- **Output / receipt:** CreativeDocumentCreateReceipt for the new copy
- **Invariants:** copy carries kind+transform(+offset)+bounds+pathPoints+tags+visible+locked; gets a fresh id; original untouched
- **Failure modes:** missing source → receipt not-created; create rejected (e.g. kind not palette-visible is irrelevant here — create is by kind) → propagated status
- **Test shape:** duplicate a Wall → objectCount+1, copy kind==source kind, position offset applied, original unchanged; duplicate a Path → pathPoints copied+offset
- **Why:** the only one of the 8 generic verbs with zero existing surface; genuinely absent
- **Touch-count reduction:** reuses CreativeDocumentCreateRequest + createDocumentObject — no new engine path

#### `Tool::Rotate / Tool::Scale + rotate/scale drag lifecycle` — **PARTIAL** · _slice-critical_ · risk: medium
- **Owner:** `src/app/iggy3d/creative/Core.hpp (Tool enum) + tools/Tools.* + apps gizmo (main.cpp)`
- **Exists as:** Move lifecycle is the template: Tool enum (Core.hpp:11) has Select/Move/Measure/Navigate; BeginMove/PreviewMove/CommitMove/CancelMove intents (Tools.hpp:31-34); Facade::applyMoveDragIntent + CreativeFacadeMoveDragReceipt; Move-only 3-axis gizmo (main.cpp:325-459). Rotate/Scale kernel mutations already exist and write.
- **Caller:** dispatchToolInput from the input frame; gizmo hit-test in the standalone render loop
- **Callees / deps:** applyDocumentMutation(Rotate/Scale), snapScalar/snapPoint, heldAxis gizmo helpers
- **Inputs:** CreativeToolInputPacket (press/move/release) on the active Rotate/Scale tool + gizmo axis grab
- **Output / receipt:** a rotate/scale-drag receipt mirroring CreativeFacadeMoveDragReceipt
- **Invariants:** press-preview-commit-cancel; commit routes ONE snapped Rotate/Scale mutation through the kernel; cancel mutates nothing
- **Failure modes:** no target → NoTarget; locked object → RejectedLocked; no rotation delta → NoChange
- **Test shape:** press gizmo ring→preview→release commits Rotate, transform.rotation changed, revision++; escape mid-drag leaves object unchanged
- **Why:** the kernel supports rotate/scale but there is no interactive way to invoke it; this is the real tool-layer gap, NOT engine work
- **Touch-count reduction:** copying the Move lifecycle means the receipt/intent scaffolding is reused, not reinvented

#### `buildLineEndpointHandleHits / pickLineEndpointHandle + moveLineEndpointWithUndo` — **NEW** · _slice-critical_ · risk: medium  · **wrapper/extraction**
- **Owner:** `apps/iggy3d_creative/StandalonePicking.* + main.cpp`
- **Exists as:** exact pattern exists for Path: buildPathPointHandleHits/pickPathPointHandle (StandalonePicking.hpp:88-97) + movePathPointWithUndo (main.cpp:535). Line shape is drawn (main.cpp:2484-2519) but has NO endpoint handles or drag.
- **Caller:** the standalone render loop when a Line-shaped object is selected
- **Callees / deps:** projectPointToScreen, applyDocumentMutation(SetBounds) to move the Line's two endpoints (Line stored as bounds min/max)
- **Inputs:** selected CreativeObject (Line shape), clip matrix, pixel cursor, drag delta
- **Output / receipt:** handle hit + a SetBounds mutation receipt on commit
- **Invariants:** only the grabbed endpoint moves; the other stays; commit through the kernel + undo
- **Failure modes:** object not Line shape → skipped; no handle under cursor → no drag
- **Test shape:** select a Line, drag endpoint handle → that endpoint's bounds component moves, other endpoint fixed, undo restores
- **Why:** Line endpoint editing is a named affordance gap; the path-handle code is a direct copy target
- **Touch-count reduction:** mirrors the path-handle helpers — pick/drag/commit skeleton is reused

#### `buildBoxBoundsHandleHits / pickBoxBoundsHandle + resizeBoxBoundsWithUndo` — **NEW** · _slice-critical_ · risk: medium  · **wrapper/extraction**
- **Owner:** `apps/iggy3d_creative/StandalonePicking.* + main.cpp`
- **Exists as:** —
- **Caller:** standalone render loop when a BoxVolume/Surface-shaped object is selected
- **Callees / deps:** projectBoxToScreen (StandalonePicking.hpp:60), appendWireframeBoxEdges for handle viz, applyDocumentMutation(SetBounds/Resize)
- **Inputs:** selected object bounds, clip matrix, pixel cursor, drag delta on a grabbed corner/face
- **Output / receipt:** SetBounds mutation receipt on commit
- **Invariants:** dragging a corner/face edits only that bound; min<=max enforced (validate→repair); Y policy per plan; commit + undo
- **Failure modes:** object has no bounds → skipped; degenerate bounds (min>max) → repaired or rejected
- **Test shape:** select a Wall, drag a corner handle → bounds grow, revision++, undo restores; drag past opposite corner → repaired not inverted
- **Why:** box-out bounds-handle drag is a named affordance gap; the box projection + wire-edge helpers already exist to build on
- **Touch-count reduction:** reuses projectBoxToScreen + appendWireframeBoxEdges

#### `path waypoint insert / remove / reorder` — **PARTIAL** · _convenience_ · risk: low  · **wrapper/extraction**
- **Owner:** `apps/iggy3d_creative/main.cpp + Mutation PathPointsMutation path`
- **Exists as:** path point MOVE fully exists: movePathPoint (main.cpp:193), movePathPointWithUndo (:535), buildPathPointHandleHits/pickPathPointHandle. Insert/remove/reorder do NOT exist. Carrier mutation is PathPointsMutation via SetPatrolRoute (main.cpp:508).
- **Caller:** standalone app path-edit keybinds on a selected Path object
- **Callees / deps:** applyDocumentMutation with makePathPointsPayload of the edited vector
- **Inputs:** CreativeObject Path, point index, (for insert) new position
- **Output / receipt:** CreativeDocumentMutationReceipt
- **Invariants:** insert/remove keep >=2 points; reorder preserves count; each commit is one PathPointsMutation + undo
- **Failure modes:** index out of range → skipped; removing below min points → rejected
- **Test shape:** insert after index i → count+1 at i+1; remove index i → count-1; undo restores full sequence
- **Why:** completes Path editing beyond move; the payload+mutation plumbing already carries arbitrary point vectors
- **Touch-count reduction:** reuses makePathPointsPayload + the existing with-undo path helper shape

#### `saveStandaloneScene / loadStandaloneScene / installDocument` — **EXISTS** · _prerequisite_ · risk: none
- **Owner:** `apps/iggy3d_creative/main.cpp + creative/world (saveCreativeWorld/openCreativeWorld) + Facade`
- **Exists as:** saveStandaloneScene (main.cpp:686), loadStandaloneScene (:709), clearToBlankScene (:736), Facade::installDocument (Facade.hpp:212), saveCreativeWorld/openCreativeWorld (world service)
- **Caller:** save/load keybinds + the capture script; undo/redo also route through installDocument
- **Callees / deps:** CreativeWorldSaveRequest/Result, CreativeWorldOpenRequest/Result, Facade::installDocument (resets selection/ghost/move-drag)
- **Inputs:** Facade/AppState, saveRoot path, saveId
- **Output / receipt:** CreativeWorldSaveResult / bool installed + CreativeFacadeDocumentInstallReceipt
- **Invariants:** save drains dirty flags on a COPY (on-disk bytes unchanged); load re-mints ids; install clears transient references so nothing dangles
- **Failure modes:** open not accepted → not installed; install rejected → old document kept
- **Test shape:** place objects→save→clear→load→snapshotsMatch (kinds+positions+bounds, ids may re-mint)
- **Why:** persistence is fully working; install is also the undo/redo primitive
- **Touch-count reduction:** installDocument centralizes all transient-state reset for load AND undo AND redo

#### `buildRoomAssetFromCreativeDocument (bake/export hook — Lane 2 seam)` — **FIX** · _slice-critical_ · risk: high
- **Owner:** `src/app/iggy3d/creative/adapters/RoomBake.*`
- **Exists as:** buildRoomAssetFromCreativeDocument (RoomBake.hpp:74); roleForObject/anchorKind/meshIdForRole (RoomBake.cpp:164-216); emits staticMesh+anchor+spatialSurface sources
- **Caller:** the room-bake step of the first-slice fire order (thief room → guard)
- **Callees / deps:** describeObject, roleForObject, RoomAsset assembly
- **Inputs:** CreativeRoomBakeRequest (document, roomId, includeHidden)
- **Output / receipt:** CreativeRoomBakeResult (RoomAsset + receipt + mesh/anchor/surface source lists)
- **Invariants:** skips hidden/editor-only/no-bounds; classifies Surface→Floor/Wall/Prop by orientation
- **Failure modes:** MissingDocument, InvalidDocument, NoRenderableObjects; AND two SILENT GAPS: (1) transform.rotation/scale are WRITTEN by the kernel but UNREAD here — a rotated wall bakes axis-aligned; (2) NO affordance wire-strings emitted (the master-plan live seam break)
- **Test shape:** bake a room with a rotated wall → currently rotation ignored (FIX target); bake with an affordance-tagged object → assert a wire-string appears (currently absent)
- **Why:** this is the cross-lane hand-off; it works for axis-aligned bounds but drops rotation and emits no affordances — Lane 1 must surface that authored data for Lane 2 to consume
- **Touch-count reduction:** n/a

#### `describeObject / shapeKindForObject / dirtyFlagsForMutation / allObjectDescriptors` — **EXISTS** · _prerequisite_ · risk: none
- **Owner:** `src/app/iggy3d/creative/document/ObjectDescriptor.*`
- **Exists as:** describeObject (ObjectDescriptor.hpp:220), shapeKindForObject (:219), categoryOf (:217), dirtyFlagsForMutation (:224), allObjectDescriptors (:221)
- **Caller:** every tool/affordance decision: buildBrushCreateRequest, movePath*WithUndo (shape gate), the new endpoint/box-handle tools, RoomBake role classification
- **Callees / deps:** the static descriptor table (107 kinds)
- **Inputs:** CreativeObjectKind
- **Output / receipt:** const CreativeObjectDescriptor& / shape/category/dirty flags
- **Invariants:** one table row per kind; shapeKind decides editing affordance, category/profile decide meaning — 'shape decides editing, kind decides meaning'
- **Failure modes:** unknown kind → Unknown descriptor (safe default)
- **Test shape:** describeObject(Wall).shapeKind==BoxVolume/Surface; every kind has a non-Unknown shapeKind (kind-coverage sentinel, Codex E59)
- **Why:** the READER table the whole doctrine leans on; new affordance tools branch on shapeKind here rather than per-kind code
- **Touch-count reduction:** adding an object kind is a table row, never a new tool/verb function

#### `dispatchToolInput / setActiveTool / selectionState (Select + Move tool spine)` — **EXISTS** · _prerequisite_ · risk: none
- **Owner:** `src/app/iggy3d/creative/Facade.* + tools/*`
- **Exists as:** Facade::dispatchToolInput (Facade.hpp:195), setActiveTool (:193), selectionState (:187), CreativeFacadeToolDispatchReceipt (:72), Move-drag receipt (:49)
- **Caller:** the standalone input frame; the new Rotate/Scale tools extend this same dispatch
- **Callees / deps:** tool core (Select/Move lifecycle), applyMoveDragIntent, updateSelectionCandidate, snap
- **Inputs:** CreativeToolInputPacket (pointer press/move/release, tool select)
- **Output / receipt:** CreativeFacadeToolDispatchReceipt (selectionChanged/moveDragChanged/moveDrag receipt)
- **Invariants:** one dispatch per input; Move commit routes exactly one snapped mutation; selection is receipt-visible
- **Failure modes:** unknown input kind → not dispatched; no active-tool match → no intent
- **Test shape:** press on object → selectionChanged; Move press-move-release → moveDrag.stage progresses Begin→Preview→Commit, one mutation applied
- **Why:** Select + Move are the two fully-wired tools; the new Rotate/Scale tools are additions to this exact dispatch, proving they are tool-layer not engine work
- **Touch-count reduction:** one dispatch seam carries all tools; a new tool is an enum value + intent handling, not a new pipeline


### Lane 2 — Runtime affordance / export (the Creative→AI seam: authored objects → RoomAsset anchors/surfaces → reasoning graph nodes/edges → session entity seeds)

> **Existing surface:** Mostly REAL, and better-wired than the brief assumes. The bake pipeline (buildRoomAssetFromCreativeDocument, RoomBake.cpp) is a complete 606-line classify→bake→receipt engine with a typed receipt, spatial-surface derivation, and stable ids. The RoomAsset payload contracts (RoomAnchorAsset, RoomSpatialSurface w/ traversalTags+collisionMask+blocks flags, RoomStaticMeshAsset w/ wall-segment fields) all EXIST (content/assets/RoomAsset.hpp). The reader (buildReasoningGraph + nodeKindForAnchor, ReasoningGraph.cpp) EXISTS and already cases on the exact strings the bake emits. The critical brief claim — "anchorKindForDescriptor emits generic occupancy strings the reader drops" — is FALSE against the code: anchorKindForDescriptor returns toString(descriptor.runtimeAnchorSemantic) (RoomBake.cpp:253-256), i.e. spawn/exit/npc/monster/pickup/light/audio/camera — real vocab strings. The reader consumes spawn/npc/monster/exit/pickup and correctly drops light/audio/camera (no node kind). So the ANCHOR half of the seam is CLOSED and test-pinned (reasoning_graph_tests, stealth_garden_tests). The GENUINE greenfield is three narrow things: (1) PatrolRoute pathPoints → std::span<Vec3> waypoint bridge for buildReasoningGraph (descriptor + mutation + storage all exist; the extractor does not — buildReasoningGraph is never called from any src/ path, only tests hand-feed literal vectors); (2) NavLink/JumpLink/ClimbLink → reasoning climb/hidden edges (kinds exist as Line/NoProjection objects that currently bake to NOTHING; graph emits only walkable edges); (3) the 5 vocab kinds the reader cases on but no descriptor emits (chokepoint/high_ground/hiding_spot/cover/patrol_post) + treasure/key — these need CreativeRuntimeAnchorSemantic enum values (append-only) before any authored object can produce them. This last one overlaps Codex E67 and is docced as "no authoring emitter yet" in affordance_vocabulary_v0_1.md.


> **Lane summary:** The bake→anchor→reasoning-node path is DONE and correctly wired — do not rebuild it; the only anchor-side work is enum-value additions (append-only) to close the vocab kinds the reader already handles but no descriptor emits (chokepoint/high_ground/hiding_spot/cover/patrol_post/treasure/key), which is co-owned with Codex E67. The ACTUAL new engine work in this lane is TWO bridges into the reasoning graph, both genuinely absent: (A) patrolRouteWaypointsFromDocument — walk a CreativeDocument's PatrolRoute (Path-shape) objects, project their pathPoints to world Vec3, and return the ordered span buildReasoningGraph already accepts. All prerequisites exist (PatrolRoute descriptor, pathPoints storage, SetPatrolRoute mutation); nothing extracts them. (B) traversalLinksFromDocument → reasoning climb/hidden/jump edges — NavLink/JumpLink/ClimbLink are authored Line objects with two endpoints that currently bake to NOTHING (hasBounds=false → SkipNoBounds/SkipUnsupportedShape); buildReasoningGraph emits only walkable edges. Wiring these means baking link endpoints as node-pair connectors and letting the graph builder emit non-walkable edge kinds. Supporting NEW-but-trivial pieces: a runtimeAnchorSemanticForKind free accessor (field exists, no by-kind function; overlaps E67), and a small extension to buildRoomAssetFromCreativeDocument to bake link objects as a RoomAsset payload (a RoomTraversalLink vector — the one new payload struct). Risk is low-to-medium and concentrated in (B): determinism (stable ordering must match the existing sort law), and the edge-vs-node-id contract. Everything else is EXISTS/PARTIAL/FIX, not NEW.

#### `buildRoomAssetFromCreativeDocument` — **EXISTS** · _prerequisite_ · risk: none
- **Owner:** `src/app/iggy3d/creative/adapters/RoomBake.*`
- **Exists as:** buildRoomAssetFromCreativeDocument at RoomBake.cpp:529 (decl RoomBake.hpp:74)
- **Caller:** adapters/RoomBake callers (save/bake flow); tests creative_document_room_bake_tests.cpp; downstream buildReasoningGraph consumes result.room
- **Callees / deps:** describeObject, classifyRoomBakeObject, anchorForObject, staticMeshForObject, appendSpatialSurfaces, setStatus; cross-lane: emits RoomAsset consumed by runtime/ai buildReasoningGraph + world/PackageSessionSeed
- **Inputs:** const CreativeRoomBakeRequest& (document ptr, roomId, sourceName, sourceSubset, includeHidden)
- **Output / receipt:** CreativeRoomBakeResult (RoomAsset room + typed CreativeRoomBakeReceipt + per-object source-provenance vectors)
- **Invariants:** null/invalid document -> receipt status set, empty room, never throws; anchors+staticMeshes empty -> NoRenderableObjects; every considered object counted exactly once; ids stable via stableObjectId(object.id)
- **Failure modes:** None new — already handles missing/invalid doc, non-finite bounds, unsupported shapes via receipt skip-counters
- **Test shape:** existing: fixture document -> assert receipt counts + room.anchors[].kind strings + spatialSurfaces roles; determinism (same doc -> byte-identical RoomAsset)
- **Why:** The engine that turns authored Creative objects into the runtime scene payload — the whole lane's spine
- **Touch-count reduction:** n/a — already the single funnel; adding a kind is data (a descriptor row), not a code touch here

#### `anchorKindForDescriptor` — **EXISTS** · _slice-critical_ · risk: none  · **wrapper/extraction**
- **Owner:** `src/app/iggy3d/creative/adapters/RoomBake.* (anon namespace)`
- **Exists as:** anchorKindForDescriptor at RoomBake.cpp:253 — returns toString(descriptor.runtimeAnchorSemantic)
- **Caller:** classifyRoomBakeObject (RoomBake.cpp:445) — sets classification.anchorKind on BakeAnchor
- **Callees / deps:** toString(CreativeRuntimeAnchorSemantic) at ObjectDescriptor.cpp:1854
- **Inputs:** const CreativeObjectDescriptor&
- **Output / receipt:** std::string_view wire string (spawn/exit/npc/monster/pickup/light/audio/camera or "")
- **Invariants:** returns a vocab string, NEVER a generic occupancy string (the brief's 'seam break' premise is stale — verify against RoomBake.cpp:255); None -> "" which never bakes an anchor (guarded by descriptorSupportsRuntimeRoomAnchor)
- **Failure modes:** None — total function over the enum; the only 'gap' is enum values that don't exist yet (chokepoint/cover/... — see runtimeAnchorSemanticForKind)
- **Test shape:** for each anchor-bearing kind, assert baked anchor.kind == expected wire string; assert light/audio/camera anchors emit no reasoning node downstream
- **Why:** The exact join point of the Creative→AI seam; the wire-string contract lives here. Flagged FIX in the brief but is actually correct — the FIX is elsewhere (missing enum values)
- **Touch-count reduction:** n/a — thin wrapper over toString

#### `nodeKindForAnchor` — **EXISTS** · _slice-critical_ · risk: none
- **Owner:** `src/runtime/ai/ReasoningGraph.* (anon namespace)`
- **Exists as:** nodeKindForAnchor at ReasoningGraph.cpp:26 (consumed by buildReasoningGraph:156)
- **Caller:** buildReasoningGraph (ReasoningGraph.cpp:156)
- **Callees / deps:** none (pure string switch)
- **Inputs:** const std::string& anchorKind, ReasoningNodeKind& out
- **Output / receipt:** bool (mapped?) + out node kind; false = ignore-and-continue (no node, no error)
- **Invariants:** unknown string -> false, never throws (affordance vocab ignore-and-continue law); cases: exit/treasure/key/pickup/spawn/npc/monster/chokepoint/high_ground/hiding_spot/cover/patrol_post
- **Failure modes:** Reader cases on treasure/key/chokepoint/high_ground/hiding_spot/cover/patrol_post that NO current descriptor emits — those branches are dormant until enum values are added (they are correct, just unreached)
- **Test shape:** reasoning_graph_tests: hand-authored RoomAnchorAsset.kind -> assert node kind + count; unknown kind -> zero nodes, no error
- **Why:** The AI-side reader of the wire strings; proves which strings already produce nodes vs which are dead branches awaiting an emitter
- **Touch-count reduction:** n/a

#### `runtimeAnchorSemanticForKind + append 5-7 CreativeRuntimeAnchorSemantic values` — **PARTIAL** · _convenience_ · risk: low  · **wrapper/extraction**
- **Owner:** `src/app/iggy3d/creative/document/ObjectDescriptor.*`
- **Exists as:** field CreativeObjectDescriptor.runtimeAnchorSemantic (ObjectDescriptor.hpp:187) + enum (hpp:133) + toString (cpp:1854); NO by-kind free accessor and NO enum values for chokepoint/high_ground/hiding_spot/cover/patrol_post/treasure/key
- **Caller:** descriptor tables (per-kind rows) set the field; a free accessor would serve callers wanting semantic without the full descriptor; overlaps Codex E67 anchor-semantics
- **Callees / deps:** describeObject; toString(CreativeRuntimeAnchorSemantic)
- **Inputs:** CreativeObjectKind (accessor) / N/A (enum+row edits)
- **Output / receipt:** CreativeRuntimeAnchorSemantic; toString adds new wire strings chokepoint/high_ground/hiding_spot/cover/patrol_post/treasure/key
- **Invariants:** enum is APPEND-ONLY (serialized + notebook-read per vocab doc note 4); wire strings FIXED, never renamed; adding a value must not reorder existing (monster ordering caveat already resolved)
- **Failure modes:** If a new toString case is missed -> returns "" -> anchor silently drops (compiler -Wswitch catches enum exhaustiveness if enabled); coordinate with E67 to avoid duplicate/divergent enum additions
- **Test shape:** creative_object_descriptor_tests: for a kind assigned the new semantic, assert describeObject(kind).runtimeAnchorSemantic + toString == wire string; round-trip through bake -> reasoning node of expected kind
- **Why:** Closes the dead reader branches (chokepoint/cover/...) by giving them an authoring emitter — the only anchor-side gap the vocab doc marks 'no authoring emitter yet'
- **Touch-count reduction:** Adding a new affordance becomes ONE enum value + ONE descriptor row + ONE toString case — no reader change (reader already handles them)

#### `patrolRouteWaypointsFromDocument` — **NEW** · _slice-critical_ · risk: medium
- **Owner:** `src/app/iggy3d/creative/adapters/RoomBake.* (new free fn, sibling to buildRoomAssetFromCreativeDocument)`
- **Exists as:** —
- **Caller:** the (not-yet-existing) src call site that invokes buildReasoningGraph on a baked room — today only tests hand-feed literal Vec3 vectors; this is the missing production bridge
- **Callees / deps:** document.objects(); describeObject (to filter kind==PatrolRoute / shapeKind==Path); reads CreativeObject.pathPoints (Object.hpp:185); toVec3 projection (RoomBake.cpp:62); cross-lane: output feeds buildReasoningGraph's std::span<const Vec3> patrolWaypoints param
- **Inputs:** const CreativeDocument& (optionally a specific PatrolRoute objectId)
- **Output / receipt:** std::vector<Vec3> ordered waypoints (+ optionally a per-route provenance receipt)
- **Invariants:** waypoint order == authored pathPoints order (patrol semantics are ordered); non-finite points dropped or route rejected (mirror validAnchorPosition); empty/absent PatrolRoute -> empty span (buildReasoningGraph already tolerates empty); deterministic given a stable object iteration order
- **Failure modes:** Multiple PatrolRoute objects — must define ordering (per-object then pathPoint) or the span is non-deterministic; pathPoints with <2 points (MutationApply.cpp:78 already rejects on write, but a loaded/legacy doc could carry them) -> drop; coordinate-space mismatch (pathPoints local vs world — verify PathProjection semantics before assuming identity)
- **Test shape:** fixture doc with one PatrolRoute of N pathPoints -> assert returned vector size N and order; feed into buildReasoningGraph -> assert N patrolPost nodes at those positions; empty doc -> empty
- **Why:** PatrolRoute is fully authorable (descriptor + SetPatrolRoute mutation + pathPoints storage all EXIST) but NOTHING turns it into the reasoning-graph waypoint input — this is the single most real piece of new lane work
- **Touch-count reduction:** Establishes the one document->waypoint funnel so patrol authoring never needs to touch the reasoning graph directly

#### `traversalLinksFromDocument (bake NavLink/JumpLink/ClimbLink) + RoomTraversalLink payload` — **NEW** · _slice-critical_ · risk: high
- **Owner:** `src/app/iggy3d/creative/adapters/RoomBake.* + content/assets/RoomAsset.hpp (new struct)`
- **Exists as:** —
- **Caller:** buildRoomAssetFromCreativeDocument (extend classify/bake to emit links); downstream buildReasoningGraph edge stage
- **Callees / deps:** describeObject (kind in {NavLink,JumpLink,ClimbLink}, shapeKind==Line, profile==LinkOrRoute); reads the link's two endpoints; pushes RoomTraversalLink onto RoomAsset
- **Inputs:** CreativeObject link objects (NavLink/JumpLink/ClimbLink — ObjectDescriptor.cpp:634-674)
- **Output / receipt:** std::vector<RoomTraversalLink>{fromPos, toPos, kind} on RoomAsset (new payload field, sibling to anchors/spatialSurfaces)
- **Invariants:** link kind -> reasoning edge kind (NavLink->walkable/derived, ClimbLink->climb, JumpLink->climb-or-new-jump); endpoints must resolve to node positions or the link is dropped (never fabricate an edge); append-only RoomAsset field (serialized)
- **Failure modes:** These links currently bake to NOTHING — Line shape with hasBounds=false hits SkipNoBounds/SkipUnsupportedShape (RoomBake.cpp:449-457); their endpoint storage is unclear (markerDefaults, NoProjection) so endpoints may live in pathPoints or transform+bounds — MUST verify where NavLink stores its two ends before baking; if stored in bounds, hasBounds=false means the endpoints are absent entirely (a prerequisite authoring/storage gap, not just a bake gap)
- **Test shape:** doc with a ClimbLink between two anchor positions -> assert RoomAsset carries one RoomTraversalLink; buildReasoningGraph -> assert a climb edge between the corresponding node ids (not walkable)
- **Why:** NavLink/JumpLink/ClimbLink are authored kinds that produce NO runtime effect today; the reasoning graph emits only walkable edges. This is the climb/jump half of the traversal seam the first playable slice (thief parkour) needs
- **Touch-count reduction:** One link-bake path so every future traversal-link kind is data, not a new bake branch

#### `buildReasoningGraph (extend edge derivation for links)` — **FIX** · _slice-critical_ · risk: medium
- **Owner:** `src/runtime/ai/ReasoningGraph.*`
- **Exists as:** buildReasoningGraph at ReasoningGraph.cpp:149 — currently emits ONLY ReasoningEdgeKind::walkable (line 201); climb/hidden/locked edge kinds exist in the enum but are never emitted
- **Caller:** menu/Notebook (future include per hpp:23), stealth/guard readout tests, the patrol/link bridge callers
- **Callees / deps:** nodeKindForAnchor, buildSpatialSurfaceSet, bakePhysicsAabbCollidersFromSpatialSurfaces, reasoningSegmentBlocked; cross-lane: consumes RoomAsset (+ new traversal links) + patrol waypoint span
- **Inputs:** const RoomAsset&, std::span<const Vec3> patrolWaypoints, const ReasoningGraphConfig&
- **Output / receipt:** ReasoningGraph {nodes, edges} — deterministic, stable-ordered
- **Invariants:** MUST preserve the determinism law (sort by kind,x,z,y; id=sorted index; edges ascending from,to); adding link edges must NOT perturb existing walkable-edge ids or ordering; empty links -> byte-identical to today
- **Failure modes:** Breaking bitwise reproducibility if link edges are inserted before node-id assignment or in unstable order; edge de-dup if a link overlaps an all-pairs walkable edge; endpoint->node-id resolution must match the sorted node set (a link endpoint that isn't a node has no id)
- **Test shape:** reasoning_graph_tests: room with links -> assert climb/jump edges present with correct kind + endpoints; determinism test (same room+links -> identical graph); no-links room -> unchanged from current golden
- **Why:** The graph is the notebook's intel substrate; without non-walkable edges the AI can't reason about climb/jump routes the thief uses — needed for the guard-reacts-to-parkour slice
- **Touch-count reduction:** n/a

#### `roleForObject + descriptorSupportsRuntimeRoomGeometry` — **EXISTS** · _prerequisite_ · risk: none
- **Owner:** `src/app/iggy3d/creative/adapters/RoomBake.* (anon namespace)`
- **Exists as:** roleForObject at RoomBake.cpp:164; descriptorSupportsRuntimeRoomGeometry at RoomBake.cpp:112
- **Caller:** classifyRoomBakeObject (RoomBake.cpp:464)
- **Callees / deps:** occupancySupportsRuntimeRoomGeometry, horizontalSurface, standingSurface, descriptorSupportsBoundsBackedLineGeometry
- **Inputs:** const CreativeObjectDescriptor&, BakeBounds
- **Output / receipt:** BakedRoomRole (Floor/Wall/Prop/Unsupported)
- **Invariants:** role decided by shapeKind+occupancy+bounds aspect (Structural+horizontal->Floor, Structural+standing->Wall), NEVER per-kind (doctrine: shape decides editing); Unsupported -> object skipped
- **Failure modes:** Wall/floor discrimination is aspect-ratio heuristic (horizontalSurface) — a cube-ish structural surface is ambiguous; BoxVolume deliberately returns Unsupported (baked via spatial surfaces path, not static mesh)
- **Test shape:** existing room_bake tests: assert floor object -> Floor role -> walkable surface; standing wall -> Wall role -> wall-segment fields + actor/projectile blocker surfaces
- **Why:** Turns shape+occupancy into the runtime role that drives mesh, material, and which spatial surfaces get emitted — the generic-systems core of the bake
- **Touch-count reduction:** Role is derived from descriptor data, so new structural kinds get floor/wall behavior free

#### `appendSpatialSurfaces (walkable / actor-blocker / projectile-blocker derivation)` — **EXISTS** · _prerequisite_ · risk: none
- **Owner:** `src/app/iggy3d/creative/adapters/RoomBake.* (anon namespace)`
- **Exists as:** appendSpatialSurfaces at RoomBake.cpp:391 (+ walkableSurfaceForObject:331, actorBlockerSurfaceForObject:348, projectileBlockerSurfaceForObject:366)
- **Caller:** buildRoomAssetFromCreativeDocument (RoomBake.cpp:576)
- **Callees / deps:** topFacePoints, boxExtentPoints, blockerNormalForRole; output consumed cross-lane by buildSpatialSurfaceSet -> PhysicsAabbColliders that reasoningSegmentBlocked/vision/hearing all share
- **Inputs:** RoomAsset&, source vector&, CreativeObject, descriptor, BakeBounds, BakedRoomRole
- **Output / receipt:** appends RoomSpatialSurface entries (Walkable for floors; Blocker+ProjectileBlocker for Structural/Collision) + provenance sources
- **Invariants:** Floor -> one Walkable surface (blocksActor=false); Structural/Collision -> actor+projectile blocker pair; surface ids stable via stableObjectId suffixes; traversalTags/collisionMask populated so the reasoning-graph collider bake and senses see identical geometry
- **Failure modes:** blockerNormalForRole falls back to +Z for non-wall props (RoomBake.cpp:297) — a legacy default, not a true selected face; Navigation-occupancy objects support geometry but produce no blocker (only Structural/Collision do)
- **Test shape:** room_bake tests assert spatialSurfaces roles/flags per object; downstream: buildReasoningGraph edge test proves a blocker surface actually blocks a walkable edge
- **Why:** These surfaces ARE the shared collision truth the reasoning graph's walkable edges are cut against — the physical-to-reasoning bridge already in place
- **Touch-count reduction:** n/a

#### `entityFromAnchor (PackageSessionSeed — anchor->session entity)` — **EXISTS** · _validation_ · risk: low
- **Owner:** `src/app/iggy3d/world/PackageSessionSeed.cpp`
- **Exists as:** entityFromAnchor at PackageSessionSeed.cpp:156 (+ npcFromAnchor:66, pickupFromAnchor:85, exitFromAnchor:120, doorFromAnchor:101, markerFromAnchor:140)
- **Caller:** PackageSessionSeed session-seeding flow (per-anchor)
- **Callees / deps:** baseEntity, objectiveIdFor; cross-lane: reads RoomAnchorAsset.kind wire strings (same strings the bake emits + reasoning reads)
- **Inputs:** const RoomAnchorAsset&, firstKeyItemId, firstTreasureItemId
- **Output / receipt:** ScenarioEntitySeed (EntityKind + interaction + stableName=anchor.id)
- **Invariants:** npc AND monster seed identically today (vocab v0.1, PackageSessionSeed.cpp:159-162); unknown kind -> markerFromAnchor fallback (generic marker entity, never error — but DOES shift entity counts/hash, per vocab doc honest-scope note); pickup/key/treasure collapse to pickup seed
- **Failure modes:** The 'harmless unknown kind' is only harmless for reasoning nodes — here it fabricates a marker ENTITY, so authoring a NEW anchor kind shifts session entity counts/receipts/hash (documented; watch when adding the 5 new semantic values)
- **Test shape:** package/session-seed tests: anchor of each kind -> assert EntityKind + interaction; monster seeds == npc seed; unknown kind -> marker entity (count-shift asserted)
- **Why:** The third reader of the same wire strings (bake emits, reasoning reads for thought, this reads for bodies) — proves the anchor vocab is a genuinely shared 3-way contract and flags the entity-count side effect of new kinds
- **Touch-count reduction:** n/a


### Lane 3 — AI / recon / notebook

> **Existing surface:** Roughly a third real, two-thirds greenfield — but the real third is exactly the load-bearing third. The DURABLE guard state this lane must read is fully built and already saved/hashed: AiActorState carries alertLevel (0..1), patrolWaypoints + patrolMode + patrolTargetIndex + patrolForward, facingDirection, lastKnownTargetPosition/Tick/hasLastKnownTarget, and searchLastReceipt (a full GuardDecisionReceipt kept AS STATE), with a matching SaveAiActorRecord in SaveEnvelope.hpp:315 that persists all of it. alertBandIndex(level,profile) + alertBehaviorForLevel EXIST in NpcAlertSystem.hpp (bands 0..5). The reasoning graph is real end-to-end: buildReasoningGraph + summarizeReasoningGraph + ReasoningGraphSummary (per-kind counts over 14 node kinds) exist, and SessionState.reasoningGraph is populated once at activation (Session.cpp:1311) and read by the guard loop. The notebook RENDER half is real: ProductNotebookReconPage struct + buildProductNotebookUiDrawList + ProductNotebookUiRequest all ship (menu/Notebook.hpp) through the L1 widget layer, receipt-tested. The StableHasher stack (addFloatQuantized/addU64/addString/addVec3Quantized, StateHash.cpp) is the exact template a recon-intel hash reuses. WHAT IS ABSENT (verified by grep returning nothing): GuardReconObservation, projectGuardRecon, alertBandName, ReconIntel, captureReconIntel, hashReconIntel, serializeReconIntel, buildNotebookReconPage (a PRODUCER — the page is only ever filled by the test fixture sampleReconPage()), transferReconIntel, and any duo consumer (grep for priestess/knight/spectre/duo hits zero source files; PlayerRoster is slot/EntityId only, no roles). So: the guard-observation SOURCE and the notebook SINK both exist as data structures; the entire PROJECTION → INTEL PACKET → PRODUCER → TRANSFER pipeline between them does not.


> **Lane summary:** The actual new work is a single pure data spine bridging two things that already exist: durable AiActorState (source) and ProductNotebookReconPage (sink). Nothing in this lane needs new AI behavior, new perception, or new rendering — it is all pure projection + a hash + a serializer + a producer + a transfer, each a thin function over structs that already carry the DNA. Fire order: (1) alertBandName — a 6-arm switch wrapper over the existing alertBandIndex, trivial, no prerequisite. (2) GuardReconObservation + projectGuardRecon — a PURE projector (durable AiActorState + a passed guard position + the session reasoningGraph → an observation of {band, facing, patrol route as node refs, last-known sighting, decision receipt factors}); its ONLY hard dependency is that the reasoningGraph be populated, which it is at activation. (3) ReconIntel + captureReconIntel — aggregates N GuardReconObservations plus a room/floor-plan sketch into one transferable packet; THIS is where the live cross-lane seam bites: the floor-plan/garrison/patrol strings the notebook wants come from RoomAsset anchors + affordance markers, and per the master plan the affordance wire-strings are emitted by NEITHER bake path yet — so captureReconIntel can ship its guard/patrol half from real state but must degrade gracefully (empty floorPlan) until Lane 2 closes the marker-export seam. (4) hashReconIntel + serializeReconIntel — direct reuse of the StableHasher pattern and the SaveEnvelope record pattern; determinism-testable in isolation. (5) buildNotebookReconPage — the missing PRODUCER: ReconIntel → ProductNotebookReconPage (which then feeds the already-shipped draw-list builder). (6) transferReconIntel + a duo consumer — the most speculative; PlayerRoster has no role concept and the knight/priestess side does not exist, so this is DEFER until there is a second controllable actor to receive intel. RISKS: the floor-plan derivation is blocked on the Lane 2 affordance-export seam (the one known live break); the duo half has no landing surface yet; everything else is low-risk pure code with obvious tests.

#### `alertBandName` — **NEW** · _prerequisite_ · risk: none  · **wrapper/extraction**
- **Owner:** `src/runtime/ai/NpcAlertSystem.{hpp,cpp}`
- **Exists as:** alertBandIndex(float,const AlertProfile&) returns the 0..5 index at NpcAlertSystem.hpp (declared alongside alertBehaviorForLevel); the NAME wrapper is absent. Precedent wrapper: aiBehaviorKindName in AiState.hpp.
- **Caller:** projectGuardRecon (to label the observed band); buildNotebookReconPage indirectly; any readout/HUD
- **Callees / deps:** alertBandIndex (or takes the index directly)
- **Inputs:** a float alert level + AlertProfile, OR just the std::uint8_t band index
- **Output / receipt:** std::string_view stable name ("idle"/"observant"/"suspicious"/"searching"/"agitated"/"combat")
- **Invariants:** total over all 6 bands; identical input -> identical string; append-only stable spellings (they get serialized into intel/notebook); never returns empty
- **Failure modes:** out-of-range index -> return "idle" fallback (mirrors patrolModeName/aiBehaviorKindName default), never UB
- **Test shape:** table test: each of the 6 bands maps to its expected string; an out-of-range index returns the fallback
- **Why:** Recon intel and the notebook need a human/stable label for the guard's alert band; the index alone is opaque and not append-safe to serialize
- **Touch-count reduction:** one name authority so intel, notebook, and any HUD share a single spelling instead of three ad-hoc switch statements

#### `projectGuardRecon` — **NEW** · _slice-critical_ · risk: low
- **Owner:** `src/runtime/ai/GuardRecon.{hpp,cpp} (NEW module; sibling to ReasoningGraph.* under runtime/ai)`
- **Exists as:** —
- **Caller:** captureReconIntel; a guard-recon unit test; potentially a debug readout
- **Callees / deps:** alertBandName, alertBandIndex, patrolModeName, reasoningNodeKindName; reads AiActorState fields + the passed ReasoningGraph
- **Inputs:** const AiActorState& (durable guard), Vec3 currentGuardPosition (passed, NOT read from session — same discipline maybeApplySearch uses), const ReasoningGraph& (session.reasoningGraph)
- **Output / receipt:** GuardReconObservation struct: {EntityId actor; std::string_view bandName; std::uint8_t bandIndex; Vec3 facingDirection; Vec3 position; std::vector<Vec3> patrolRoute; PatrolMode patrolMode; bool hasLastKnown; Vec3 lastKnownPosition; std::uint64_t lastKnownTick; GuardDecisionReceipt lastReceipt (copied); std::vector<std::uint32_t> nearestReasoningNodeIds}
- **Invariants:** PURE — no session, no tick source beyond what is passed, no RNG (mirrors buildReasoningGraph's determinism law); identical (actor,pos,graph) -> bitwise-identical observation; reads ONLY durable AiActorState fields (never the transient route/search working set unless explicitly chosen)
- **Failure modes:** empty patrolWaypoints -> empty patrolRoute (valid, back-compat, like the no-patrol guard); empty reasoningGraph -> empty nearestReasoningNodeIds (valid, post-load state); hasLastKnownTarget=false -> lastKnown fields left default and flagged
- **Test shape:** given a hand-built AiActorState (alertLevel in each band, a 3-waypoint loop, a last-known sighting) + a small ReasoningGraph, assert the observation's band name, route length, and last-known flag; assert determinism (two calls equal); assert empty-graph/empty-route degrade cleanly
- **Why:** The projector is the SOURCE of all recon: it turns the durable guard state the game already saves into an inspectable observation the intel packet and notebook read. Everything downstream depends on it
- **Touch-count reduction:** n/a

#### `GuardReconObservation` — **NEW** · _slice-critical_ · risk: none
- **Owner:** `src/runtime/ai/GuardRecon.hpp (NEW struct)`
- **Exists as:** —
- **Caller:** projectGuardRecon returns it; captureReconIntel consumes a vector of them; tests
- **Callees / deps:** none (plain data); composes existing types EntityId/Vec3/PatrolMode/GuardDecisionReceipt
- **Inputs:** n/a (data struct)
- **Output / receipt:** n/a
- **Invariants:** plain-old-data, copyable, no owning pointers (unlike ProductNotebookUiRequest which holds a raw page*); append-only field order if it later serializes
- **Failure modes:** n/a
- **Test shape:** covered by projectGuardRecon tests
- **Why:** The typed receipt of one guard's scoutable state; the unit of aggregation for ReconIntel
- **Touch-count reduction:** n/a

#### `captureReconIntel` — **NEW** · _slice-critical_ · risk: medium
- **Owner:** `src/runtime/ai/ReconIntel.{hpp,cpp} (NEW module)`
- **Exists as:** —
- **Caller:** a scout/notebook action handler at capture time; buildNotebookReconPage's caller; tests
- **Callees / deps:** projectGuardRecon (per guard), alertBandName; cross-lane: RoomAsset anchors/markers for the floor-plan+garrison strings (Lane 2)
- **Inputs:** std::span<const AiActorState> guards, a position resolver (guard positions from WorldState/entities), const ReasoningGraph&, const RoomAsset& (for floor plan + anchor-derived garrison/patrol labels), a title/scope
- **Output / receipt:** ReconIntel struct: {std::string title; std::vector<GuardReconObservation> guards; std::vector<std::string> floorPlan; std::vector<std::string> garrison; std::vector<std::string> patrols; std::vector<std::string> hazards; std::uint64_t captureTick}
- **Invariants:** pure over its inputs; deterministic; guard half is derivable from real durable state TODAY; floor-plan/garrison/hazard half degrades to empty vectors when the anchor/affordance strings are unavailable
- **Failure modes:** CROSS-LANE PREREQUISITE UNMET: the affordance/marker wire-strings the floor plan needs are emitted by NEITHER bake path yet (the one known live seam break in the master plan) — so floorPlan/hazards ship empty until Lane 2 closes the marker-export seam; empty guards span -> empty guards vector (valid)
- **Test shape:** given 2 hand-built guards + a ReasoningGraph, assert ReconIntel.guards has 2 observations with correct bands; assert floorPlan is empty when no RoomAsset markers are provided (documents the degraded path); assert determinism
- **Why:** The transferable intel packet — the interface between the thief side and the duo side named in the game vision; aggregates per-guard observations + the room sketch into one hashable/serializable unit
- **Touch-count reduction:** n/a

#### `ReconIntel` — **NEW** · _slice-critical_ · risk: none
- **Owner:** `src/runtime/ai/ReconIntel.hpp (NEW struct)`
- **Exists as:** Field-for-field it is a SUPERSET of ProductNotebookReconPage (menu/Notebook.hpp): floorPlan/garrison/patrols/hazards already exist there as std::vector<std::string>. ReconIntel is the runtime-side, guard-aware source; the notebook page is its rendered projection.
- **Caller:** captureReconIntel returns it; hash/serialize/buildNotebookReconPage/transfer consume it
- **Callees / deps:** composes GuardReconObservation + std::string vectors
- **Inputs:** n/a
- **Output / receipt:** n/a
- **Invariants:** plain data, copyable; append-only field order (it serializes + hashes); the string-vector fields deliberately MATCH ProductNotebookReconPage's spellings so the producer is a straight copy for that half
- **Failure modes:** n/a
- **Test shape:** covered by captureReconIntel + hash + serialize tests
- **Why:** The durable, transferable, hashable intel record; the asymmetric-co-op payload
- **Touch-count reduction:** reuses ProductNotebookReconPage's exact string-field shape so buildNotebookReconPage is a memberwise copy, not a translation layer

#### `hashReconIntel` — **NEW** · _validation_ · risk: low  · **wrapper/extraction**
- **Owner:** `src/runtime/ai/ReconIntel.cpp (NEW)`
- **Exists as:** No recon hasher exists, but the ENTIRE mechanism exists: StableHasher (core/hash/StableHash.hpp: addU64/addString/addFloatQuantized/addBool + addVec3Quantized) and the field-hashing helper pattern in StateHash.cpp (addU64/addVec3Field/addFloatField/addEnum). This is that pattern applied to ReconIntel fields.
- **Caller:** save/replay verification; a transfer-integrity check in transferReconIntel; determinism tests
- **Callees / deps:** StableHasher::add* (addString/addU64/addFloatQuantized/addVec3Quantized)
- **Inputs:** const ReconIntel&
- **Output / receipt:** StableHashValue (std::uint64_t)
- **Invariants:** identical intel -> identical hash (bitwise); tag-per-field like StateHash.cpp so field reorder/rename changes the hash; floats quantized (never raw) so it survives serialize round-trip; order-stable over the guards vector
- **Failure modes:** unquantized float would break round-trip equality — mitigated by reusing addFloatQuantized; a field added without a tag would silently not hash — mitigated by mirroring the StateHash discipline
- **Test shape:** hash a fixed ReconIntel; assert stable value; assert a single-field mutation changes it; assert serialize->deserialize->hash equals the original hash
- **Why:** Recon intel is a save/replay artifact and a cross-side transfer payload; it needs a deterministic identity for verification, exactly like SessionState's currentStateHash
- **Touch-count reduction:** reuses the StableHasher + StateHash field-helper pattern wholesale rather than a new hashing scheme

#### `serializeReconIntel` — **NEW** · _validation_ · risk: low  · **wrapper/extraction**
- **Owner:** `src/runtime/save/ (extend SaveEnvelope.hpp with a SaveReconIntelSection + SaveCodec support) OR src/runtime/ai/ReconIntel.cpp`
- **Exists as:** No recon serializer, but SaveCodec + SaveEnvelope already serialize the near-identical SaveAiActorRecord (SaveEnvelope.hpp:315, including patrolWaypoints/alertLevel/lastKnownTargetPosition/facingDirection) and vectors of records. This is a new SaveReconIntelSection following that exact codec pattern.
- **Caller:** save flow when intel is persisted into a notebook/session; transferReconIntel (to snapshot before handoff); tests
- **Callees / deps:** SaveCodec primitives (the same ones SaveAiSection uses)
- **Inputs:** const ReconIntel& (write) / a byte buffer or SaveEnvelope section (read)
- **Output / receipt:** a SaveReconIntelSection / byte stream; and the inverse deserialize -> ReconIntel
- **Invariants:** round-trip identity: deserialize(serialize(x)) == x AND hashReconIntel(that) == hashReconIntel(x); append-only record fields with defaults (SaveAiActorRecord back-compat precedent); schema-versioned like the rest of the envelope
- **Failure modes:** forgetting a default on a new field breaks old-save load — mitigated by the SaveAiActorRecord default-matching convention; the reasoningNodeIds/receipt sub-objects need explicit codec support
- **Test shape:** round-trip a populated ReconIntel through serialize+deserialize; assert field equality and hash equality; assert an older/short record loads with defaults
- **Why:** Intel must survive save/load and be movable between the two co-op sides as bytes, not just in-memory
- **Touch-count reduction:** n/a

#### `buildNotebookReconPage` — **NEW** · _slice-critical_ · risk: low  · **wrapper/extraction**
- **Owner:** `src/app/iggy3d/menu/Notebook.{hpp,cpp} (extend) — the PRODUCER the module explicitly notes is a 'separate step'`
- **Exists as:** The SINK exists: ProductNotebookReconPage struct + buildProductNotebookUiDrawList (menu/Notebook.hpp) ship and are receipt-tested. What is absent is any PRODUCER — today the page is only filled by the test fixture sampleReconPage() in product_notebook_ui_draw_list_tests.cpp:39. This function is that missing producer.
- **Caller:** the in-game notebook open/refresh action; a producer test
- **Callees / deps:** alertBandName (to render guard rows); reads ReconIntel fields
- **Inputs:** const ReconIntel&, a page number
- **Output / receipt:** ProductNotebookReconPage (title, floorPlan, garrison, patrols, hazards, pageNumber)
- **Invariants:** pure (intel -> page); deterministic; the string-vector fields are a near-memberwise copy since ReconIntel deliberately mirrors the page's shape; guard observations render into garrison/patrols rows deterministically ordered
- **Failure modes:** empty floorPlan (the unmet Lane 2 seam) still produces a VALID page (blank sketch, populated garrison from real guard state) — the notebook must not require the floor plan to render; more guards than page rows -> deterministic truncation, not overflow
- **Test shape:** given a ReconIntel with 2 guard observations and empty floorPlan, assert the page has 2 garrison rows with the right band names and an empty-but-valid floorPlan; feed the page into buildProductNotebookUiDrawList and assert it still emits a receipt
- **Why:** Closes the loop from live guard state to the rendered Moleskine page — the visible payoff of the whole lane and the thief-side of the co-op interface
- **Touch-count reduction:** reuses the shipped draw-list builder unchanged; this is the one missing arrow into it

#### `transferReconIntel` — **DEFER** · _deferred_ · risk: high
- **Owner:** `src/runtime/session/ or a new co-op module (NEW) — landing surface does not exist yet`
- **Exists as:** —
- **Caller:** an asymmetric co-op handoff action (does not exist); a transfer test
- **Callees / deps:** hashReconIntel (integrity check), serializeReconIntel (if crossing a boundary)
- **Inputs:** const ReconIntel& (from the thief/scout side), a destination consumer handle
- **Output / receipt:** a receipt {bool accepted; StableHashValue intelHash; reason} + the intel delivered to the consumer
- **Invariants:** the delivered intel hashes identically on both sides (hashReconIntel before == after); transfer is idempotent by hash; never mutates the source intel
- **Failure modes:** NO CONSUMER EXISTS — grep for priestess/knight/spectre/duo returns zero source files; PlayerRoster is slot+EntityId only, with no role concept. There is nothing to receive the intel, so this cannot be validated end to end yet
- **Test shape:** once a second actor/role exists: transfer a ReconIntel, assert the consumer holds intel with a matching hash; assert source unchanged
- **Why:** The notebook is the interface between the thief and the knight+priestess duo (game vision); this is the actual handoff
- **Touch-count reduction:** n/a

#### `duoConsumeReconIntel` — **DEFER** · _deferred_ · risk: high
- **Owner:** `src/runtime/ (NEW — the knight+priestess side has no home)`
- **Exists as:** —
- **Caller:** transferReconIntel; the duo's decision/planning layer
- **Callees / deps:** reads ReconIntel guard observations + reasoning node refs to seed the duo's plan
- **Inputs:** const ReconIntel&, the duo's state
- **Output / receipt:** whatever the duo's tactical layer needs (marked positions, known patrols) + a consume receipt
- **Invariants:** read-only over the intel; deterministic seeding; consuming twice with the same intel is idempotent
- **Failure modes:** the duo side does not exist (no roles in PlayerRoster, no knight/priestess actors, no second-side decision layer); this is pure downstream speculation until a controllable second side ships
- **Test shape:** deferred until the duo actor exists; then: consume a fixed intel, assert the duo's known-patrols/known-guards reflect the observations
- **Why:** The downstream consumer that makes recon MATTER — intel with no reader is dead data; but per the target-first doctrine the reader is where leverage lives, and this reader has no surface yet
- **Touch-count reduction:** n/a


### Lane 4 — Pre-Bake Validation (validate → REPAIR → receipt)

> **Existing surface:** Roughly 55% of this lane already exists in some form — but scattered across three subsystems and NONE of it is unified under a repairable umbrella receipt. The three real precedents: (1) AsciiRoomGrid.cpp is the exact receipt shape to mirror — buildAsciiRoomGrid returns {ok, status, reasonCode, vector<AsciiRoomDiagnostic{severity,reasonCode,message}>} and already runs concrete checks (no_floor, missing_player_spawn, multiple_player_spawns). It is NOT flood-fill; it counts, it does not traverse. (2) RoomBake.cpp already silently validates at bake time: validBakeBounds() (lines 73-84) is a full degenerate/non-finite bounds guard, and the classification path emits skippedNoBoundsCount/skippedUnsupportedAnchorCount/skippedUnsupportedShapeCount — so 'degenerate bounds', 'bad anchor', and 'missing descriptor facts' are ALREADY DETECTED, just as silent skip-counters with no repair and no per-object diagnostic. (3) validateCreativeObjectParentGraph (Document.cpp:298) already fully covers 'stale parent ids' + parent cycles, and is already called on every mutation via DocumentMutation.cpp:115. Reachability exists as graph Dijkstra (planRoute/nearestReachableNode in ReasoningRoute.cpp) over a ReasoningGraph, and the occupancy substrate exists (projectObjectsToGrid → CreativeSpatialCell grid). What is genuinely greenfield: the umbrella CreativeDocument-level validateDocumentPreBake receipt, a reusable GRID flood-fill primitive (the graph one does not serve token-gen/patrol on a raw occupancy grid), floating/grounded detection, overlapping-structural-volume detection built on the occupancy grid, and disconnected-patrol-route checking (patrol data today lives on runtime AiActorState, not on document objects — so this check is blocked on the authoring representation existing).


> **Lane summary:** Lane 4 is more extraction-and-unification than greenfield. Four of the ten pieces already exist in working form and honesty demands saying so: stale-parent/cycle detection is DONE (validateCreativeObjectParentGraph, Document.cpp:298, already wired into every mutation); degenerate-bounds detection is DONE inside bake (validBakeBounds, RoomBake.cpp:73-84) but buried as a silent skip; bad/unsupported-anchor detection is DONE inside bake (anchorKindForDescriptor + SkipUnsupportedAnchor); descriptor coverage is DONE as a static test (creative_object_descriptor_tests.cpp:142, overlapping Codex E59). The ACTUAL new work is three-fold. (1) The umbrella: validateDocumentPreBake, a read-only CreativeDocument-level function returning a receipt of CreativeValidationDiagnostic that — crucially — carries a suggestedRepair CreativeMutationRequest, turning today's silent RoomBake skip-counters into named, repairable diagnostics. This is the doctrine spine (validate→REPAIR→receipt) and the one new receipt type the lane must define. (2) floodFillReachability, the highest-leverage NEW primitive: reachability today is graph Dijkstra (planRoute/nearestReachableNode over a ReasoningGraph), which does not serve a raw authored occupancy grid; a grid flood over the EXISTING projectObjectsToGrid occupancy is what token-gen, navmesh, patrol, and parkour all need and none have. (3) The genuinely-absent spatial checks that build on that flood + the existing occupancy projection: floating/grounded detection (NEW, nothing computes support-below today), overlapping-structural-volume detection (a NEW reader over the EXISTING occupancy grid), and patrol-route connectivity (NEW and partly BLOCKED — authored patrol waypoints live only on runtime AiActorState, not as document objects yet, so this check waits on the authoring representation). The correct build order: define the diagnostic/repair receipt type first, then floodFillReachability, then extract the three already-existing bake-time checks into diagnostics (cheap, high honesty payoff, and forces bake+validator to share one predicate so they can never drift), then the two new grid readers, and defer patrol connectivity until authored routes exist. Biggest risk across the lane is predicate DRIFT: if the validator and RoomBake keep separate copies of 'is this bakeable', an object can pass validation and still vanish at bake — every extraction must lift the shared predicate into a common header so the two agree by construction.

#### `validateDocumentPreBake` — **NEW** · _slice-critical_ · risk: medium
- **Owner:** `src/app/iggy3d/creative/validation/DocumentValidation.* (NEW file family)`
- **Exists as:** No document-wide validator exists. The closest umbrella is buildAsciiRoomGrid (AsciiRoomGrid.cpp:87-262) which returns the receipt+diagnostics shape to copy, and RoomBake's classify loop (RoomBake.cpp:440-500) which already runs per-object skip decisions silently. This function unifies those into a repairable receipt over a CreativeDocument.
- **Caller:** Bake flow (buildRoomAssetFromCreativeDocument's caller in adapters/RoomBake), a future 'Validate' UI action in Facade, and token-gen before it commits
- **Callees / deps:** floodFillReachability, checkDegenerateBounds, checkFloatingObjects, checkOverlappingStructuralVolumes, checkPatrolRouteConnectivity, checkAnchorsDropped, validateCreativeObjectParentGraph (Document.cpp:298, EXISTS), checkDescriptorCoverage; reads CreativeDocument::objects() and projectObjectsToGrid (cross-lane: spatial)
- **Inputs:** const CreativeDocument&, CreativeValidationRequest{cellSize, includeHidden, treatUnknownAsError}
- **Output / receipt:** CreativeDocumentValidationReceipt{bool ok; string reasonCode; vector<CreativeValidationDiagnostic{severity, reasonCode, objectId, message, bool repairable, CreativeMutationRequest suggestedRepair}>; counts}
- **Invariants:** Pure/read-only over the document (never mutates); deterministic ordering (objects in document order, checks in fixed order); ok==true IFF zero error-severity diagnostics; every diagnostic carries a reasonCode string that is stable across runs.
- **Failure modes:** Empty document → ok with reasonCode 'creative_doc_empty' (warning, not error); a check throwing must be contained so one bad object cannot abort the whole receipt; must not double-count an object flagged by two checks in the top-level counts.
- **Test shape:** Table test: build a document with one seeded defect per case (degenerate wall, floating platform, overlapping volumes, orphan parent, unreachable spawn), assert exactly the expected reasonCode appears once and ok==false; a clean document asserts ok==true with empty error diagnostics.
- **Why:** The doctrine's validate→REPAIR→receipt spine needs ONE entry point that produces a repairable receipt before bake, instead of RoomBake silently dropping objects into skip-counters. This is the umbrella every other check hangs under.
- **Touch-count reduction:** Collapses the N places that today each re-derive 'is this object bakeable' (RoomBake classify, future token-gen, future navmesh) into one receipt they all read — adding a new check becomes one row here, not a new guard in every consumer.

#### `floodFillReachability` — **PARTIAL** · _slice-critical_ · risk: medium
- **Owner:** `src/app/iggy3d/creative/validation/Reachability.* (NEW file family)`
- **Exists as:** Reachability EXISTS but only as GRAPH Dijkstra: nearestReachableNode + the frontier loop in ReasoningRoute.cpp:18-115 and planRoute (ReasoningRoute.hpp:57). That operates on a ReasoningGraph, not a raw occupancy grid. The occupancy grid substrate also EXISTS: projectObjectsToGrid (SpatialProjection.hpp:123) → vector<CreativeSpatialCell>. What is missing is the BFS/flood over that grid from seed cells.
- **Caller:** validateDocumentPreBake (for unreachable-spawn/unreachable-link checks), and cross-lane: token-gen, navmesh build, patrol validation, parkour reach — the highest-leverage shared primitive
- **Callees / deps:** projectObjectsToGrid (SpatialProjection, cross-lane spatial), occupancyKindForObject to decide which cells are walls vs floor
- **Inputs:** const CreativeSpatialGrid& (or the projected cells + CreativeGridSize3), span<seed CreativeGridCoord3>, a blocked-predicate (which occupancyKind blocks)
- **Output / receipt:** CreativeReachabilityResult{vector<bool> reachableMask indexed by grid cell; count reachableCells; count unreachableWalkableCells; vector<CreativeGridCoord3> unreachableSeedsOrRegions}
- **Invariants:** 6- or 4-connectivity fixed and documented; deterministic (seed order does not change the mask, only the region-id labelling if any); a cell blocked by a Structural/Collision occupancy is never marked reachable; pure.
- **Failure modes:** Empty seed set → whole mask false (caller must treat as 'nothing reachable', not crash); grid larger than expected memory → must cap or stream; diagonal-vs-orthogonal choice must match the movement lane's traversal or reachability lies.
- **Test shape:** Grid fixture: a floor split by a full wall with a door gap → seed on one side, assert the far side is reachable through the gap; seal the gap → assert far side unreachableWalkableCells>0. Determinism test: two different seed orderings produce identical masks.
- **Why:** Named in scope as 'BUILD ONCE, reused by token-gen/navmesh/patrol/parkour — the highest-leverage validator.' The graph Dijkstra does not serve a raw authored-grid flood; this is the missing grid primitive that four consumers need.
- **Touch-count reduction:** One flood primitive instead of each consumer (token-gen, navmesh, patrol, parkour) hand-rolling its own reachability over the same occupancy grid.

#### `checkDegenerateBounds` — **FIX** · _validation_ · risk: low  · **wrapper/extraction**
- **Owner:** `src/app/iggy3d/creative/validation/DocumentValidation.*`
- **Exists as:** validBakeBounds (RoomBake.cpp:73-84) already checks finiteness and max>min on all three axes AND clamps to float range — it is a complete degenerate-bounds detector, but it lives INSIDE the bake as a silent skip (SkipNoBounds, RoomBake.cpp:459) with no diagnostic and no repair. This function EXTRACTS that logic into a validator that emits a repairable diagnostic.
- **Caller:** validateDocumentPreBake
- **Callees / deps:** objectHasBounds/describeObject (ObjectDescriptor, EXISTS) to know if bounds are even expected; reuse the validBakeBounds predicate (should be lifted to a shared header so bake and validator agree)
- **Inputs:** const CreativeObject&, const CreativeObjectDescriptor&
- **Output / receipt:** optional<CreativeValidationDiagnostic> with reasonCode 'creative_bounds_degenerate' and suggestedRepair = a SetBoundsMutation inflating to a minimum non-zero AABB around the object center
- **Invariants:** Only flags objects whose descriptor.hasBounds==true (a Point marker with no bounds is not degenerate); the predicate MUST be the same one bake uses, or an object could pass validation and still be skipped at bake — the whole point of the extraction.
- **Failure modes:** If bake and validator drift (two copies of the min>max rule), validation lies; NaN/inf must be caught as degenerate not silently compared; a legitimately thin surface (zero-depth Surface shapeKind) must not be falsely flagged — the rule must key off shapeKind.
- **Test shape:** Object with max==min on one axis → diagnostic with reasonCode 'creative_bounds_degenerate' and a suggestedRepair whose applied result passes validBakeBounds; a valid box → no diagnostic. Cross-check test: any object the validator passes is NOT counted in skippedNoBoundsCount by bake.
- **Why:** Degenerate bounds are explicitly in scope and are the #1 cause of silent bake drops today; surfacing + repairing them is pure doctrine.
- **Touch-count reduction:** Bake and validator share ONE bounds predicate instead of two copies that can disagree.

#### `checkFloatingObjects` — **NEW** · _validation_ · risk: medium
- **Owner:** `src/app/iggy3d/creative/validation/DocumentValidation.*`
- **Exists as:** No grounding/floating detection exists anywhere in creative. Bounds and transform.position exist on CreativeObject (Object.hpp) and the occupancy grid exists, but nothing computes 'is there support beneath this object'. AsciiRoomGrid has elevation/floor data but no float check either.
- **Caller:** validateDocumentPreBake
- **Callees / deps:** projectObjectsToGrid or a direct AABB-below query over document objects (cross-lane spatial); describeObject to know which kinds are expected to be grounded (structural vs a hanging light)
- **Inputs:** const CreativeObject&, span<const CreativeObject> (to find support below), grounding tolerance
- **Output / receipt:** optional<CreativeValidationDiagnostic> reasonCode 'creative_object_floating', suggestedRepair = MoveMutation snapping the object down onto the nearest surface below (drop-to-floor)
- **Invariants:** Only structural/grounded-expected kinds are candidates (a ceiling-mounted light or a flying platform is not 'floating'); a descriptor flag or category (Structural) gates candidacy; tolerance is explicit, not a magic number.
- **Failure modes:** False positives on intentionally elevated geometry (platforms, upper floors) if the 'expected grounded' set is too broad; the drop-repair could drop an object into/through another object if it does not consult occupancy; objects on a ramp/slope need slope-aware support, not flat-Y.
- **Test shape:** Floor at y=0, a Wall whose min.y=2 with nothing beneath → diagnostic 'creative_object_floating' + a drop repair landing it at y=0; the same wall resting on the floor → no diagnostic; an intentional Platform at height → no diagnostic (not in candidate set).
- **Why:** Floating (non-grounded) objects are explicitly in scope; a floating wall bakes to a collider a thief can walk under — a real slice-breaking authoring bug.
- **Touch-count reduction:** n/a

#### `checkOverlappingStructuralVolumes` — **PARTIAL** · _validation_ · risk: medium  · **wrapper/extraction**
- **Owner:** `src/app/iggy3d/creative/validation/DocumentValidation.*`
- **Exists as:** The occupancy substrate EXISTS: projectObjectsToGrid (SpatialProjection.hpp:123) already stamps each object's cells with objectId+occupancyKind into a shared grid; a cell claimed by two Structural objects IS an overlap. But nothing today reads the grid for collisions — projection is used for occupancy, not conflict detection. This is a NEW reader over an EXISTING projection.
- **Caller:** validateDocumentPreBake
- **Callees / deps:** projectObjectsToGrid (cross-lane spatial, EXISTS); occupancyKindForObject (EXISTS) to filter to Structural/Collision
- **Inputs:** span<const CreativeObject>, CreativeSpatialProjectionRequest (cellSize, grid size)
- **Output / receipt:** vector<CreativeValidationDiagnostic> reasonCode 'creative_structural_overlap', each naming the two objectIds; repair is advisory (suggestedRepair may be empty — auto-resolving an overlap is ambiguous, so severity 'warning' unless fully enclosed)
- **Invariants:** Only Structural/Collision occupancy pairs count (a Trigger volume legitimately overlaps a floor); symmetric pairs reported once (A,B not also B,A); deterministic pair ordering by ascending objectId.
- **Failure modes:** Grid cellSize too coarse → misses thin overlaps or false-merges adjacent walls sharing a boundary cell (boundary-touching is NOT overlap — must test interior cell co-claim, not edge adjacency); huge grids blow memory; a wall and its own door legitimately co-occupy and must be whitelisted via parent relationship.
- **Test shape:** Two walls occupying the same cells → one 'creative_structural_overlap' diagnostic naming both ids; two flush-adjacent walls sharing only a boundary → no diagnostic; a Trigger over a Floor → no diagnostic (not structural).
- **Why:** Overlapping structural volumes are in scope; they double-bake colliders and corrupt the room a guard navigates.
- **Touch-count reduction:** Reuses the existing occupancy projection instead of a new spatial index just for overlap.

#### `checkAnchorsDropped` — **FIX** · _slice-critical_ · risk: low  · **wrapper/extraction**
- **Owner:** `src/app/iggy3d/creative/validation/DocumentValidation.*`
- **Exists as:** anchorKindForDescriptor (RoomBake.cpp:253) + the SkipUnsupportedAnchor decision (RoomBake.cpp:440-445) already detect a bad/unsupported anchor at bake time and silently increment skippedUnsupportedAnchorCount. runtimeAnchorSemantic lives on the descriptor (ObjectDescriptor.hpp:187). This EXTRACTS that silent skip into a diagnostic, plus adds the 'undropped anchor' notion (anchor floating above the surface it should sit on).
- **Caller:** validateDocumentPreBake
- **Callees / deps:** anchorKindForDescriptor + descriptorSupportsAnchorBake (RoomBake, EXISTS); describeObject for runtimeAnchorSemantic; optionally checkFloatingObjects' support query for the 'undropped' part
- **Inputs:** const CreativeObject&, const CreativeObjectDescriptor&, span<const CreativeObject> (support for drop)
- **Output / receipt:** optional<CreativeValidationDiagnostic> reasonCode 'creative_anchor_unsupported' (semantic has no bake mapping) or 'creative_anchor_undropped' (spawn/exit floating off the floor), repair = MoveMutation dropping the anchor to the surface
- **Invariants:** Only kinds with runtimeAnchorSemantic != None are candidates; the 'unsupported' rule must match RoomBake's descriptorSupportsAnchorBake exactly (shared predicate) so validation and bake agree; a spawn point IS expected grounded, a light anchor is not.
- **Failure modes:** Drift between this and RoomBake's anchor support set → a spawn passes validation then vanishes at bake; over-aggressive drop moving an intentionally elevated anchor (a rooftop sniper spawn) — candidacy must respect the semantic.
- **Test shape:** A SpawnPoint at y=3 above a floor at y=0 → 'creative_anchor_undropped' + drop repair to y=0; an anchor kind bake does not support → 'creative_anchor_unsupported' matching skippedUnsupportedAnchorCount; a valid grounded spawn → no diagnostic.
- **Why:** Bad/undropped anchors are in scope and directly break the first slice — the thief spawn and guard spawn are anchors; a floating or unmapped spawn means no playable room.
- **Touch-count reduction:** Bake and validator share the anchor-support predicate instead of two copies.

#### `checkDescriptorCoverage` — **PARTIAL** · _validation_ · risk: low  · **wrapper/extraction**
- **Owner:** `src/app/iggy3d/creative/validation/DocumentValidation.* (runtime side) + tests (sentinel side)`
- **Exists as:** A coverage SENTINEL already exists as a TEST: creative_object_descriptor_tests.cpp:142 asserts every non-Unknown descriptor has a non-Unknown shapeKind, and allObjectDescriptors() (ObjectDescriptor.hpp:221) enumerates the table. This overlaps Codex E59 (kind-coverage sentinel) and E60 (category-from-descriptors) in the E-queue. What is missing is a RUNTIME per-object check: 'this placed object's kind resolves to Unknown descriptor facts'.
- **Caller:** validateDocumentPreBake; the static sentinel side is Codex E59's test
- **Callees / deps:** describeObject, categoryOf, shapeKindForObject, occupancyKindForObject (ObjectDescriptor/SpatialProjection, all EXIST)
- **Inputs:** const CreativeObject& (and, for the sentinel, allObjectDescriptors())
- **Output / receipt:** optional<CreativeValidationDiagnostic> reasonCode 'creative_descriptor_incomplete' when an object's kind maps to Unknown category/shape/occupancy or missing required facts; repair is not object-level (the fix is a descriptor table row) so severity 'error', suggestedRepair empty
- **Invariants:** Must not duplicate the static sentinel's job — the static test guards the TABLE, this guards a placed object whose kind is Unknown or stale (e.g. deserialized from an old save with a retired kind); deterministic.
- **Failure modes:** Overlap/duplication with Codex E59/E60 if this re-checks the whole table at runtime instead of just the placed object — keep runtime scope to the object, table scope to the test; a save with a genuinely-removed kind must degrade gracefully, not crash.
- **Test shape:** Object with kind=Unknown → 'creative_descriptor_incomplete'; every real kind → no diagnostic (guaranteed by the static sentinel). Coordinate with E59 so the two do not double-fire in CI.
- **Why:** Missing descriptor facts are in scope and are the root cause of silent bake skips (an Unknown occupancy never projects, an Unknown shape never bakes). Surfacing it turns a silent drop into a named error.
- **Touch-count reduction:** Leans on the existing descriptor table + the E59 sentinel rather than a parallel coverage mechanism.

#### `checkStaleParentIds` — **EXISTS** · _prerequisite_ · risk: none  · **wrapper/extraction**
- **Owner:** `src/app/iggy3d/creative/document/Document.* (EXISTS) — surfaced by validateDocumentPreBake`
- **Exists as:** validateCreativeObjectParentGraph (Document.cpp:298) already fully implements this: it builds an id index, runs validateRestoredParentPayload per object (catches parent ids pointing at nonexistent objects), and parentGraphContainsCycle (catches cycles), returning a reasonCode string ('parent_cycle', etc.). It is ALREADY called on every mutation (DocumentMutation.cpp:115) and on restore (Document.cpp:799).
- **Caller:** validateDocumentPreBake calls the EXISTING validateCreativeObjectParentGraph and lifts its reasonCode string into a CreativeValidationDiagnostic
- **Callees / deps:** validateCreativeObjectParentGraph (EXISTS), validateRestoredParentPayload (EXISTS), parentGraphContainsCycle (EXISTS)
- **Inputs:** span<const CreativeObject> (from CreativeDocument::objects())
- **Output / receipt:** The existing function returns string_view reasonCode; the wrapper maps non-empty → CreativeValidationDiagnostic{reasonCode 'creative_parent_stale'/'parent_cycle', severity error}. Repair = ClearParentMutation on the offending object.
- **Invariants:** Do NOT reimplement — call the existing validator; its guarantees (no self-parent, no dangling id, no cycle) already hold and are pinned by tests; the wrapper only adapts the return type into the receipt.
- **Failure modes:** The existing function returns on the FIRST failure (short-circuits), so the umbrella receipt will surface one parent error at a time, not all — acceptable, but the test must know that; re-running after a repair must re-detect the next one.
- **Test shape:** Object with parentId pointing at a deleted id → validateDocumentPreBake surfaces reasonCode from validateCreativeObjectParentGraph with a ClearParent repair; a two-object cycle → 'parent_cycle'. Assert the umbrella does not re-implement the check (it delegates).
- **Why:** Stale parent ids are in scope but ALREADY SOLVED — honesty demands marking this EXISTS and merely surfacing it, not dressing it as new engine work.
- **Touch-count reduction:** Zero new validation logic — the umbrella reuses the mutation-time validator, so parent integrity has exactly one implementation shared by mutate/restore/pre-bake.

#### `checkPatrolRouteConnectivity` — **NEW** · _deferred_ · risk: high
- **Owner:** `src/app/iggy3d/creative/validation/DocumentValidation.*`
- **Exists as:** Patrol DATA exists only on RUNTIME AiActorState (patrolWaypoints as vector<Vec3>, patrolMode, patrolTargetIndex — AiState.hpp:144-146) and route-finding exists as planRoute (ReasoningRoute). But patrol routes as CREATIVE-DOCUMENT objects (a PatrolRoute/nav-link kind with authored waypoints) are not yet a document representation this can read — the LinkOrRoute profile exists in descriptors but the authored waypoint payload path is the applyPathPointsMutation stub (MutationApply.hpp:130, returns NoChange). So this check is blocked on the authoring representation.
- **Caller:** validateDocumentPreBake
- **Callees / deps:** floodFillReachability OR planRoute (cross-lane AI) to test each consecutive waypoint pair is connected; projectObjectsToGrid for the occupancy the route walks; describeObject to find LinkOrRoute/patrol kinds
- **Inputs:** const CreativeObject& (a patrol-route object once its waypoints are authorable), the occupancy grid
- **Output / receipt:** optional<CreativeValidationDiagnostic> reasonCode 'creative_patrol_disconnected' naming the first waypoint pair with no route; repair advisory (drop a waypoint onto reachable floor), severity error
- **Invariants:** A route with <2 waypoints is trivially connected (not an error); consecutive-pair connectivity uses the SAME reachability primitive as everything else (floodFillReachability) so patrol validity == navmesh validity; deterministic.
- **Failure modes:** Blocked until authored patrol waypoints exist as document data — if built against runtime AiActorState it validates the wrong layer; a Loop patrol must also check last→first, a PingPong must not; false 'disconnected' if it uses graph reachability where the authored grid has a valid but ungraphed path.
- **Test shape:** Patrol object with waypoints on two floor islands with no connection → 'creative_patrol_disconnected' naming the gap pair; waypoints all on one connected floor → no diagnostic; a Loop whose close-the-loop leg is blocked → diagnostic. (Test gated on the authored-waypoint representation landing.)
- **Why:** Disconnected patrol routes are in scope and directly break the slice's guard — a guard whose patrol has an unreachable leg stalls. But it is honestly NEW and partly blocked on the authoring data model.
- **Touch-count reduction:** Once floodFillReachability exists, patrol connectivity is a thin consumer of it, not its own pathfinder.

#### `makeValidationDiagnostic / CreativeValidationDiagnostic (receipt type + helper)` — **NEW** · _slice-critical_ · risk: low
- **Owner:** `src/app/iggy3d/creative/validation/DocumentValidation.*`
- **Exists as:** The SHAPE exists twice as precedent to copy, not reuse: AsciiRoomDiagnostic{severity, reasonCode, message} (AsciiRoomSource.hpp:11) and CreativeMutationApplyReceipt{status, message, dirtyFlags,...} (MutationApply.hpp:47). Neither carries a suggestedRepair mutation, which is the doctrine-critical new field.
- **Caller:** every check* function above; validateDocumentPreBake aggregates them
- **Callees / deps:** CreativeMutationRequest (Mutation.hpp) to type the suggestedRepair; toString helpers for stable reasonCodes
- **Inputs:** severity, reasonCode string, CreativeObjectId, message, optional CreativeMutationRequest suggestedRepair
- **Output / receipt:** CreativeValidationDiagnostic value; and applyRepairs(document, receipt) that runs each suggestedRepair through the EXISTING applyDocumentMutation and returns a follow-up receipt
- **Invariants:** reasonCode strings are stable, namespaced 'creative_*', and enumerated in one place (mirror AsciiRoom's reasonCode discipline); a diagnostic with repairable==true MUST carry a valid suggestedRepair that, when applied, clears that same reasonCode on re-validation — this is the validate→REPAIR→receipt contract made testable.
- **Failure modes:** A repair that does not actually clear its own diagnostic (validate→repair→re-validate must converge); applying repairs in the wrong order (drop-to-floor before overlap-resolve) fighting each other — applyRepairs must re-validate between passes or document ordering; a repair mutation rejected by locks (LockedObject) must be reported, not silently dropped.
- **Test shape:** Round-trip: validate a defective doc → apply its suggestedRepairs via applyDocumentMutation → re-validate → assert ok==true (the repair closed the loop). Assert a locked object's repair returns a LockedObject-flavored follow-up diagnostic rather than pretending success.
- **Why:** This is the load-bearing doctrine object: without a suggestedRepair field the receipt is diagnostic-only, not validate→REPAIR→receipt. It is the one genuinely new type the lane must define.
- **Touch-count reduction:** One diagnostic/repair type for ALL checks instead of each check inventing its own result shape (the mistake AsciiRoom+MutationApply avoided within their own subsystems).


### Lane 5 — Scale / deferred (spatial index, large-scene query acceleration)

> **Existing surface:** Roughly 60% of the substrate this lane would ever need already exists, and it is currently correct-enough — this lane is almost entirely DEFER, not greenfield build. Grounded facts: (1) id→object lookup is ALREADY O(1): CreativeDocument::findObject uses `objectIndex_` (std::unordered_map<CreativeObjectId,std::size_t>, Document.hpp:233), so the naive "linear scan to find an object by id" problem this lane might imagine does not exist. (2) A full uniform-grid voxelization substrate EXISTS: SpatialProjection.* (worldBoundsToGridBounds/toGridIndex/clampGridBounds/projectObjectsToGrid, SpatialProjection.hpp:82-125) already turns a span of objects + CreativeBounds into occupied grid cells with a validated receipt. This is the exact math the roadmap's CreativeObjectAABBIndex would reuse (cell↔object), just in the object→cell direction. (3) The ONLY current O(n) full scans are two, both un-guarded by object count: the per-click pick candidate gather (apps/iggy3d_creative/main.cpp:1210, `for obj in document().objects()` building projectBoxToScreen candidates) and projectObjectsToGrid over the WHOLE document every pick frame (bridge/ViewportPickFrame.cpp:84-93). (4) There is NO ray/overlap/frustum query surface anywhere in creative/ — grep for overlap|intersect|frustum|queryRay in creative/ returns nothing; the only "broadphase" symbol in the tree is an unrelated physics render-debug marker (view/RenderBridge). (5) CreativeObjectAABBIndex is explicitly SPECULATIVE/UNCARDED per the algorithm map (docs/creative_mode/creative_editor_algorithms_map_v0_1.md:29, and E53 filed no card). So: the index does NOT exist, but its substrate (grid math + O(1) id map + dirty-flag receipts as the maintenance signal) all exists. Everything below is deferred until a measured object-count trigger.


> **Lane summary:** The ACTUAL new work in this lane is ZERO right now, and the honest recommendation is to build nothing until a measured trigger fires. The scenes are small (a room-bake fixture is on the order of tens of objects; the first playable slice is one authored room), and both O(n) paths (pick gather at main.cpp:1210, whole-document voxelization at ViewportPickFrame.cpp:93) are correct-enough at that scale — an O(n) scan over ~50 boxes projecting AABBs is sub-millisecond and is also the correctness ORACLE the map insists any index must match before the scan is deleted (algorithm map line 86). When a real trigger fires, the new work is a single persistent CreativeObjectAABBIndex: a coarse (~8 m) hash grid keyed by objectId+CreativeBounds, built as a THIN reuse of existing SpatialProjection grid math (worldBoundsToGridBounds/toGridIndex), maintained incrementally off the Transform|Bounds dirtyFlags every mutation receipt already emits, and rebuilt wholesale (rebuildFrom) on undo/redo snapshot boundaries. It serves four otherwise-separate queries from one structure — ray-pick candidate gather, overlap-at-placement validation, frustum cull, snap-neighborhood. queryOverlap is the one that is also slice-adjacent (placement validation could want it before big scenes), but even it is O(n)-fine today. TRIGGERS (make them concrete, assert them in a bench test, don't guess): build the index when a single scene routinely exceeds ~500 objects OR a pick/cull frame measurably exceeds ~2 ms in the standalone capture loop, whichever first; the greedy-mesh bake lane (map step 12) may pull queryOverlap earlier for hidden-face culling on big rooms. Until then: keep the full scan, add only a cheap object-count instrument so the trigger is data, not vibes. Risk of building early = a second source of spatial truth to keep in sync with the receipt stream for no measured payoff, which violates the leverage-in-readers/one-substrate doctrine.

#### `CreativeDocument::findObject` — **EXISTS** · _prerequisite_ · risk: none
- **Owner:** `src/app/iggy3d/creative/document/Document.*`
- **Exists as:** CreativeDocument::findObject(CreativeObjectId), Document.cpp:471 (const) / :480 (mutable); backed by objectIndex_ std::unordered_map<CreativeObjectId,std::size_t>, Document.hpp:233
- **Caller:** Facade::findObject (Facade.cpp:896), every mutation path (createDocumentObject/removeDocumentObject etc.), main.cpp inspector/undo logging (~30 call sites)
- **Callees / deps:** objectIndex_.find; objects_[slot]
- **Inputs:** CreativeObjectId
- **Output / receipt:** const CreativeObject* / CreativeObject* (nullptr if absent)
- **Invariants:** objectIndex_ stays in lockstep with objects_ (removeDocumentObject re-indexes trailing slots, Document.cpp:660-663); id→slot is O(1) amortized
- **Failure modes:** stale slot after erase if re-index skipped (guarded by the loop at :660); returns nullptr for unknown/deleted id — callers already null-check
- **Test shape:** existing document tests: create N, findObject each id, assert pointer identity + null for bogus id; add: erase middle object, assert all survivors still findable at correct data
- **Why:** Proves the 'find object by id' path is ALREADY O(1) — this lane must NOT build an id index; it exists
- **Touch-count reduction:** n/a

#### `projectObjectsToGrid` — **EXISTS** · _deferred_ · risk: none
- **Owner:** `src/app/iggy3d/creative/spatial/SpatialProjection.*`
- **Exists as:** projectObjectsToGrid(std::span<const CreativeObject>, const CreativeSpatialProjectionRequest&), SpatialProjection.hpp:123 / .cpp; called at bridge/ViewportPickFrame.cpp:93 over facade->document().objects()
- **Caller:** ViewportPickFrame (bridge/ViewportPickFrame.cpp:84-93) per pick frame; creative_spatial_projection_tests.cpp
- **Callees / deps:** projectObjectToGrid → per-shape projectBox/Volume/Line/Path/Point/LinkObjectToGrid; worldBoundsToGridBounds; toGridIndex; clampGridBounds
- **Inputs:** span of all objects + grid size/cellSize/clamp flags
- **Output / receipt:** CreativeSpatialProjectionReceipt (status + occupied cells + projectedBounds + message)
- **Invariants:** validate→receipt: InvalidGrid/EmptyProjection/OutOfBounds statuses returned, never silent; object→cell direction
- **Failure modes:** O(n) over the WHOLE document every pick frame with no object-count guard — the real current bottleneck when scenes grow; produces correct cells regardless
- **Test shape:** existing projection tests assert cell sets per shape; add a bench that times projection at 50/500/5000 objects to locate the measured trigger
- **Why:** This IS the reusable grid substrate the AABB index would sit on; it is also the un-guarded scan that first pressures this lane
- **Touch-count reduction:** n/a

#### `pick candidate gather (inline scan)` — **EXISTS** · _deferred_ · risk: none
- **Owner:** `apps/iggy3d_creative/main.cpp`
- **Exists as:** the `for (const CreativeObject& obj : appState.facade.document().objects())` loop building objectPickCandidates via projectBoxToScreen, main.cpp:1210-1222; pickClosest lives app-locally per E42
- **Caller:** the standalone frame loop on click/hover
- **Callees / deps:** visualBoundsForObject; projectBoxToScreen; (down-stream) pickClosest ray-vs-AABB
- **Inputs:** document().objects(), camera.clipFromWorld, viewport extent
- **Output / receipt:** std::vector<ObjectVisualPickBounds> then nearest-hit TargetRef
- **Invariants:** scans only visible objects; nearest ray-entry wins (E42 slab pick); this scan is the correctness ORACLE any index must match (map line 86)
- **Failure modes:** O(n) per click; fine at tens/hundreds, measurable past ~500; no functional bug — purely perf
- **Test shape:** a determinism test already implied by --capture; add: assert pickClosest result over full scan == result over index (when index exists) on a shared fixture
- **Why:** The second un-guarded O(n) path; the thing queryRay would accelerate — kept as oracle until then
- **Touch-count reduction:** n/a

#### `CreativeObjectAABBIndex (type + rebuildFrom)` — **DEFER** · _deferred_ · risk: medium  · **wrapper/extraction**
- **Owner:** `src/app/iggy3d/creative/spatial/ (new: CreativeObjectAABBIndex.* — does not exist)`
- **Exists as:** —
- **Caller:** ViewportPickFrame / pick gather / placement validation — only once a trigger fires; today nobody
- **Callees / deps:** REUSES worldBoundsToGridBounds/toGridIndex/clampGridBounds (SpatialProjection.hpp:82-94); keyed by objectId+CreativeBounds; coarse ~8 m buckets (NOT 1 m occupancy res, per map line 84)
- **Inputs:** rebuildFrom(std::span<const CreativeObject>)
- **Output / receipt:** populated multimap cell→objectId; a build receipt (bucketCount/objectCount/cellSize)
- **Invariants:** MUST reproduce the O(n) scan's query results exactly (oracle-checked before scan deletion); one bucket size, not per-kind; only ONE spatial truth — derived from document, never authoritative
- **Failure modes:** drift from document if maintenance is missed → build wholesale on snapshot boundaries to self-heal; 8 m bucket avoids multimap bloat a 75×45 floor would cause at 1 m
- **Test shape:** build over a fixture, assert queryOverlap/queryRay set == brute-force scan set for random rays/boxes; assert rebuildFrom is idempotent
- **Why:** The single structure serving 4 queries — but speculative/uncarded (map:29, E53 filed no card); build ONLY on measured trigger (>~500 objs or >~2 ms pick/cull frame)
- **Touch-count reduction:** collapses pick-gather + overlap + cull + snap-neighborhood into one query surface instead of a bespoke scan per feature

#### `CreativeObjectAABBIndex::insert / remove / update (incremental maintenance)` — **DEFER** · _deferred_ · risk: medium  · **wrapper/extraction**
- **Owner:** `src/app/iggy3d/creative/spatial/ (new, does not exist)`
- **Exists as:** —
- **Caller:** the mutation receipt consumer — whatever installs CreativeDocumentMutationReceipt after applyDocumentMutation (Facade); driven by dirtyFlags
- **Callees / deps:** worldBoundsToGridBounds; the receipt's dirtyFlags (Transform|Bounds) as the change signal (map line 159-160); objectId key
- **Inputs:** objectId + old bounds + new bounds (from receipt), or the changed object
- **Output / receipt:** void + updated buckets
- **Invariants:** maintenance keyed off the SAME dirtyFlags that already drive undo change-detection and snap-cache invalidation (one receipt, three consumers); insert on create, remove on delete, re-bucket only when Transform|Bounds set
- **Failure modes:** missed flag → stale bucket (mitigated: wholesale rebuildFrom on undo/redo boundary resets truth); moving across bucket boundary must remove-then-insert, not just insert
- **Test shape:** apply create/move/resize/delete mutations, after each assert index query set == fresh rebuildFrom set (incremental == wholesale)
- **Why:** Keeps the index cheap (no full rebuild per edit) by riding the receipt stream that already exists — but same speculative status; deferred with the index
- **Touch-count reduction:** one dirty-flag consumer maintains the index for ALL future editing verbs; no per-tool index code

#### `CreativeObjectAABBIndex::queryOverlap` — **NEW** · _deferred_ · risk: medium
- **Owner:** `src/app/iggy3d/creative/spatial/ (new, does not exist)`
- **Exists as:** —
- **Caller:** placement/overlap-at-placement validation (slice-adjacent: create/move commit could reject or warn on overlap); greedy-mesh bake hidden-face culling (map step 12)
- **Callees / deps:** worldBoundsToGridBounds(queryBounds); bucket lookup; per-candidate CreativeBounds AABB-vs-AABB refine
- **Inputs:** CreativeBounds query box (+ optional exclude objectId)
- **Output / receipt:** list of overlapping CreativeObjectId (broadphase candidates, then exact-refined)
- **Invariants:** returns a SUPERSET from buckets then exact-tests each candidate (no false negatives); excludes the moving object itself; validate→receipt if wired to placement
- **Failure modes:** no overlap query exists in creative/ TODAY (grep-confirmed) — placement does not validate overlap yet; O(n) brute-force is the correct-enough stand-in until scenes are large
- **Test shape:** two-box fixture: overlapping pair returns each other, disjoint pair returns empty; assert index result == O(n) brute-force overlap on random boxes
- **Why:** The one query with a near-term slice pull (placement validation, bake culling); genuinely absent — but still DEFER the accelerated form; do brute-force O(n) overlap first if a slice needs it before the trigger
- **Touch-count reduction:** placement validation + bake culling share one query instead of two bespoke scans

#### `CreativeObjectAABBIndex::queryRay` — **DEFER** · _deferred_ · risk: medium
- **Owner:** `src/app/iggy3d/creative/spatial/ (new, does not exist)`
- **Exists as:** partially fronted by app-local pickClosest ray-vs-AABB (E42, main.cpp) which does the RAY MATH but with no broadphase — it scans all candidates
- **Caller:** pick candidate gather (main.cpp:1210) once it needs to stop scanning all objects
- **Callees / deps:** DDA/ray-march over coarse buckets reusing grid coord math; then exact ray-vs-AABB (already in pickClosest) per candidate
- **Inputs:** world-space ray origin+dir (from clipFromWorld inverse, E42)
- **Output / receipt:** ordered candidate objectIds along the ray (nearest-first) for exact slab test
- **Invariants:** MUST return the same nearest hit as the full-scan slab pick (oracle, map line 86); only accelerates candidate gather, does NOT replace the exact ray-vs-AABB test
- **Failure modes:** ray-march bucket traversal bugs → missed candidate → wrong pick; guarded by oracle equivalence test; today the full scan is correct and fast enough
- **Test shape:** assert queryRay+exact == full-scan pickClosest for a battery of rays on the standalone fixture; the --capture pick determinism harness is the vehicle
- **Why:** Accelerates the pick scan — but the exact ray test already exists (E42) and the scan is the oracle; pure perf, deferred until measured pick-frame pressure
- **Touch-count reduction:** reuses the existing exact ray-vs-AABB; only adds broadphase — not a new pick algorithm

#### `CreativeObjectAABBIndex::queryFrustum` — **DEFER** · _deferred_ · risk: medium
- **Owner:** `src/app/iggy3d/creative/spatial/ (new, does not exist)`
- **Exists as:** —
- **Caller:** per-frame render/draw-list cull — NOT wired today; the draw path currently iterates all visible objects (main.cpp:2494 document().objects() loop)
- **Callees / deps:** 6-plane frustum from camera.clipFromWorld; bucket-vs-plane reject; per-candidate CreativeBounds-vs-frustum refine
- **Inputs:** 6 frustum planes (extracted from clipFromWorld)
- **Output / receipt:** visible CreativeObjectId set for the frame
- **Invariants:** conservative — never culls a visible object (plane test on bucket AABB is a superset); receipt optional (culledCount/visibleCount for debug)
- **Failure modes:** no frustum cull exists today; drawing every object is correct and fine for one room; over-aggressive plane test would pop objects — mitigated by conservative bucket test
- **Test shape:** place objects inside/outside a known frustum, assert in-set == brute-force frustum test; assert no visible object ever omitted
- **Why:** The 4th query the one index serves — purely a large-scene render optimization; the current draw-all path is correct; deepest DEFER of the four (no slice pulls it)
- **Touch-count reduction:** same index serves cull as serves pick/overlap — no separate cull structure

#### `object-count / query-time instrument (trigger sensor)` — **PARTIAL** · _validation_ · risk: low  · **wrapper/extraction**
- **Owner:** `src/app/iggy3d/creative/spatial/ or bridge/ViewportPickFrame.* (thin, near-absent)`
- **Exists as:** objectCount already surfaced on the room-bake receipt (adapters/RoomBake.cpp:555 via document.objectCount()); no pick/cull frame timer yet
- **Caller:** the standalone --capture loop / a bench test; a dev overlay
- **Callees / deps:** document().objectCount(); a std::chrono span around the pick gather + projectObjectsToGrid
- **Inputs:** the live document + the per-frame pick/projection call
- **Output / receipt:** objectCount + last pick-frame microseconds (logged / asserted)
- **Invariants:** read-only, must not perturb behavior; exists so the DEFER triggers are DATA not guesses
- **Failure modes:** none functional; if absent, the whole lane's 'build when >~500 objs or >~2 ms' trigger is unmeasurable and someone builds the index on vibes
- **Test shape:** a bench test that grows the document to 50/500/5000 objects and records pick-frame time, printing where it crosses ~2 ms — this test IS the trigger authority
- **Why:** The single highest-value thing to add NOW: it converts every DEFER above into a measured decision and prevents premature index work
- **Touch-count reduction:** one sensor gates all four deferred queries — build none of them until it fires

