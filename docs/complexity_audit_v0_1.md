<!-- Generated 2026-07-07 by a 12-agent complexity audit (10 buckets, adversarially verified, spot-checked by hand: the clamber_candidate validator drift is confirmed real — EditableRoomDocument.cpp:48 accepts it, RoomAsset.cpp:275 does not). Cite finding #+bucket when scoping a card; version-bump on corrections. -->

# iggy3d Complexity Audit — v0.1

*Checked in at `docs/complexity_audit_v0_1.md`. Spot-verified against the tree on `iggy3d-main`.*

**Complexity, working definition (the user's):** *the number of places a developer must understand, edit, and verify to safely change one behavior.* LOC is **not** the metric — a focused 300-line table-driven file with one owner is simple; a 40-line helper called from five lifecycle paths with hidden state rules is complex.

**Read this to scope cards, not as a laundry list.** Every finding was call-graph-verified. Findings the audit *credits* (already at target) are at the end — do not re-flag them.

---

## 1. Overall headline

**Complexity in iggy3d is not spread evenly — it clusters in two seams, and everything else is either already clean or a mechanical mop-up.**

1. **The room traversal-tag vocabulary has no owner.** The decision "which tags are valid + what they mean" (`walkable / blocker / clamber / vault / wire_walk / …`) is duplicated across **13 non-test files (~100 literal occurrences)**, two independent room bakers, two validators **that have already drifted** (`clamber_candidate` is accepted by `EditableRoomDocument.cpp:45` and rejected by `RoomAsset.cpp:273`), and three separate tag→slot parse tables. This is the one **high-severity** finding and the single most leveraged card in the repo.

2. **The product-app `activeRoom → activeRoomCollision` derived-truth pair is kept in sync by hand-discipline, not by code.** Collision is baked at **15 call sites** from a **12-writer** input, with **no revision/dirty guard** — every mutation site must remember to rebake before the next session tick reads it. It surfaces in three buckets at once (hidden state, temporal coupling, feature touch-count).

Below those two, the pattern is healthy: geometry math is core-owned and credited, the CreativeObjectDescriptor is a clean single-owner table, the receipt split delivered its low touch-count, and AppKernel/Facade earn their keep. The rest are **mechanical, behavior-preserving mop-ups** — enum switches that shadow existing tables, diverged NaN guards in copy-pasted snap math, a color literal drifted across three render files, and include-hygiene dragging a 62-include god-struct header into ~120 TUs.

**One-line verdict:** *Two seams need an owner; the rest needs a janitor.*

---

## 2. Per-bucket verdicts

| Bucket | Severity | One-line finding |
|---|---|---|
| 1. Feature-Add Touch Count | **med** | Two of four concepts add cleanly (receipt field, movement verb); the two enum-heavy ones carry avoidable per-kind duplication — a `toString`/`allowedMutations` switch shadowing the descriptor, and a hand-written deserialize half in the AI save path. |
| 2. Ownership Ambiguity | **HIGH** | Creative descriptor lane is a clean single-owner win, but the room traversal-tag vocabulary has **no authoritative owner**: duplicated across 13 files + two bakers, and the **two validator copies have already drifted**. |
| 3. Duplicate Algorithms | **med** | Most geometry is core-owned and credited; **three real duplicates remain, two with DIVERGED NaN/inf/overflow guards** — latent-bug carriers, not mere copy-paste. |
| 4. Branch Ladders & Kind Tables | **med** | One offender: `ProductPrimitiveDrawKind` (22 values) exhaustively switched in 3 render files with per-kind color **already drifted**. `FrontendAction`/`ProductRoomEditorTool` are benign compiler-guarded ladders. |
| 5. Hidden State Mutation | **med** | Document/receipts/session-dirty all cleanly funneled; the one offender is the `activeRoom`/`activeRoomCollision` pair, rebuilt by hand-repeated call pairs across 6 files with no scheduler. |
| 6. Temporal Coupling | **med** | Creative revision→bake is *not* coupled (stateless — credit). The real defect is product-app `activeRoomCollision`: 15 bake sites, 12 writers, no dirty guard; a mid-tick rebake branch in `Controller.cpp` makes "which state a reader sees" depend on handler ordering. |
| 7. File Bloat w/ MIXED Responsibilities | **med** | Only 3 of 11 largest files genuinely mix domains. Worst is `OpeningMenuView.cpp` — a misnamed **1653-line** SDL catch-all rendering the menu **plus 6 unrelated debug/HUD/viewport domains**. Receipt split + creative `main.cpp` extraction credited. |
| 8. Test Fragility | **low** | `tests/unit` is contract-oriented and low-fragility; the real cost is **missing shared test infrastructure** (`expect()`/`near()`/fixtures copy-pasted across 214+ of 233 files), not brittle pins. |
| 9. API Surface & Wiring Cost | **med** | God-struct *usage* is well-managed; the one systematic cost is **include hygiene**: 13 headers hard-include the 62-include `ProductAppWindowState.hpp` when a forward decl suffices. |
| 10. Abstraction Debt | **med** | Abstractions are mostly load-bearing; the one real debt is the **stalled `CreativeActiveIdentity` migration** running a 12-field parallel mirror into the god-struct instead of replacing it. |

---

## 3. Prioritised remediation roadmap

Ranked by **complexity-reduction-per-effort** = `(touch-points × change-frequency) / effort`. Cut top-down. **Codex** = creative/render/ascii lane, **claude** = gameplay/AI/movement lane, **shared** = seam card needing a brokered contract.

| # | Fix | Bucket | Lane owner | Effort | Card | Why it ranks here |
|---|---|---|---|---|---|---|
| **1** | Route `Object.cpp:168 toString()` → `describeObject(kind).displayName`; make `Mutation.cpp:551 allowedMutations()` a pure filter over `descriptorAllowsMutation()`. Deletes a 108-case + a 102-case switch. | 1 | creative | **S–M** | **Codex** | Kills two exhaustive per-kind switches at once, removes the `AGENTS.md` hand-verify step, drops object-kind add from 4 sites to 2. Highest reduction/effort. |
| **2** | Give `AiBehaviorKind` (+ the ~20 other `enumText`/`parseEnum` pairs) one `name↔value` table via `IGGY3D_ENUM_TABLE` so `parseEnum` derives from the map `enumText` already uses. | 1 | runtime/sim | **M** | **claude** | Fixes a real round-trip bug class (name `parseEnum` lacks → silent `None`) for **every runtime enum at once**; 42 functions collapse. |
| **3** | Introduce one authoritative **`TraversalTag` catalog** (shared enum + string table at the bake/movement seam): closed valid set + tag↔string + single `validSemanticTag`. Replace the two drifted validators, `RoomBake.cpp` literal emission, and the three `MovementTraversal*` parse tables. Bakers keep their own occupancy→tag *policy* but reference catalog constants. | 2 | **shared** | **M** | **shared** (broker) | **The #1 leverage card.** Collapses a 13-file/~100-literal web into one catalog: a new affordance = 1-file change. Fixes the live `clamber_candidate` drift as a side effect. M only because it crosses lanes. |
| **4** | Give `activeRoomCollision` one owner keyed on a revision token: add `activeRoomRevision`, cache baked-at revision, replace the 15 rebake calls + 12 writer obligations with one `ensureActiveRoomCollisionFresh(...)` called once before the tick (`InputFrame.cpp:525`). Delete the mid-tick branch (`Controller.cpp:2235-2241`). | 5+6 | product-app | **M** | **claude** | Second-highest leverage: one fix retires findings in **three buckets**. Turns "remember 2 coupled writes at the right fidelity" into one guard + one refresh. |
| **5** | Extract a `constexpr` metadata table keyed by `ProductPrimitiveDrawKind` (base color / markerSize / layer). Replace `colorForRoomKind` + the `RenderBridge` counting switch with lookups; have `OpeningMenuView` read the shared color. Keep the two genuine render-dispatch switches. | 4 | render | **M** | **Codex** | Fixes the **confirmed** `ElevatedFloorTile` color drift (`{92,126,102}` vs `{137,168,143}`); stops a draw-kind add touching 3 files. Mirrors the proven UI-command-kind metadata pattern. |
| **6** | Finish the `CreativeActiveIdentity` migration: repoint the 3 readers at `creativeApp.identity`; delete `window.activeCreative`, `mirrorProductActiveCreativeIdentity` (`Operations.cpp:30`) + its 5 call sites; collapse callers onto `identity.worldActive()`. | 10 | product-app | **M** | **claude** | Removes a live 12-field double-write and a 7-vs-1 split reader census. Stalled migration → single truth. |
| **7** | De-dupe the three snap/grid math copies with diverged guards: make `spatial/Snap::snapScalar` + `DocumentSnap::snapCreativeDocumentScalar` delegate to one checked impl (at minimum add the `validStep`+non-finite guard); extend core `GridFootprint` with a guarded 3D variant and have `SpatialProjection::worldToGridCoord` delegate. | 3 | creative (+core) | **S–M** | **Codex** | Retires two latent NaN/inf/overflow carriers. Small per-fix, high safety; tests already pin both wrappers. |
| **8** | Split `OpeningMenuView.cpp` (1653 LOC) by domain into `MenuPanelsView`, `DebugHudView`, `RoomEditorOverlayView`, `ScenePrimitiveView` + shared `SdlDraw.hpp`; keep `drawOpeningMenuView` as thin entry. | 7 | render | **L** | **Codex** | Biggest mixed-responsibility file (6 domains behind one name) but pure churn; high effort, lower urgency than ownership cards. |
| **9** | Extract `Operations.cpp` (1734 LOC) along its 4 seams into `SaveSlotOperations`, `CreativeWorldLaunch`, `CreativeIdentityMirror`; leave session bootstrap in place. **Do #6 first** — it deletes the mirror content this moves. | 7 | product-app | **M** | **shared** | Second mixed-domain file; sequenced after #6 so the mirror extraction shrinks. |
| **10** | Move the ~1100-LOC CPU mesh-gen block out of `BufferImageResources.cpp` (1553 LOC) into `RoomMeshBuilder.cpp/.hpp` (zero Vulkan dep); leave the ~300-line GPU class. | 7 | render | **M** | **Codex** | Clean CPU/GPU seam, no behavior change. |
| **11** | Forward-declare `ProductAppWindowState` in the 13 headers that use it only by ref/ptr; move the real include into the `.cpp`. Start with `ReceiptBuilder.hpp` (23 includers) + `RendererLifecycle.hpp` (18). | 9 | product-app | **S** | **claude** | Mechanical, behavior-preserving; stops the 62-include definition re-parsing in ~40 TUs. Cheap build win. |
| **12** | Add `tests/support/TestMain.hpp` + `Fixtures.hpp` (`expect`/`near`/`nearlyEqual` + builders + `IGGY_TEST(fn)` auto-registration, killing the silent-drop hazard). Migrate opportunistically — **do NOT bulk-rewrite 214 green files.** | 8 | shared | **M** | **shared** | Lowest urgency (suite green) but removes real boilerplate + the silent-drop hazard going forward. |

**Sequencing:** #1, #7, #11 are near-free — land immediately. #3 and #4 are the two structural wins and the *scoped* cards. #6 must precede #9. #8/#10 are pure-churn L cards — schedule when a render-lane worker is idle.

---

## 4. Flagship touch-count table

*"To add one X today, a developer edits N files; the target is M."*

| Concept | Files touched today | Target |
|---|---|---|
| **Movement affordance / traversal tag** | **~13 files** (2 bakers, 2 drifted validators, ASCII pipeline ×3, 3 movement parse tables, collision, debug) | **1** — a row in the `TraversalTag` catalog |
| **Creative object kind** | **4 hand-edited** (enum + descriptor row + `toString` switch + `allowedMutations` switch) | **2** — enum line + descriptor-table row |
| **AI behaviour rung** | **~5–6** (enum, name switch, `parseEnum` switch, `alertBehaviorForLevel`, decision logic) | **3** — enum + name-table row + genuine per-rung semantics |
| **`activeRoom` state change** | **~2 coupled writes + a rebake obligation** per site, ×12 writers / 15 rebake sites, no guard | **1** — `setActiveRoom(...)` + one `ensureActiveRoomCollisionFresh` guard |
| **Render draw kind** | **3 files** (`PrimitiveDrawList`, `OpeningMenuView`, `RenderBridge`), color literals drifted | **1** — a row in the draw-kind metadata table |
| **Receipt field** *(at target — credit)* | **1** — one `appendReceiptField` in the owning appender | **1** ✓ |
| **Movement traversal verb** *(at target — credit)* | **~3–4, all inside `src/runtime/movement/`**, no cross-lane touch | **~3** ✓ |

---

## 5. What already earns its keep — do NOT touch

- **CreativeObjectDescriptor** — sole authority for per-kind facts; `RoomBake`/`SpatialProjection`/standalone all derive via `describeObject(kind)`. The shape the traversal-tag catalog (#3) should imitate.
- **Receipt appender split** — 17 domain files; `ReceiptBuilder.cpp` ~85 lines. Adding a domain is a 1-file change. **Do not re-flag as bloat.**
- **CreativeDocument mutation funnel** — private state, all writes in `MutationApply.cpp`, single revision bridge. Non-const `findObject` is read-only.
- **Session dirty/hash** — funnels through `markDirtyAndHash` (`Session.cpp:481`). The model the `activeRoom`/collision pair (#4) should copy.
- **Core geometry math** — ray-AABB, OBB, frustum, Euler, mat4 `projectPoint`, `GridFootprint` all core-owned. The three duplicates in #7 are the exceptions.
- **`AppKernel`** — load-bearing: owns 7 app-lifetime members + the real `run` orchestration; removes argument-threading coupling.
- **`creative::Facade`** — real primitive bounding the creative state machine behind one typed-receipt surface; drives undo via its install path.
- **`ProductCreativeUiCommandKind`** — already table-driven; the model for the draw-kind fix (#5).
- **creative `main.cpp` 12-file extraction** — domain logic already in `Standalone*`/`CreativeRenderer*` siblings; a big orchestrator, not domain-mixing. **Do not re-run the split.**
- **`tests/unit` contract orientation** — low-fragility; the gap is shared *infrastructure* (#12), not brittle tests.

---

*Cut cross-lane slices from this map. Cite the finding # + bucket when scoping a card; version-bump on corrections.*
