<!-- Generated 2026-07-07 by a 6-agent decomposition workflow (enumerate -> cluster -> adversarial critique); v1 folds the verified critique fixes. The destination for every ownership-consolidation slice. -->

## 0. Corrections applied (post-critique v1)

Adversarial critique returned **NEEDS_FIXES**; these were verified against disk and folded in:

- **Coverage** — two dropped members restored to **CreativeAuthoringStore #4**: `asciiRoomActivation` (235) and
  `creativeBakedRoomAutoRefresh` (367). Coverage is now enforced by rule, but the real gate is a
  **member-enumeration test** (assert every struct member is claimed by exactly one Store / remainder / delete-set)
  — not a hand count. Build that test before asserting DONE.
- **Second nested duplicate** — `roomEditing` holds a nested `activeRoomCollision` (`EditingState.hpp:20`) *and*
  `activeRoom` (`:19`). **RoomStore #1 must sever BOTH**, and the in-flight collision slice's **G5** scope widens to
  kill the nested collision copy too, else it lands in #4 as a divergent copy of the canonical truth.
- **Naming** — `InputOwnershipStore` → **`InputDeviceStore`** (its one "ownership" field, `inputOwner`, is a DELETE, so
  the store's real domain is input-device + capture-surface state).
- **Fire order** — only ranks **1–3** are a committed sequence (the live deficit slices). Ranks **4–11 are an
  UNORDERED "structural, unscheduled" bucket** (move-off-god-struct, no freshness debt, no readiness signal) — do
  not read 4<5<… as an order.
- **Provisional stores** — **DebugHudStore #9** and **PresentPathStore #11** are now resolved as real stores;
  **FrontendWindowShell #10** is the
  residual junk-drawer holding pen. Resolve delete-vs-store for each **before** standing it up.

---

# God-Struct Decomposition Target Map

**Target:** `ProductAppWindowState` — `/Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp`, struct body **lines 193–425** (~154 single-line member declarations; the exact count varies with multi-line `std::string` decls — a member-enumeration TEST, not a hand count, is the coverage gate; see §0).
**Composer:** `AppKernel` — `/Users/kogaryu/iggy3d/src/app/iggy3d/AppKernel.hpp` (today owns `window` + `activeSession` + `creativeApp` + `frontend` + `saves` + `settings` + `worldSetupDraft`).
**Doctrine:** target-first / directed emergence — every ownership-deficit slice is a **directed landing** onto a Store named here, validated against this destination, not an ad-hoc patch. **Naming law: Store, not Kernel.**

---

## 1. Thesis

`ProductAppWindowState` is a ~176-member god-struct because it is the junk drawer where every subsystem parked state that had no owner. The fix is not to patch scatter site by site — it is to **name the destination first**, then let each slice land toward it.

The destination: **`ProductAppWindowState` becomes a thin composition of owned domain Stores that `AppKernel` holds**, plus a ~4-field app-global remainder of genuine window/SDL lifecycle bits. Each Store owns exactly one domain's truth. **Three** Stores carry **derived** state and therefore a **freshness token** (the `activeRoomCollision` exemplar discipline — stamp `{revision, sessionHash}`, rebuild on demand iff drifted); the rest are plain owned state or write-once-per-frame telemetry that just needs a home.

Two forces shape the map:

- **The ownership-deficit audit** (`docs/ownership_deficit_audit.md`, Gate-1 ratified 2026-07-07) already ranks the first three landings — `activeRoom`→RoomStore (score 9.0), `activeCreative`→delete (4.0), `creativeFly`→CreativeFlyAnchorStore (3.0) — plus two pure **DELETEs** (`inputOwner` + `gameplayInputSuppressed`; `runtimeStateHash`). Those are removed, not rehomed.
- **The 10 receipt appenders** (`src/app/iggy3d/receipt/*Fields.cpp`) mirror domain boundaries — a **hint, not a law**. At 6 seams the receipt-emission domain differs from the state-ownership domain. **Do not fork a Store along a receipt seam.**

---

## 2. The domain Stores

11 Stores + an app-global remainder. Fire-order rank = leverage × readiness (audit score where one exists; in-flight beats greenfield). Line numbers are the on-disk declarations.

| # | Store | Owns (members) | Derived? / Freshness token? | Receipt appender it mirrors | Landing slice (deficit) |
|---|-------|----------------|:---:|---|---|
| **1** | **RoomStore** *(= the exemplar `ActiveRoomCollisionFreshnessStore`)* | `activeRoom` (254), `activeRoomRevision` (256), `activeRoomCollision` (257) — **absorbing** BOTH nested duplicates `roomEditing.activeRoom` (`EditingState.hpp:19`) and `roomEditing.activeRoomCollision` (`EditingState.hpp:20`) — sever both in the collision slice's G5. One owner of `{activeRoom, activeRoomCollision, revision}`. | **YES / YES** — `activeRoomCollision` is a `SpatialSurfaceSet` derived from `activeRoom.room` + live doors. Token = `{roomRevision, sessionHash}`. | `ActiveRoomFields.cpp` — clean **1:1** (reads exactly `window.activeRoom` + `window.activeRoomCollision`, verified). | **#1, score 9.0 — IN FLIGHT.** `activeRoom` rides in on the collision slice's G4/G5 removal gates; one `ensureActiveRoomCollisionFresh` verb rebuilds the pair, killing the window↔roomEditing duplicate. Do **not** stand up a second store. |
| **2** | **CreativeIdentityStore** *(mirror-delete, not a new store)* | `activeCreative` (265) — a 12-field mirror of `creativeApp.identity`. **Target: DELETE.** Identity lives on `creative::CreativeAppState`. | YES / n/a — derived mirror; fix is **delete**, not tokenize. | `SaveStateFields.cpp` (reads `activeCreative.saveStatus`). **MISMATCH:** emitted in the *save* receipt, not a creative-identity one — reader migrates onto `creativeApp.identity`. | **#2, score 4.0 (= audit #10, re-issued, 2 writers).** Migrate FrontendRouter + `SaveStateFields` readers onto `creativeApp.identity`; fix the independent writer `save/Flow.cpp:78-79` (`recordPauseCreativeFacadeMissing`); delete `window.activeCreative`. |
| **3** | **CreativeFlyAnchorStore** *(new freshness store, exemplar one size smaller)* | `viewport.creativeFlyPositionMeters` + `creativeFlyAnchorValid` (nested in `ProductViewportState`) — **carved OUT of ViewportStore**. Holds `{positionMeters, provenance, session token}`. | **YES / YES** — `creativeFlyAnchorValid` is a hand-rolled stale-bit set by 3 provenances with no who/when stamp. Verb = `ensureFreshAnchor(session)`. | **None** — anchor is not receipt-emitted; readers are camera/projection (`mapMakerAnchorFor`, `cameraAnchorOverride`, `makeProductVulkanFrame`, InputFrame fly-integrate). | **#3, score 3.0.** Own new store + `ensureFreshAnchor(session)`. `[CS]` latent (no snapshot ring touches viewport) → no acute deadline; waits for its own Gate-0 preflight. |
| **4** | **CreativeAuthoringStore** | `creativeDocumentRevision` (266), `creativeDocumentChangedThisFrame` (267), `creativeUndo` (268), `creativeBakedRoomStale{,DocumentId,Revision,Status,ReasonCode}` (269–275), `creativeNavigateActive` (280), `creativeUiProjection/Input/Last/Command` (363–366), `creativeBakedRoomAutoRefresh` (367), `creativeViewportPick*` (368–392, 25 fields), `creativeWireframe*` (393–420, 40 fields), `worldSetup` (231), `worldCreation` (232), `asciiRoomDraft` (233), `asciiRoomPreview` (234), `asciiRoomActivation` (235), `roomEditing` (236), `roomEditingLast*` (237–242), `roomEditorCursorReady` (243), `roomEditorCursor` (244), `roomEditor{Status,ReasonCode,LastOperation,LastOperationAccepted,LastPrimitiveId}` (245–249), `roomEditorOverlay/Preview/PlacementPreview/Hud` (250–253). Full creative + room-authoring surface. | Mostly **write-once-per-frame telemetry**, NOT scattered-derived-cache. `creativeBakedRoomStale*` already carries a `{documentId,revision}` token (the model to copy); `creativeUndo` is recomputed each frame from `creativeApp.undoStack` (authoritative). / **NO** freshness landing. | **Split across FOUR** — `CreativeUiFields.cpp` + `CreativePickWireframeFields.cpp` + `CreativeReceiptRecording.cpp` (superset re-emitter) + `WorldAuthoringFields.cpp`. **MISMATCH — do NOT fork four ways.** | **Not queued.** Audit *clears* the load-bearing members. Later **structural move-off-god-struct**, not a freshness slice. |
| **5** | **SaveSessionStore** | `saveSession` owns `saveFlow`, `saveDelete`, `saveRecover`, `productSaveLoadResult`, `productSaveLoad{Source,SelectedId,SelectedEnabled}`, `savedMarkerBind`, `selectedProductSave`, `productSave{Status,ReasonCode,DurableReason,Source,SaveId,SessionSaved}`, `activeProductSaveId`, `saveSlot*`, and `deleted*Save*`. `runtimeSessionCreated` is excluded and corrected to `GameplayStore`. | **NO / NO** — plain owned selection + result + flow status (seed implied derived; it is not). | `SaveStateFields.cpp` — largely 1:1 through `window.saveSession`, BUT also reads `activeCreative` (→#2) and `roomEditor*` (→#4). **MISMATCH:** those readers migrate OUT. | **DONE.** Straight move-off-god-struct into `SaveSessionStore`; no freshness token needed. |
| **6** | **ViewportStore** | `ProductAppWindowState::viewport` now owns camera/viewport state plus the folded `mapMaker*` and `mapMakerGrid*` fields moved by E155. | **NO / NO** — structural fold complete; no freshness token was added because map-maker status/grid fields are plain owned state. | `window.viewport` remains read by gameplay receipt appenders; `mapMaker*` receipt reads moved through `FrontendSettingsWindowFields.cpp` without changing keys/order. | **DONE as E155.** Structural fold into existing `ProductViewportState`; creativeFly anchor remains carved out as #3. |
| **7** | **InputDeviceStore** | `ProductAppWindowState::inputDevice` now owns the G1 device/action fields moved by E165 (`gamepadAvailable`, `gamepadName`, `gamepadMapping`, `lastInputAction`, `lastInputAccepted`), the G2 controller/capture fields moved by E166 (`mouseCapture`, `controllerModeToggle`, `controllerAction`), and the G3 interaction-mode fields moved by E167 (`interactionMode`, `interactionModeHud`). `gamepadMenuSelectUsed` is excluded and belongs to **FrontendWindowShell #10**. **DELETES** top-level `inputOwner` (349) + `gameplayInputSuppressed` (326). | **NO / NO** — owned input-surface + device identity. | `FrontendSettingsWindowFields.cpp` + `FeedbackSurfaceAutomationVulkanFields.cpp` (lastInputAction/Accepted). **MISMATCH — split two ways.** | **DONE as E165-E167.** Prior partial landing deleted `window.inputOwner` + `window.gameplayInputSuppressed`; `mouseCapture.inputOwner` stays inside `inputDevice.mouseCapture`. |
| **8** | **GameplayStore** | `ProductAppWindowState::gameplay` now owns the gameplay lifecycle flags, command/movement/planner state, action/outcome/tape state, product transition state, and visibility/render diagnostics moved by E157-E160. | **NO / NO** — structural move complete; no freshness token was added because these fields remain flat count/status mirrors and stateless per-frame diagnostics. | **Still emitted through existing gameplay receipt appenders** — `GameplayRuntimeMovementFields.cpp` + `GameplaySceneStateFields.cpp` + `PhysicsReceiptRecording.cpp`; receipt shape was intentionally preserved. | **DONE as E157-E160.** Structural move-off-god-struct complete. |
| **9** | **DebugHudStore** | `ProductAppWindowState::debugHud` now owns the five HUD/debug mirrors moved by E162: `topDownMap`, `devCollisionOverlay`, `npcBehaviorDebugHud`, `physicsDebugHud`, `positionHud`. | **NO / NO** — recomputed-and-recopied every projection frame; only readers are receipt emitters. | **Split across THREE** — `DebugHudFields.cpp` + `FrontendSettingsWindowFields.cpp` (topDownMap, devCollisionOverlay) + `GameplaySceneStateFields.cpp` (positionHud). **MISMATCH preserved without changing receipt keys.** | **DONE as E162.** Structural move into `src/app/iggy3d/debug/DebugHudStore.hpp`; no freshness token. |
| **10** | **FrontendWindowShell** | `startup` (230), `launchAction` (227), `launchStatus` (228), `packageLoadStatus` (229), `openingMenuVisible` (198), `menuTextDrawn` (199), `selectedRowDrawn` (200), `mouseMenuSelectUsed` (201), `gamepadMenuSelectUsed` (203), `selectedSettingsTab` (224), `automationControl` (352), `productVulkanMenu` (362), `menuRowCount` (423), `framesPresented` (421), `eventPollCount` (422), `status` (424). | **NO / NO** — plain owned lifecycle/telemetry. | **Split across FOUR** — `StartupProbeFields.cpp` + `StartupWorldBuildoutFields.cpp` + `FrontendSettingsWindowFields.cpp` + `TailFields.cpp` (head=startup, tail=status). | **Not queued.** Structural move. |
| **11** | **PresentPathStore** *(render/Vulkan present coordinator)* | `ProductAppWindowState::presentPath` now owns the nine present-loop members moved by E163: `productVulkanRenderer`, `productVulkanSurfaceCreated`, `productVulkanSwapchainReady`, `productVulkanFrameSubmitted`, `productVulkanFrameSubmittedCount`, `productVulkanStatus`, `productVulkanReasonCode`, `productVulkanRenderingPath`, `productVulkanRecordMode`. `productVulkanMenu` stays in FrontendWindowShell. | **NO / NO** — present-loop status mirrors. | `FeedbackSurfaceAutomationVulkanFields.cpp`; receipt key order preserved. | **DONE as E163.** Structural move into `src/app/iggy3d/window/PresentPathStore.hpp`; no freshness token. |

> **Fire order (leverage × readiness):** **1** RoomStore (in flight via the collision guard) → **2** CreativeIdentityStore (cheapest standalone delete) → **3** CreativeFlyAnchorStore (its own preflight, latent-CS) → then structural Stores **4–11** as capacity allows. Only 1–3 have live deficit slices; 4–11 are move-off-god-struct with no freshness debt.

### Receipt-vs-state mismatches (6 seams — receipt split is a HINT, not a law)

1. `SaveStateFields.cpp` bundles `activeCreative` (→#2) + `roomEditor*` (→#4) with true save state — readers migrate OUT.
2. **CreativeAuthoringStore's one state-domain is fractured across FOUR appenders** — do NOT fork the store four ways.
3. **GameplayStore fractured across THREE**, with `physicsMovementPlanner` **double-emitted**.
4. **DebugHudStore's five HUDs scattered across THREE** appenders.
5. `window.viewport` read by **both** gameplay appenders; `mapMaker*` by settings — no single viewport appender.
6. **InputDeviceStore split TWO ways** (FrontendSettingsWindow + FeedbackSurfaceAutomationVulkan).

---

## 3. The app-global remainder — fields that genuinely STAY on the window

SDL/window-existence **lifecycle bits** — not domain truth, not derived caches. **Do not over-store them.**

| Member | Line | Why it stays |
|--------|------|--------------|
| `requested` | 194 | SDL window-creation requested — window lifecycle. |
| `sdlAvailable` | 195 | SDL subsystem availability — app lifecycle. |
| `created` | 196 | Window created — lifecycle bit. |
| `drawable` | 197 | Drawable surface exists — lifecycle bit. |

> **4 fields.** Everything else lands in a Store. Present path/status/surface readiness lives in
> **PresentPathStore #11** so the render receipt consumes one present-loop owner.

---

## 4. DONE definition

The god-struct is decomposed when **all four hold**:

1. **`ProductAppWindowState` is a thin composition** — its body contains only the ~4 app-global lifecycle bits (§3). Every other member has moved into one of the 11 Stores.
2. **`AppKernel` holds the 11 Stores** as members (alongside today's `activeSession`/`creativeApp`/`frontend`/`saves`/`settings`/`worldSetupDraft`) and is the sole composer.
3. **The three derived Stores carry a freshness token + one `ensure*` verb** — RoomStore (`ensureActiveRoomCollisionFresh`, `{roomRevision, sessionHash}`), CreativeFlyAnchorStore (`ensureFreshAnchor(session)`), and the CreativeIdentity mirror is **deleted** (identity reads hit `creativeApp.identity` directly). No stored derived value survives without a provenance stamp.
4. **The audit's DELETEs are gone** — `window.inputOwner`, `window.gameplayInputSuppressed`, `window.runtimeStateHash` are removed (not rehomed); their readers re-derive via `resolveProductActiveSurface` / `session.stateHash()`.

**Coverage invariant:** every member (lines 193–425) lands in **exactly one** of: a Store (1–11), the app-global remainder (§3), or the audit DELETE set (`inputOwner`, `gameplayInputSuppressed`, `runtimeStateHash`). None unaccounted; none twice.

*Full source at `/private/tmp/claude-501/-Users-kogaryu-iggy3d/b9aedc32-186b-4229-a203-ff1cc12b8f24/scratchpad/target_map.md`.*
