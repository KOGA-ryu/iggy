<!-- Gate-0 preflight v0.2 (2026-07-07), 5-agent workflow + adversarial critique. NEEDS_FIXES on caller inventory completeness -- see the sizing note. The design SHAPE (4-gate, thread-first-delete-last, leaf stays mirror-backed until G4) is compiler-safe: the exact caller list need not be perfect because the G4 field-delete makes every un-migrated reader a compile error, finalizing the inventory. -->

## SIZING NOTE (post-critique)

**This is L, not M/L.** Two preflight passes each found more callers (v1 missed the routing hot path; v2 missed four bridge/ui wrapper predicates + ~7 test fabricators). The mirror is deeply embedded in routing + test infrastructure. **This is fine and safe** because the migration is *compiler-guarded*: keep the leaf predicate mirror-backed through G1-G3, thread `creativeApp` only where it already exists, then at **G4 delete the field** -- the compiler flags every remaining reader, which are fixed in G4. So the inventory is best-effort for *sizing*; the compiler *finalizes* it. Add to the v2 inventory (critique): the 4 wrappers `productCreative{Input,WireframeFrame,ViewportPick,Ui}ActiveForWindow` (give each a `const creative::CreativeAppState*` param) + their direct test assertions, and the ~7 test files that set `window.activeCreative.*` with no creativeApp -- migrate to `creativeApp.identity`.

---

# Preflight v2 — Delete `window.activeCreative` (12-field mirror of `creative::CreativeActiveIdentity`)

**Classification:** M/L move-off-god-struct (NOT the full spine ceremony, but multi-gate because the mirror feeds a **hot-path routing predicate** with a wide caller fan-out). v1 under-scoped this as "S delete"; the adversarial critique proved M/L. This v2 pins the caller fan-out + `creativeApp` availability exactly.

**Recon verdict:** every critique finding re-checked against current code and **CONFIRMED** (with 3 corrections logged in §8). The field is a pure derived copy; repointing every reader to the identity is output-preserving; the receipt GOLDEN does not move.

---

## 1. Problem

### 1a. The mirror is a pure derived copy of the identity
`window.activeCreative` is a `ProductActiveCreativeState activeCreative` member of the god-struct. The struct (`src/app/iggy3d/ProductActiveCreativeState.hpp:11-24`) has **exactly 12 fields**, a byte-for-byte twin of `creative::CreativeActiveIdentity` (`src/app/iggy3d/creative/CreativeAppState.hpp:18-57`) — same field names, same defaults:

| both sides default to | fields |
|---|---|
| `"none"` | saveId, savePath, worldId, saveSavedAtUtc |
| `"creative_world_save_not_requested"` | saveStatus, saveReasonCode |
| `0` | documentId (`kInvalidDocumentId==0`, `Document.hpp:21`), nextObjectId (`kInvalidObjectId==0`, `Object.hpp:16`), objectCount, saveDirtyFlagsBefore/Drained/After |

The mirror is written **only** through `mirrorProductActiveCreativeIdentity` (`Operations.cpp:31-56`) and `clearProductActiveCreativeIdentity` (`Operations.cpp:58-69`), both of which copy from the identity and re-apply the *same* `empty()->"none"` / `empty()->"creative_world_save_not_requested"` normalization the identity already carries. **The identity is the sole source of truth; the mirror is redundant at every read.**

### 1b. The hot-path routing predicate (the reason this is M/L)
The routing leaf reads only **3** of the 12 fields:

- `productCreativeWorldActiveForWindowMirror` (`FrontendRouter.cpp:324-331`) — reads `activeCreative.{saveId,worldId,documentId}`. Its body is **character-identical** to `CreativeActiveIdentity::worldActive()` (`CreativeAppState.hpp:52-56`).
- `productCreativeWorldActiveForWindow` (`FrontendRouter.cpp:315-317`) delegates to it.
- `productCreativeDocumentEditorActiveForWindow` (`FrontendRouter.cpp:333-337`) = `interactionMode==Creative && productCreativeWorldActiveForWindow`.

**Identity twins already exist:** `productCreativeWorldActiveForIdentity(identity)` (`FrontendRouter.cpp:319-322` → `identity.worldActive()`) and the dual predicate `productCreativeDocumentEditorActiveForSource(window, creativeApp*)` (`Flow.cpp:82-90`) which uses the identity when `creativeApp!=nullptr` and falls back to the window mirror when null.

**Transitive fan-out is the real cost.** The mirror predicate is read through three *window-only* routing helpers that themselves fan out to ~7 more files:
- `productActiveSurfaceContextForWindow` (`FrontendRouter.cpp:251-262`, reads mirror at :259) → called from ReceiptBuilder, InteractionModeState, Loop, InputFrame, ActionHandlers, ProjectionRefresh, InputRouter.
- `productCreativeSurfaceKindForWindow` (`FrontendRouter.cpp:339-357`, reads at :342) → ReceiptBuilder.
- `productMapMakerLiveForWindow` (`FrontendRouter.cpp:309-313`) → ReceiptBuilder, InputFrame, ProjectionRefresh, ActionHandlers.

Making the **leaf** identity-only would force identity through all of these. The safe slice keeps the leaf mirror-backed as a fallback and repoints only where a live `creativeApp` already exists — deleting the mirror **last**.

### 1c. The receipt reader (heaviest independent consumer — not routing)
`appendProductSaveStateFields` (`receipt/SaveStateFields.cpp:24`, signature `(RenderReceipt&, const FrontendState&, const ProductAppWindowState&)`) makes **all 12** `activeCreative.*` reads at lines 35-58 and emits 12 `active_creative_*` receipt keys. It is called from `buildProductAppReceipt` (`ReceiptBuilder.cpp` / `.hpp:22-27`), whose only production caller is `AppKernel.cpp:135` (where `creativeApp` is in scope, passed as `&creativeApp` at :106/:124).

### 1d. The pause null-facade self-read + divergence (`Flow.cpp:65-80`)
`recordPauseCreativeFacadeMissing` runs on the `creativeApp==nullptr` branch (`Flow.cpp:105-106`) and:
- copies **live launched** `activeCreative.{saveId,worldId,documentId,objectCount,nextObjectId}` into `result.creativeSave.*` (`Flow.cpp:71-75`);
- writes `activeCreative.saveStatus`/`saveReasonCode = reason` **mirror-only** (`Flow.cpp:78-79`) — no identity counterpart, and the reason is *already* on `result.launchStatus` (:76) + `window.launchStatus` (:77).

---

## 2. Migration by layer

### (a) Routing predicates — move the callers to identity/source predicates

**Recommended predicate:** lift `productCreativeDocumentEditorActiveForSource(window, const creative::CreativeAppState*)` out of `Flow.cpp:82-90` into `FrontendRouter` (next to the existing predicates) so all callers can adopt it. A nullable `const CreativeAppState*` is sufficient — `interactionMode` still lives on `window`, the identity rides on `creativeApp`. Its `nullptr` fallback branch (`Flow.cpp:89`) is the **one line that retires last**.

**GROUP A — `creativeApp`/identity ALREADY in scope (switch predicate, no new threading):**

| caller | site | in-scope source |
|---|---|---|
| Flow pause | `Flow.cpp:182` | already uses `...ForSource` |
| InputFrame (context helpers) | `InputFrame.cpp:901,1364,1507` | `ProductWindowInputFrameContext.creativeApp` (`InputFrame.hpp:87`) |
| bridge InputFrame | `bridge/InputFrame.cpp:68` (+182/283) | request `creative` (`InputFrame.hpp:60/77`) |
| bridge WireframeFrame | `bridge/WireframeFrame.cpp:62` (81) | request `creative` (`WireframeFrame.hpp:19`) |
| bridge ViewportPickFrame | `bridge/ViewportPickFrame.cpp:44` (59) | request `creative` (`ViewportPickFrame.hpp:20`) |
| ui UiFrame | `ui/UiFrame.cpp:49` | request `creative` (used at UiFrame.cpp:73) |

**GROUP A-hop — one caller hop away (thread `creativeApp` into a plain helper whose caller holds it):**

| caller | site | source |
|---|---|---|
| `productWindowEditorMousePickSurfaceReady` | `InputFrame.cpp:280` | caller `InputFrame.cpp:943` holds `context.creativeApp`, BUT the intermediate `ProductWindowEditorMousePickPreviewContext` (`InputFrame.hpp:115-121`) has **no** creativeApp field → add one, sourced from the enclosing `ProductWindowInputFrameContext` |
| `updateProductWindowMouseCapture` | `InputFrame.cpp:317` | caller (`~InputFrame.cpp:1597`) holds `context.creativeApp` → thread as param |

**GROUP B — `creativeApp` NOT in scope, request/context struct must gain a defaulted field (this is what makes the slice M/L):**

| caller | site | struct to extend | build site that has `creativeApp` |
|---|---|---|---|
| `presentProductVulkanFrame` | `FramePresenter.cpp:787` | `ProductWindowFramePresenterRequest` (`FramePresenter.hpp:54-68` — no field) | Loop.cpp:285 (`request.creativeApp` live) |
| projection HUD / frame | `ProjectionRefresh.cpp:39,697,756,768,816` | `ProductGameplayProjectionFrameRequest` (`gameplay/ProjectionRefresh.hpp:42`) + `ProductGameplayProjectionRefreshRequest` (:27) | Loop.cpp:280 build + `AppKernel.cpp:127` `refreshProductGameplayProjectionMetrics` (creativeApp in scope) |
| `productWindowTitle` | `Loop.cpp:49` (def :30) | plain `(frontend, window)` → add param | callers Loop.cpp:181/191 hold `request.creativeApp` |
| `recordNoWindowMouseCapturePolicy` | `Loop.cpp:68` (def :57) | plain `(frontend, window)` → add param | caller Loop.cpp:138 holds `request.creativeApp` |

**GROUP C — HARDEST, no `creativeApp` path at all (the true scope drivers v1 missed):**

| caller | site | context | resolution |
|---|---|---|---|
| `applyProductGameplayMapMakerToggleAction` | `ActionHandlers.cpp:788` | `ProductGameplayMapMakerToggleActionContext` = `{frontend, window}` only (`ActionHandlers.hpp:86-89`) | add `const creative::CreativeAppState* creative` to the context, thread from the app kernel dispatch |
| `clearProductPauseOwnedTransientModes` | `Transitions.cpp:24-27` | window-only; caller `openProductPauseTransition(frontend, window, action)` at :184 has **no** creativeApp | thread an identity/creativeApp arg down, OR keep this one on the mirror fallback until last |

> **Do not collapse Group C into the leaf-flip.** Route these through `...ForSource` with the mirror fallback intact so they keep working until G4.

### (b) Receipt reader — thread the identity through `buildProductAppReceipt`

Add a `const creative::CreativeActiveIdentity&` parameter (pass the **POD identity by const-ref**, NOT `CreativeAppState*` — keeps the new `creative/` coupling to the identity struct only):

1. `appendProductSaveStateFields` — repoint the 12 reads (`SaveStateFields.cpp:35-58`) to `identity.*`.
2. `buildProductAppReceipt` (`ReceiptBuilder.hpp:22-27` + `.cpp`) — add the param, forward to `appendProductSaveStateFields`.
3. Production call `AppKernel.cpp:135` — pass `creativeApp.identity`.
4. **17 test files / 24 call sites** — pass a default `CreativeActiveIdentity{}` (golden unchanged). Files: `product_{interaction_mode_state, movement_debug_hud, creative_ui_window_frame, mouse_capture_policy, creative_ui_projection_receipt, window_renderer_lifecycle, creative_ui_command_receipt, creative_ui_input_frame, creative_wireframe_frame, vulkan_room_frame, receipt_key_order, menu_transitions, creative_ui_frame, frontend_router, top_down_map_overlay, creative_world_launch, creative_viewport_pick_frame}_tests.cpp`.
   - **Do NOT default-silence the two that assert live identity content:** `product_frontend_router_tests` and any launch/pause receipt test that today sets `window.activeCreative.*` must switch to setting `creativeApp.identity` (or their asserted golden diverges).

### (c) Pause self-read + divergence (`Flow.cpp:65-80`)

- **Drop** `Flow.cpp:78-79` (the mirror-only `saveStatus`/`saveReasonCode` write) — ratified, see §3.
- **The `Flow.cpp:71-75` 5-field self-read:** on the `nullptr` branch there is **no identity to hoist** (the enclosing fn's `creativeApp` is null at :99/:105). These 5 become explicit **defaults** (`none`/`0`). Behavior note in §3.
- **The `Flow.cpp:89` fallback gate:** once the mirror is gone, `...ForSource` on the `nullptr` branch can no longer read the mirror. Re-express it to key on `window.interactionMode == ProductInteractionMode::Creative` alone (or a default identity → `worldActive()==false`). Verify the `creativeApp==nullptr` test path through `executeProductPauseSaveFlow` (`Flow.cpp:182`) still routes identically (`ok` at :116 is already `false` there, so return-to-title is unaffected — only the receipt's `active_creative_save_*` values change).

### (d) Delete the funnel + struct + field + TSV row (G4)

1. Delete `mirrorProductActiveCreativeIdentity` + `clearProductActiveCreativeIdentity` bodies (`Operations.cpp:31-69`) and their decls (`Operations.hpp:117-121`); the identity's own `clear()` (`CreativeAppState.hpp:36-49`) is the surviving reset.
2. Repoint the 6 writer call sites:
   - `Operations.cpp:817,846` (mirror sync) → delete (identity already holds the value).
   - `Operations.cpp:1353,1682` — these call `clearProductActiveCreativeIdentity(window)` with **identity=nullptr** on gameplay-entry (menu-new-world / load). No creativeApp in scope; the mirror is being abandoned → **drop the call** (identity is separately owned and re-initialized on next creative launch) OR clear a reachable `creativeApp->identity` if the enclosing fn gains it.
   - `ActionHandlers.cpp:336`, `Flow.cpp:131` — already pass `&...->identity`; drop the mirror half, keep `identity.clear()`.
3. Delete the mirror predicate `productCreativeWorldActiveForWindowMirror` (`FrontendRouter.cpp:324-331`); make `productCreativeWorldActiveForWindow` route through the identity (or delete it and inline `...ForSource`).
4. Delete the `activeCreative` member from the god-struct and the `ProductActiveCreativeState` struct/header.
5. Remove the `activeCreative\tdelete` row from `docs/god_struct_member_ownership.tsv:72`.

---

## 3. Divergence resolution + Reader-3 behavior note

**Divergence (RATIFIED-RECOMMENDED — drop the write):** `Flow.cpp:78-79` writes `activeCreative.saveStatus/saveReasonCode = "product_creative_save_facade_missing"` with no identity counterpart. This is **safe to drop** — the same reason is already recorded on `result.launchStatus` (`Flow.cpp:76`) and `window.launchStatus` (`Flow.cpp:77`). The only observable loss is the receipt's `active_creative_save_status`/`_reason_code` showing that reason **in the null-facade test path** (no decision consumer reads it).

**Reader-3 live→default behavior note (VERIFIED):** today `Flow.cpp:71-75` copies the **live launched** `activeCreative.{saveId,worldId,documentId,objectCount,nextObjectId}` (last mirror write) into `result.creativeSave.*` on the `nullptr` branch. After the slice they become **defaults** (`none`/`0`). This is a genuine value change on `result.creativeSave` for that branch, BUT **no consumer reads it there**: `ok` (`Flow.cpp:116`) = `creativeSaveAccepted && creativeSaveSaved`, both `false` on this branch, so routing / return-to-title never touch the 5 ids — they surface only in the receipt. **Document in the commit; grep pause-save receipt tests for `facade_missing` + `active_creative_save_*` before landing** to catch any test that pins the live values.

---

## 4. Truth-gate obligations

1. **TSV:** remove `docs/god_struct_member_ownership.tsv:72` (`activeCreative\tdelete`). This is the field-ownership registry; the row's obligation is satisfied by the delete. (No separate coverage TSV under `tests/` references `activeCreative` — verified; the memory note's "coverage TSV" == this ownership TSV.)
2. **Receipt GOLDEN does NOT move — and here is why:** `tests/golden/product_receipt_key_order.golden:282-293` carries the 12 `active_creative_*` rows at **exactly the identity defaults** (`none` / `0` / `creative_world_save_not_requested`). Re-pointing the 12 reads to a **default `CreativeActiveIdentity{}`** reproduces those rows byte-for-byte, and the 12 emits stay in place so key order is unchanged. The fixture never launches a creative world, so default identity == default mirror.
3. **Identity-is-normalized invariant (guard):** the receipt equivalence depends on identity fields being pre-normalized (`empty()->"none"`, `empty()->"creative_world_save_not_requested"`), which every writer does today (`Operations.cpp:805-816/824-844`, mirrored in `mirrorProductActiveCreativeIdentity`). Add a comment on `CreativeActiveIdentity` that these fields are the normalized form the receipt reads directly — a future raw-empty writer would emit `""` and drift the golden.

---

## 5. Gate / slice sequence (each suite-green, independently reviewable)

> Sequence law: **thread `creativeApp` everywhere FIRST, delete the mirror LAST.** A half-migrated caller that reads a deleted field is a compile break; one that reads a stale identity is a behavior break.

### G1 — Add identity predicate + establish `creativeApp` at the hot callers (no behavior change)
- Lift `productCreativeDocumentEditorActiveForSource(window, creativeApp*)` into `FrontendRouter`; keep its mirror fallback.
- Add defaulted `const creative::CreativeAppState* creative=nullptr` to the Group-B request structs (`ProductWindowFramePresenterRequest`, both ProjectionRefresh requests) and populate from `request.creativeApp` at the Loop/AppKernel build sites. Add `creative` to `ProductGameplayMapMakerToggleActionContext` and `ProductWindowEditorMousePickPreviewContext`.
- **No predicate reads switch yet.** Suite green proves the plumbing compiles and the new fields are wired without changing routing.

### G2 — Migrate routing callers to the source/identity predicate
- Switch Group A + A-hop + B + C callers of `...WorldActiveForWindow` / `...DocumentEditorActiveForWindow` to `...ForSource(window, creative)` (or `...ForIdentity` where only `worldActive()` is needed — but **preserve the `interactionMode==Creative &&` composite**, don't drop it).
- The three window-only routing helpers (`productActiveSurfaceContextForWindow` etc.) stay mirror-backed for now (their fan-out is out of scope until the leaf is safe).
- Co-migrate `product_frontend_router_tests.cpp:617-700` (`creativeSurfaceClassifierSplitsDocumentFromLegacyMapMaker`) which asserts `productCreativeWorldActiveForWindowMirror` and mirrors identity into window via `markCreativeDocumentWindow`; rewrite `staleIdentity.activeCreative.documentId=42U` to set the identity. Update `product_creative_world_launch_tests.cpp:4200`.
- Suite green.

### G3 — Receipt threading
- Add `const creative::CreativeActiveIdentity&` to `buildProductAppReceipt` → `appendProductSaveStateFields`; repoint the 12 reads.
- Prod: `AppKernel.cpp:135` passes `creativeApp.identity`. Tests: default identity in 15 files; **live identity** in `product_frontend_router_tests` + any launch/pause receipt test that set `activeCreative.*`.
- Suite green; **golden unchanged** (§4.2).

### G4 — Drop divergence + delete the mirror + TSV row
- Drop `Flow.cpp:78-79`; convert the `Flow.cpp:71-75` self-read to defaults; re-express the `Flow.cpp:89` fallback.
- Delete the funnel (`Operations.cpp:31-69` + decls), repoint the 6 writer sites, delete the mirror predicate, delete the struct + god-struct field, remove the TSV row.
- Suite green; golden unchanged; `grep activeCreative` → zero.

---

## 6. Acceptance

- `grep -rn "activeCreative" src/ tests/` → **zero** hits (field, struct, mirror predicate, and every test reference gone).
- `grep -rn "ProductActiveCreativeState\|mirrorProductActiveCreativeIdentity\|productCreativeWorldActiveForWindowMirror" src/` → zero.
- Full unit suite green, reported **verbatim** (baseline count from the current tree — capture at G1 start, must not regress).
- `tests/golden/product_receipt_key_order.golden` **unchanged** (byte-diff clean) — receipt key-order gate green.
- `docs/god_struct_member_ownership.tsv` no longer contains the `activeCreative` row; TSV coverage/ownership gate green.
- Each gate G1–G4 lands as its own commit, each independently suite-green.

---

## 7. Rollback

- Per-gate: `git revert` the gate's single commit. G1–G3 are additive/output-preserving (defaulted params, unswitched fallback), so reverting any one leaves the tree green without touching the others.
- G4 is the only irreversible-in-behavior gate (deletes the struct). If a hidden reader surfaces post-G4, revert G4 alone to restore the mirror + funnel + TSV row; G1–G3 remain in place (the identity predicates and receipt threading are harmless with the mirror present — the mirror fallback in `...ForSource` simply goes unused).
- Guard against ordering slips: if any gate goes red, revert **only that gate** — do not fold fixes forward into the next gate, or the "each gate suite-green" invariant (and the clean bisect) is lost.

---

## 8. Recon corrections logged (verified against current code)

1. **Field/receipt count:** the mirror is **12 fields**, and the receipt reads **all 12** (not just the 3 routing fields). Prior notes that said "12-field mirror" + "12 receipt keys" are both correct but describe *different* subsets vs the 3-field routing predicate — the doc now separates them.
2. **`buildProductAppReceipt` test churn:** **17 files / 24 calls**, NOT "~28 test call sites." No shared receipt helper — each file's local helper needs the arg.
3. **ProjectionRefresh path:** lives at `src/app/iggy3d/gameplay/ProjectionRefresh.{hpp,cpp}` (recon said `window/`). Two request structs (`ProductGameplayProjectionRefreshRequest` :27, `ProductGameplayProjectionFrameRequest` :42), neither has `creativeApp`; the refresh entrypoint `refreshProductGameplayProjectionMetrics` is called from `AppKernel.cpp:127` where `creativeApp` is live.
4. **Writer clear-sites gap:** `clearProductActiveCreativeIdentity(window)` at `Operations.cpp:1353,1682` passes **identity=nullptr** (gameplay-entry from menu/load) — no creativeApp in scope. G4 must drop these calls rather than repoint them (mirror abandoned; identity separately owned). Not called out in v1.
5. **Coverage TSV grounding:** the only receipt golden referencing `active_creative` is `product_receipt_key_order.golden`; the only ownership registry is `docs/god_struct_member_ownership.tsv:72`. There is no separate `tests/`-side coverage TSV — the "REMOVE the activeCreative row" obligation resolves to the ownership TSV.

---

# FIRST IMPLEMENTATION CARD — Gate G1 (builder E-card format)

```
E-CARD: G1 — Add identity/source predicate + thread creativeApp to the hot routing callers (no behavior change)

GATE:    Move-off-god-struct, gate 1 of 4. Additive plumbing only. ZERO routing/receipt behavior change.
         (Full slice = delete window.activeCreative; this gate only establishes the identity path.)

WHY:     window.activeCreative is a pure derived copy of creative::CreativeActiveIdentity. To delete it we
         must first give every hot-path routing caller access to a live creativeApp. This gate wires that
         access WITHOUT switching any predicate read, so it is provably output-preserving.

DO:
  1. Lift the dual predicate into FrontendRouter (keep the mirror fallback):
       - Move productCreativeDocumentEditorActiveForSource(const ProductAppWindowState&,
         const creative::CreativeAppState*) from save/Flow.cpp:82-90 into FrontendRouter.cpp/.hpp,
         next to productCreativeWorldActiveForIdentity (FrontendRouter.cpp:319-322).
       - Flow.cpp keeps calling it (now via the FrontendRouter header); its :182 call site is unchanged.
       - Body unchanged: creativeApp!=nullptr -> interactionMode==Creative &&
         productCreativeWorldActiveForIdentity(creativeApp->identity); else fall back to
         productCreativeDocumentEditorActiveForWindow(window).

  2. Add a defaulted creativeApp field to the Group-B request/context structs (populate from the
     build site that already holds request.creativeApp; DO NOT read it anywhere yet):
       - ProductWindowFramePresenterRequest (window/FramePresenter.hpp:54-68):
           add `const creative::CreativeAppState* creative = nullptr;`
           populate at Loop.cpp:285 from request.creativeApp.
       - ProductGameplayProjectionFrameRequest (gameplay/ProjectionRefresh.hpp:42) AND
         ProductGameplayProjectionRefreshRequest (gameplay/ProjectionRefresh.hpp:27):
           add `const creative::CreativeAppState* creative = nullptr;`
           populate at Loop.cpp:280 (frame) and at AppKernel.cpp:127 refreshProductGameplayProjectionMetrics
           call (refresh) from the in-scope creativeApp.
       - ProductGameplayMapMakerToggleActionContext (menu/ActionHandlers.hpp:86-89):
           add `const creative::CreativeAppState* creative = nullptr;`
           populate from the app-kernel dispatch site that builds this context.
       - ProductWindowEditorMousePickPreviewContext (window/InputFrame.hpp:115-121):
           add `const creative::CreativeAppState* creativeApp = nullptr;`
           populate from the enclosing ProductWindowInputFrameContext at InputFrame.cpp:943.

  3. Forward-declare creative::CreativeAppState in the touched headers (avoid pulling CreativeAppState.hpp
     into FramePresenter/ProjectionRefresh/ActionHandlers headers; a fwd-decl of the pointer type suffices).

DO NOT:
  - Switch ANY predicate read to the identity (that is G2). Every ...ForWindow read stays as-is.
  - Touch the receipt (G3), Flow.cpp:71-79 (G4), or delete anything (G4).
  - Change the mirror predicate FrontendRouter.cpp:324-331 or the writer funnel.

FILES:
  src/app/iggy3d/menu/FrontendRouter.hpp        (declare ...ForSource)
  src/app/iggy3d/menu/FrontendRouter.cpp        (define ...ForSource, moved from Flow)
  src/app/iggy3d/save/Flow.cpp                  (remove local def, include FrontendRouter header)
  src/app/iggy3d/window/FramePresenter.hpp      (+creative field)
  src/app/iggy3d/gameplay/ProjectionRefresh.hpp (+creative field on both request structs)
  src/app/iggy3d/menu/ActionHandlers.hpp        (+creative field on toggle context)
  src/app/iggy3d/window/InputFrame.hpp          (+creativeApp on mouse-pick preview context)
  src/app/iggy3d/window/Loop.cpp                (populate FramePresenter + ProjectionFrame requests)
  src/app/iggy3d/AppKernel.cpp                  (populate ProjectionRefresh request)
  src/app/iggy3d/window/InputFrame.cpp          (populate mouse-pick preview context)
  src/app/iggy3d/menu/ActionHandlers.cpp        (populate toggle context at dispatch)

TESTS:
  - No golden change. No new assertions required (this gate is plumbing).
  - Audit brace-init construction of the 5 extended aggregates in tests/unit/product_vulkan_*_tests.cpp,
    product_window_input_frame_tests.cpp, product_frontend_router_tests.cpp, product_menu_transitions_tests.cpp:
    the trailing defaulted field keeps positional/designated init compiling, but any test that brace-inits
    ALL fields positionally must append the new nullptr. Prefer designated-initializer call sites; fix
    positional ones by appending `nullptr` / `.creative = nullptr`.

ACCEPTANCE:
  - Full unit suite green, count == the baseline captured at G1 start (report verbatim).
  - git diff shows ONLY: one moved predicate + 5 new defaulted struct fields + their populate lines.
  - grep -n "productCreativeWorldActiveForWindow\|productCreativeDocumentEditorActiveForWindow" src/
    is UNCHANGED from pre-gate (no read switched).
  - tests/golden/product_receipt_key_order.golden byte-identical.

ROLLBACK: single-commit revert; additive-only, so revert leaves the tree green with no dependents.

REVIEW FOCUS: confirm no predicate READ changed (only the source predicate was relocated + new fields added
              but not yet consumed); confirm every new field defaults to nullptr; confirm the Loop/AppKernel
              populate sites actually pass the live creativeApp (not nullptr) so G2 has real data to switch to.
```
