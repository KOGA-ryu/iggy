# Creative series review — 2026-07-03

> Multi-agent local review of the Codex creative series, range `33cdfe88^..722decb0`
> (32 commits, ~13.4k insertions, 74 files). 8 dimension finders (projection-math,
> pick-routing, command-input, mutation-receipts, facade-state, ui-projection, ub-memory,
> tests-quality), then TWO adversarial refuters per critical/major finding (logic lens +
> reachability lens; a finding survives only if neither refutes), then a completeness critic.
> Tally: 49 raw → 37 deduped → **16 CONFIRMED** (2 critical, 14 major), 2 plausible,
> 2 refuted, 17 minors passed through unverified (cost cap, logged).
>
> The KNOWN-DISEASE list (plan §7: SetLength/SetDepth axis bug, false-receipt sleepers,
> legacy Facade path, uint32/64 seam, app-lifetime facade, missing W0 targets, snap name
> collision, no draw adapter, empty live document) was EXCLUDED from finder scope — nothing
> below re-reports it.
>
> **Mid-review trunk movement:** the branch advanced to `cd6b9f4e` ("codex: W0 pin creative
> foundation contracts" — the two missing test targets + the product_spec §8 amendment) and
> the working tree carries uncommitted `MutationApply.{cpp,hpp}` W1 work. That in-flight work
> already fixes **F12** (AttachTo guard) and **P1** (aliased-verb receipt stamping) and converts
> two sleepers (UnlinkTarget, ClearSocket) to NoChange. Findings below are stated at the review
> anchor `722decb0`; in-flight coverage is marked.

## Cluster A — the live-wiring seam (unit-proven loop is broken in the running app)

The 4G loop proof composes every stage by hand and passes; the PRODUCTION composition in
`Loop.cpp`/`InputFrame.cpp` marries units and planes that no test exercises with a real click.

- **F1 [CRITICAL] `src/app/iggy3d/window/Loop.cpp:166` — high-DPI pick: physical-pixel viewport
  vs window-coordinate clicks.** Pick viewport is sized from `drawableExtent()` (pixels,
  Loop.cpp:152-159) while the click is raw `SDL_GetMouseState` (window points,
  MouseInput.cpp:16-21) passed unnormalized (InputFrame.cpp:1034, 1089-1100).
  `pointerToCreativeGridCoord` (ViewportPick.cpp:87-93) divides click by viewport size, so on
  the Retina Mac (highDpi hardwired at Loop.cpp:105, 2x) localX/Y max out at ~0.5: grid
  x,y ≥ 32 are **unreachable** and every pick lands at half the intended coordinate —
  silently, since `containsPointer` never rejects. The menu path normalizes the same click by
  `eventState.windowWidth/Height` (InputFrame.cpp:637-651); the creative path skips it.
  *Failure:* Retina, 1280×720 window: click on a Room at grid x=40 → localX=0.3125 → grid
  x=20 → Miss; three quadrants of the 64×64 grid are unpickable; landing picks select wrong
  objects, feeding wrong ids into mutation commands. Both refuters upheld; severity kept.

- **F2 [major] `src/app/iggy3d/window/Loop.cpp:142` — same DPI seam for UI hit regions.**
  Draw-list rects are fixed constants in virtual units; `virtualWidth/Height` is fed
  `drawableExtent` so the renderer scale is 1.0 (overlay drawn at 1 unit = 1 pixel), but
  `routeProductCreativeUiInputFrame` hit-tests those rects with the raw window-coordinate
  click. On 2x displays the drawn row sits at points y≈98 while its hit region is at y=196:
  clicks on visible buttons miss (fall through to pick/selection), clicks on empty screen hit
  invisible regions. Corrupts the click-consumption input to F1's pick.

- **F3 [major] `src/app/iggy3d/window/Loop.cpp:173` — pick plane pinned to z=0.** The app
  passes literal 0 as `creativeViewportPickZ`; `pickCreativeViewportCell` matches only cells at
  the request z. The 4G proof's own room projects at z=1 and the test must pass z=1 to hit it —
  the identical room created in the live app is **permanently unpickable** (receipt `miss` on a
  dead-center click). Kills multi-height content (the ASCII surface is explicitly multi-height).

- **F4 [major] `src/app/iggy3d/window/FramePresenter.cpp:844` — SDL renderer draws no creative
  overlay, but the input frame still consumes clicks.** Only the Vulkan path appends the
  overlay (762-768); `presentProductSdlFrame` ignores `creativeUiDrawList`, yet drawlist build +
  click routing key solely off `interactionMode==Creative`. On Vulkan-OFF builds (the Linux box
  gate config) toggling Creative activates a fully invisible click surface: the x∈[34,374] band
  swallows clicks and invisibly cycles tools.

- **F5 [major] `tests/unit/product_creative_pick_flow_tests.cpp:140` — the live chain is never
  exercised with a real click.** The 4G proof state-pins every stage composed BY THE TEST; the
  production composition (InputFrame.cpp:1034-1148, Loop.cpp:140-173) has no test that passes a
  click + draw list + session through the live entry point. This is the enabler of F1–F4:
  reorder/gate the live block arbitrarily and the whole suite stays green.

## Cluster B — click-consumption law is incoherent

- **F6 [major] `src/app/iggy3d/window/CreativeUiInputFrame.cpp:95` — higher-priority menu
  consumption bypasses creative suppression.** `routeProductCreativeUiDownstreamClick` checks
  `higherPriorityUiConsumed` BEFORE `creativeUiConsumed` and early-returns with
  `clicked=true, suppressed=false` (test-pinned). Combined with the starter band being
  hit-tested unconditionally (F7), one click can execute a creative UI command AND overwrite
  `frontend.selectedAction` AND flow into the viewport pick + selection dispatch.

- **F7 [major] `src/app/iggy3d/window/InputFrame.cpp:1080` — phantom starter/pause row hits
  during plain gameplay.** `openingMenuActionAt` hit-tests the menu band (x[30,370], y=150+52k)
  even when screen==Gameplay and nothing is drawn; the dispatch is a no-op but
  `higherPriorityMouseConsumed=true` is still set — defeating creative click suppression exactly
  under the overlay (the two bands overlap almost entirely).

- **F8 [major] `src/app/iggy3d/window/InputFrame.cpp:1136` — creative bridge and room editor
  double-handle the same keys/click.** `processProductCreativeInputActions` consumes nothing;
  the same actions then reach the room editor when `roomEditing.ready`. The chord toggle allows
  Creative on the room_editor surface, so pressing '2' switches the room-editor tool AND jumps
  the creative facade tool; one click both places geometry and changes creative selection.
  (Refuter note: reachability CONFIRMED and stronger than claimed.)

- **P2 [plausible] `src/app/iggy3d/window/CreativeViewportPickFrame.cpp:68` — menu-consumed
  clicks still reach the pick.** The pick frame only knows `downstreamClickSuppressed`
  (creative-UI suppression), never `higherPriorityUiConsumed`; a pause-menu click also records a
  `pick_hit` receipt. One logic refuter partially disagreed on the selection-change consequence;
  the contradictory-receipts consequence stands.

## Cluster C — mutation/projection semantics

- **F9 [major] `src/app/iggy3d/creative/SpatialProjection.cpp:170` — Move never moves a
  box-profile object's occupancy.** Point profile projects `transform.position`; Box/Volume/Line
  project `object.bounds`; `applyMoveMutation` writes ONLY position. A Move on any structural
  kind returns Applied/"object moved" while its projected cells — and pick hits — never change.
  Beyond K1/K2: the verb writes a real field, just not the one its profile projects.
  **Contract gap → ruled by new D8 (plan §1): placement mutations must move what the profile
  projects (Move translates bounds with position).**

- **F10 [major] `src/app/iggy3d/creative/SpatialProjection.cpp:727` — clampToGrid fabricates
  border occupancy for out-of-grid points.** Box/Volume clamp = intersection (fully-outside →
  EmptyProjection, correct); Point/Line clamp = RELOCATION component-wise into the grid, then
  status=Projected with a border cell the object does not inhabit. clampToGrid=true is both the
  struct default and the app request; no test pins it. Law: clamp must never fabricate
  occupancy — intersect or reject, uniformly across profiles.

- **F11 [major] `src/app/iggy3d/creative/DocumentMutation.cpp:154` — `message` consumed twice
  across indeterminately-sequenced arguments.** `rejectDocumentMutation` passes `message` by copy
  into one argument and `std::move(message)` into another of the SAME call; C++17 argument
  initialization is indeterminately sequenced, so on GCC (the box toolchain) every rejection
  receipt's `objectReceipt.message` is likely "" — compiler-dependent receipt content. Mac
  Clang and box GCC will disagree.

- **F12 [major] `src/app/iggy3d/creative/MutationApply.cpp:486` — applyAttachMutation has no
  no-change/self-attach/target-existence guard.** Re-attach to current parent = Applied +
  changed=true + revision bump (revision inflation breaks change-detection); self-attach is
  accepted. `applySetParentMutation` two functions up has the equality guard. **FIX IN FLIGHT**
  (uncommitted working-tree diff adds the guard).

- **P1 [plausible→fixed] `src/app/iggy3d/creative/MutationApply.cpp:106` — aliased verbs stamp
  the alias's kind + wrong dirtyFlags.** SetSpawnFacing fell through to applyRotateMutation
  (receipt kind=Rotate; Navigation|Gameplay dirty flags dropped); same for SetTriggerShape →
  SetBounds. **FIX IN FLIGHT** (uncommitted `applyRotateMutationAs`/`applySetBoundsMutationAs`
  preserve the requested kind).

## Cluster D — editor-state lifecycle (no invalidation laws)

- **F13 [CRITICAL] `src/app/iggy3d/creative/Facade.cpp:107` — tool switch desyncs the two
  measurement flags; Cancel goes permanently dead.** `setActiveTool` clears
  `toolState_.measurementActive` (the input gate) but never cancels `measurementState_` (the
  editor state); `cancelMeasurement`/`clearMeasurement` have zero callers outside Measure.cpp.
  After Measure→Select mid-measurement, the HUD reports an active measurement forever and no
  input can clear it (Cancel only dispatches when the INPUT gate is true) — only
  `Facade::reset()` recovers. Both refuters upheld with full mechanical traces.

- **F14 [major] `src/app/iggy3d/creative/Ghost.cpp:183` — ghost preview has no hide path.**
  `applyGhostToolIntent` handles only PreviewPointer; CancelToolAction, tool switches, and
  pointer-leave all leave ghost state untouched; `hideGhost()` has zero production callers. Once
  shown, the ghost panel renders forever, frozen at its last point with a stale sourceTool.

- **F15 [major] `src/app/iggy3d/creative/Facade.cpp:301` — removing the selected object leaves
  dangling TargetRefs everywhere.** `removeObject` never checks selection/inspection/state
  against the removed id; `clearSelection`/`clearInspection` have no external callers. The
  selection panel shows the phantom target ("visible=unknown"), stays clickable, and every click
  yields MissingObject — unclearable short of reset. NEW consequence beyond K3 (which is the
  missing receipt on this path). Law needed: a document removal must invalidate every editor ref
  to that id (interim guard now; structural fix rides W3's receipted remove).

## Cluster E — test health

- **F16 [major, DYNAMICALLY CONFIRMED] `tests/unit/product_creative_ui_projection_tests.cpp:185`
  — trunk suite is RED at HEAD.** 4H changed `rowText()` to append " visible=..." to Selected
  AND Inspected rows and updated three test files — but missed this one, whose exact-text pin
  still expects the pre-4H format. **Verified by build+run on the Mac 2026-07-03: ctest reports
  `product_creative_ui_projection_tests ... Failed` ("FAIL: facade inspected row").** The 4H
  commit violated the per-order full-ctest gate. Root cause: `populateInspectedFacade()` fixture
  copy-pasted across three files (fixture drift).

## Refuted (killed by verification — do not act on)

- InputFrame.cpp:1090 "clicks stale in relative-cursor mode" — refuted on capture-policy trace.
- InputFrame.cpp:1042 "hidden overlay row under pause journal mutates document" — refuted
  (pause gating holds on the traced path).

## Minors (17, UNVERIFIED — triage during the matching W-order, do not trust blindly)

- SpatialProjection.cpp:781 — Line projection rasterizes the AABB diagonal, not the rail.
- SpatialProjection.cpp:341 — unchecked double→int32 casts: UB for non-finite/huge coords
  (mutations accept unvalidated doubles upstream).
- SpatialProjection.cpp:355 — zero-extent boxes vanish only when cell-aligned.
- SpatialProjection.cpp:630 — Water/LavaVolume project as Volume with occupancyKind Unknown.
- CreativeViewportPickFrame.cpp:63 — `click_suppressed` status unreachable in live wiring.
- DocumentMutation.cpp:195 — ApplyFailed receipts stamped "…applied through object mutation
  pipeline".
- DocumentMutation.hpp:59 — `allowNoChange` option documented, never read.
- DocumentMutation.cpp:217 — batch `attemptedCount` lies when the batch stops early.
- MutationApply.cpp:434 — SetParent accepts parentId=0/self/nonexistent ids.
- MutationApply.cpp:384 — Resize accepts negative sizes → inverted bounds, Applied receipt.
- Snap.cpp:29 — isValidSnapSettings demands both axis steps > 0 even for masked-off axes.
- Ghost.cpp:163 — invalid snap settings: mutates state, changed=true + accepted=false.
- Snap.cpp:70 — unknown-bits axis mask reports snapped=true, snaps nothing.
- Ui.cpp:109 — status-row ghost-visibility flag ORs in a bit already present (no-op signal).
- Ui.cpp:97 — status row data0/data1 record constants, not model counts.
- product_vulkan_creative_ui_overlay_tests.cpp:41 — final "show it" stage pinned by
  is-not-empty smoke only.
- creative_facade_mutation_tests.cpp:69 — no negative test for removing a SELECTED object.

## Completeness critic (residual risk + process)

- **Lane contamination:** the planner's `docs/ai-lane-maximum.md` v1.8 hunk (A8b log, unrelated
  to creative) rode inside Codex commit `d063dc10` "3H creative ui" — violates the
  per-lane-commit rule; the destiny-doc's version-bump/mirror discipline can't see it.
- **Write-only receipt surface:** zero non-test readers exist for any of the ~90 new
  `creative_ui_*`/`creative_viewport_pick_*` receipt fields — the validate→receipt DNA currently
  has no reader (doctrine watch: leverage lives in the readers; schedule the first one).
- **Test harness blind spot:** all 24 new test mains chain assertions with `&&` — the first
  failure short-circuits the rest, so a multi-failure regression reports one symptom per binary.
- product_creative_ui_projection_receipt_tests.cpp:233-240 — circular pin (asserts receipt
  fields equal the producer's own runtime values, not expected literals).
- CMake wiring verified CLEAN (all 17 new lib sources + all 24 test targets registered).
- Nobody built/ran on the box toolchain (GCC/Vulkan-OFF) — F11 is exactly the class of defect
  that will behave differently there.
