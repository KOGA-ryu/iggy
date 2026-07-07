# E135: Delete `window.activeCreative` mirror (deficit #2) — one comprehensive card, to be sliced

## Objective

Delete `window.activeCreative` (the 12-field mirror of `creative::CreativeAppState::identity`); make
`creativeApp.identity` the **single source of truth** by re-pointing every reader; delete the mirror funnel +
struct + field + ownership-TSV row. **Compiler-guarded:** thread `creativeApp`/`identity` everywhere **first**,
delete the field **last** — the compiler turns every un-migrated reader into a compile error, finalizing the
inventory.

**This is ONE comprehensive card for the full L migration** — **36 non-test reader sites + 15 test files.**
It is the input to the slicer: decompose it into builder-safe, suite-green slices (see **§ Suggested slicing**).
**Design authority — read it fully:** `docs/activecreative_mirror_delete_preflight_v0_2.md` (v2 preflight,
Gate-1 ratified 2026-07-07). This card is the executable summary; the preflight has per-site detail.

## Ratified decisions (Gate-1, 2026-07-07)

- **Drop** the `Flow.cpp:78-79` mirror-only `saveStatus`/`saveReasonCode` write — the facade-missing reason is
  already on `window.launchStatus` (`:77`) + `result.launchStatus` (`:76`); no decision consumer loses info.
- **Accept the Reader-3 live→default change:** `Flow.cpp:71-75`'s 5 self-read fields
  (`saveId/worldId/documentId/objectCount/nextObjectId`) become defaults on the null-facade branch — safe
  because `ok==false` there (`Flow.cpp:116`), so routing/return-to-title never read them.
- **Receipt reader** gets `const creative::CreativeActiveIdentity&` **by const-ref** (POD coupling only, NOT
  `CreativeAppState*`).
- **Thread-first-delete-last:** keep the leaf predicate **mirror-backed** through the threading; delete the
  field last. Do NOT make the leaf identity-only early (drags in the three window-only fan-out helpers).

## The migration (per-site detail in preflight §2)

1. **Routing predicates.** Lift `productCreativeDocumentEditorActiveForSource(window, const CreativeAppState*)`
   from `Flow.cpp:82-90` into `FrontendRouter` so all callers can adopt it (nullable `creativeApp*`; uses
   identity when non-null, mirror fallback when null — the fallback retires last).
   - **Group A** (`creativeApp`/`identity` in scope → switch predicate): `Flow.cpp:182`, `InputFrame.cpp:901/1364/1507`,
     bridge `InputFrame/WireframeFrame/ViewportPickFrame`, `ui/UiFrame.cpp:49`.
   - **The 4 wrapper predicates** (give each a `const CreativeAppState*` param; each internal caller already
     holds `request.creative`) + co-migrate their direct test assertions:
     `productCreativeInputActiveForWindow` (`bridge/InputFrame.cpp:66`), `productCreativeWireframeFrameActiveForWindow`
     (`bridge/WireframeFrame.cpp:60`), `productCreativeViewportPickActiveForWindow` (`bridge/ViewportPickFrame.cpp:42`),
     `productCreativeUiActiveForWindow` (`ui/UiFrame.cpp:47`).
   - **Group A-hop** (thread one hop): `InputFrame.cpp:280` (add `creativeApp` to the mouse-pick preview context),
     `InputFrame.cpp:317` (thread param).
   - **Group B** (request/context struct gains a defaulted `const CreativeAppState* creative` field, sourced at
     the build site that has it): `FramePresenter.cpp:787` (`ProductWindowFramePresenterRequest`, build `Loop.cpp:285`),
     `ProjectionRefresh.cpp:39/697/756/768/816` (the two projection requests, build `Loop.cpp:280` + `AppKernel.cpp:127`),
     `Loop.cpp:49` (`productWindowTitle`) + `Loop.cpp:68` (`recordNoWindowMouseCapturePolicy`).
   - **Group C** (no `creativeApp` path → route through `...ForSource` with mirror fallback intact until the delete):
     `ActionHandlers.cpp:788` (map-maker toggle context gains `creative`), `Transitions.cpp:24-27` (pause).
2. **Receipt reader.** Thread `const CreativeActiveIdentity&` through `buildProductAppReceipt`
   (`ReceiptBuilder.hpp:22-27` + `.cpp`) → `appendProductSaveStateFields` (repoint the 12 reads
   `SaveStateFields.cpp:35-58` to `identity.*`, preserving the empty→`"none"`/`"..._not_requested"` normalization).
   Prod caller `AppKernel.cpp:135` passes `creativeApp.identity`. **~24 test call sites across 17 files** pass a
   default `CreativeActiveIdentity{}` (golden unchanged) — **EXCEPT** `product_frontend_router_tests` and any
   launch/pause receipt test that sets `window.activeCreative.*` today, which switch to `creativeApp.identity`.
3. **Pause self-read + divergence** (`Flow.cpp:65-90`): drop `:78-79`; `:71-75` 5 fields → defaults; re-express
   the `:89` fallback gate to key on `interactionMode==Creative` (no mirror). Verify the `creativeApp==nullptr`
   pause path still routes identically.
4. **Test fabricators** (~7 files set `window.activeCreative.*` with no `creativeApp` to drive routing):
   `product_creative_pick_flow_tests`, `product_creative_move_drag_frame_tests`, `product_vulkan_creative_ui_overlay_tests`,
   `product_creative_input_frame_tests`, `product_vulkan_room_frame_tests`, `product_creative_ui_window_frame_tests`,
   `product_creative_wireframe_frame_tests` → give each a `CreativeAppState` whose identity carries the live
   fields, route through the now-creative-aware predicate. `product_starter_menu_action_tests` mirror-sync
   assertions → assert `creativeApp.identity.*`.
5. **Delete.** `mirrorProductActiveCreativeIdentity` (`Operations.cpp:31-56`) + its 3 write call lines; drop the
   2 `identity=nullptr` clear calls at `Operations.cpp:1353/1682`; delete field `ProductAppWindowState.activeCreative`
   (`:267`) + include (`:32`); delete `src/app/iggy3d/ProductActiveCreativeState.hpp`; delete
   `productCreativeWorldActiveForWindowMirror` if it loses its last caller; **remove the `activeCreative` row
   from `docs/god_struct_member_ownership.tsv`.**

## Truth-gate obligations

- **Coverage gate** (`product_god_struct_ownership_coverage_tests`): remove the `activeCreative` row from the
  ownership TSV — the bijection stale-row check enforces it.
- **Receipt oracle** (`product_receipt_key_order_tests`): the golden does **NOT** move (default identity ==
  default mirror for the fixture). If any emitted VALUE changes (it shouldn't), regenerate.

## Acceptance

- `grep -rn "activeCreative" src | grep -v test` → **ZERO** (field + struct + funnel gone). The compiler flags
  every straggler at the delete step.
- **Full suite green and UNCHANGED** — behaviour-preserving (the mirror was redundant at every read). Coverage
  gate + receipt oracle green. No dangling read.

## Suggested slicing (for the slicer)

Natural seams = the preflight's 4 gates, each its own suite-green commit; the leaf stays mirror-backed until the last:
1. Lift `...ForSource` into `FrontendRouter` + thread `creativeApp` to Group-A/hot callers + the 4 wrappers — **no behavior change.**
2. Migrate Group-B/C routing callers (struct fields + threading).
3. Thread `identity` through the receipt (`buildProductAppReceipt` + the ~24 test sites).
4. Drop the divergence write + delete the mirror funnel/struct/field + TSV row + fix compile breaks.

## Do Not

- Do NOT delete the field before threading is complete; do NOT make the leaf predicate identity-only before the delete.
- Do NOT change identity semantics or reorder receipt fields. Do NOT name anything `Kernel`.
- Do NOT stage, commit, or push (unless the slicer's per-slice cards say otherwise).

## Completion Brief

- Files changed:
- Predicates/callers migrated (Group A / wrappers / B / C):
- Receipt threading (prod + test sites):
- Divergence + pause branch resolution:
- Delete + TSV row removed:
- `grep activeCreative src` → zero?:
- Suite / coverage / receipt oracle:
- Concerns/deferred:

## Decomposition Brief - Codex

- This parent work order was too large for one builder slice and has been
  decomposed into four ready cards:
  - `E136-activecreative-g1-source-predicate-plumbing.md`
  - `E137-activecreative-g2-routing-callers.md`
  - `E138-activecreative-g3-receipt-identity-threading.md`
  - `E139-activecreative-g4-delete-mirror.md`
- The parent is retained in `done/` as design input only. Builder should not
  claim this file directly.
- `PRIORITY.md` now pulls E136-E139 in numeric order.
