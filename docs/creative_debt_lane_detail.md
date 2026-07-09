# Creative Debt Lane — Detailed (HEAD-verified)

Companion to `docs/creative_debt_lane.md` (the summary + ordering). This is the buildable detail for items
#1-#5: exact leak sites (file:line), concrete fix steps, builder slices, and exit tests — verified against HEAD
by swarm `wpfml49wv`. Where the summary doc's premises were stale, the corrections are inline below.
`Re-anchor line numbers at card time.` Items #6 (Vulkan guardrail) and #7 (hotspots) stay in the summary +
`docs/refactor_targets.md`.

---

## 1. Seal Facade leaks — no raw document mutation outside the Facade

*Verified against HEAD `138db2b0`. The debt-doc premise is largely correct but two of its named free functions are stale — corrections called out inline below.*

### Problem

The Facade (`src/app/iggy3d/creative/Facade.{hpp,cpp}`) is supposed to be the only door to `CreativeDocument` mutation, but three production callers reach past it. Grepping every mutating vector (`documentForPersistence()`, the `DocumentMutation` free fns, `applyDocumentMutation`, and `CreativeDocument::removeDocumentObject`) for sites **outside `Facade.cpp`** yields exactly this production leak set (test files excluded — they legitimately drive the lower layers directly):

**Leak A — path editing bypasses the Facade entirely (the real leak).**
`apps/iggy3d_creative/StandalonePathEditing.cpp` hand-computes new path-point vectors, then reaches through the persistence door straight into the raw mutation executor:
- `StandalonePathEditing.cpp:88-92` (`movePathObjectWithUndo`) — `cr::applyDocumentMutation(appState.facade.documentForPersistence(), objectId, cr::CreativeMutationKind::SetPatrolRoute, cr::makePathPointsPayload(afterPoints))`
- `StandalonePathEditing.cpp:169-173` (`movePathPointWithUndo`) — identical raw call for single-point moves.

The vector math (`translatePathPoints`, `movePathPoint` at `StandalonePathEditing.cpp:15-45`) and the undo-snapshot bracketing live in the **app helper**, not behind a command. `Facade.hpp` exposes **zero** path methods (`grep 'ath' Facade.hpp` = empty). These are the only two production callers of `applyDocumentMutation` and of `makePathPointsPayload`/`SetPatrolRoute` (`DocumentSection.cpp`/`DocumentWireframe.cpp` `pathPoints.push_back` hits are save/read serialization, not mutation). Contrast this with non-path moves, which already route correctly through `facade.dispatchToolInput(press/move/release)` → BeginMove/CommitMove (`main.cpp:522/532`, `StandaloneGizmo.cpp:107`). Path editing is the lone tool that never got a Facade command.

**Leak B — staged-document raw remove inside the bridge (borderline; not a live-document leak).**
`src/app/iggy3d/creative/bridge/UiCommandFrame.cpp:343-366` (`handleRemoveSelectedRoomShell`) copies the live document (`CreativeDocument stagedDocument = context.facade.document();`), calls raw `stagedDocument.removeDocumentObject(objectId)` in a loop (`:347`), then re-enters through `context.facade.installDocument(std::move(stagedDocument))` (`:366`). This does **not** mutate the Facade's live document — it goes back through the `installDocument` command — so it is a copy-mutate-reinstall, not a true bypass. But it is the one production site outside `Facade.cpp` that calls the raw `CreativeDocument::removeDocumentObject`, and there is no Facade atomic-multi-remove command to express "remove this shell's N children as one transaction." The sibling single-delete path is already clean (`UiCommandFrame.cpp:250-252` uses `context.facade.removeDocumentObject(...)`).

**False positive — `documentForPersistence()` in the save path is a READ, keep it.**
`src/app/iggy3d/creative/CreativeWorldOperations.cpp:368` does `request.document = &facade.documentForPersistence();` where `CreativeWorldSaveRequest::document` is a non-owning pointer consumed by `saveCreativeWorld` (`world/WorldService.hpp:56`). This is persistence *read*, which the debt doc explicitly permits ("persistence read stays"). No change — but the exit test must not flag it.

**Stale premises in the debt doc (correct at card-write time):**
- `removeDocumentObject` is **not** a `DocumentMutation` free function — it is a method on `CreativeDocument` (`document/Document.hpp:197-199`, `Document.cpp:635`) and is **already wrapped** by `Facade::removeDocumentObject` (`Facade.hpp:208-211`, `Facade.cpp:792-816`). No new single-remove command is needed.
- `moveDocumentObject`/`resizeDocumentObject` free fns exist (`DocumentMutation.hpp:158/188`) but have **no production caller** outside `Facade.cpp` (the move path uses `Facade.cpp:708`; all other hits are tests). They are not a live leak; the fix is to keep them from *becoming* one via the header-encapsulation step below.
- Path-point **add / delete** commands (called for in the doc) **do not exist as leaks today** — the app only translates a whole path and moves one existing point; there is no add/remove-point code anywhere. Add/delete are therefore *forward-looking* commands to define now (so the next tool can't hand-roll them), not existing leaks to reroute.

### Fix

Every document mutation goes through a Facade-owned command that returns a receipt. Concretely:

1. **Add path-editing commands to the Facade** (`Facade.{hpp,cpp}`), modeled on the existing `removeDocumentObject`/`createDocumentObject` command shape (validate → apply via the internal `applyDocumentMutation` on `document_` → return a `CreativeFacadeMutationReceipt`-style struct with revision before/after + `CreativeDocumentMutationStatus`):
   - `movePathObject(CreativeObjectId, CreativeVec3 delta)` — translate whole path.
   - `movePathPoint(CreativeObjectId, std::size_t pointIndex, CreativeVec3 delta)`.
   - `addPathPoint(CreativeObjectId, std::size_t insertIndex, CreativeVec3 position)` — forward-looking.
   - `deletePathPoint(CreativeObjectId, std::size_t pointIndex)` — forward-looking.
   Each reads the object's current `pathPoints` (`Object.hpp:185`), computes the new vector (moving `translatePathPoints`/`movePathPoint` from `StandalonePathEditing.cpp` into `Facade.cpp` as file-local helpers), and applies via `SetPatrolRoute` + `makePathPointsPayload` **internally** — the app never touches `applyDocumentMutation` again. Shape/descriptor guards (`shapeKind == Path`) that currently live in the app helper move into the command.

2. **Reduce `StandalonePathEditing.cpp` to orchestration.** `movePathObjectWithUndo` / `movePathPointWithUndo` keep their undo-snapshot bracketing (`pushUndoSnapshot`/`discardUndoSnapshot`) and SDL logging but call `appState.facade.movePathObject(...)` / `movePathPoint(...)` instead of `applyDocumentMutation(documentForPersistence(), ...)`. The `#include ".../mutation/Mutation.hpp"` and the direct `documentForPersistence()` use disappear from this TU.

3. **Add a Facade atomic multi-remove command** for the room-shell case: `removeDocumentObjects(std::span<const CreativeObjectId>)` returning a batch receipt, implemented as the copy-mutate-install transaction currently inlined in `UiCommandFrame.cpp:343-366`. Rewrite `handleRemoveSelectedRoomShell` to call it, deleting the local `stagedDocument` + raw `removeDocumentObject` loop. (Alternatively, if a batch command is judged out of scope for #1, leave B as-is and note it as sanctioned copy-mutate-install in `AGENTS.md` — but the raw `removeDocumentObject` call must then be explicitly whitelisted in the exit test.)

4. **Header-encapsulate the MutationApply handlers (the #5 dependency).** `mutation/MutationApply.hpp` currently exposes ~30 per-verb `applyXxxMutation(...)` helpers publicly (`MutationApply.hpp:99-137`). Verified: **no caller outside `MutationApply.cpp` uses any of them** (grep of all 30 names across `src/`/`apps/`/`tests/` = empty). Move every per-verb helper to file-local (anonymous namespace / `static`) in `MutationApply.cpp`; the header keeps only `applyMutation` (the two generic overloads), the status predicates (`mutationApplySucceeded`/`Failed`/`Changed`), `makeMutationApplyReceipt`, `rejectMutation`, and `toString`. This makes it *structurally impossible* for a future app helper to call an internal verb handler — it closes the door #1 is trying to lock. Zero-risk because there are no external callers today.

**Persistence read stays.** `documentForPersistence()` remains for the save path (`CreativeWorldOperations.cpp:368`); only mutation moves behind commands. Optionally rename it to signal intent (e.g. `documentForSave()`), but that is cosmetic and out of scope for sealing.

**Owner files.** `creative/Facade.{hpp,cpp}` (new path + batch-remove commands); `apps/iggy3d_creative/StandalonePathEditing.{hpp,cpp}` (reroute); `creative/bridge/UiCommandFrame.cpp` (batch-remove reroute); `creative/mutation/MutationApply.{hpp,cpp}` (encapsulate handlers). Read-only touch: `creative/document/DocumentMutation.{hpp,cpp}` (unchanged; the free fns stay but gain no new external caller).

### Slices

**Slice 1 — Facade path commands + reroute the two live leaks.**
Add `movePathObject`/`movePathPoint` to `Facade.{hpp,cpp}`; move the vector-math helpers into `Facade.cpp`; rewrite `StandalonePathEditing.cpp:88-92` and `:169-173` to call them.
*Acceptance:* `StandalonePathEditing.cpp` contains no `applyDocumentMutation` and no `documentForPersistence` (grep = 0); path-move capture scenario (`StandaloneCaptureScenario.cpp:495-540`, `movePathObjectWithUndo`/`movePathPointWithUndo`) still produces an `Applied` receipt with the same revision-delta behavior; suite green.

**Slice 2 — forward-looking add/delete path-point commands.**
Add `addPathPoint`/`deletePathPoint` to the Facade with their own receipt paths and descriptor/index guards, plus unit coverage in `tests/unit/creative_facade_*` (mirror `creative_document_path_tests.cpp` assertions).
*Acceptance:* new tests exercise add/insert-at-index and delete-at-index through the Facade (reject on non-Path kind, reject on out-of-range index, revision increments on change); no app or bridge code hand-rolls `pathPoints` insert/erase.

**Slice 3 — atomic multi-remove command + bridge reroute.**
Add `Facade::removeDocumentObjects(std::span<const CreativeObjectId>)`; rewrite `UiCommandFrame.cpp:343-366` to use it; delete the inline `stagedDocument` loop.
*Acceptance:* `UiCommandFrame.cpp` contains no `CreativeDocument stagedDocument` and no `stagedDocument.removeDocumentObject` (grep = 0); room-shell remove tests (`creative_room_shell_tests.cpp`) stay green with identical install-receipt outcomes.

**Slice 4 — encapsulate MutationApply verb handlers.**
Move the ~30 per-verb `applyXxxMutation` declarations out of `MutationApply.hpp` into file-local scope in `MutationApply.cpp`; header keeps only `applyMutation` + predicates + `makeMutationApplyReceipt`/`rejectMutation`/`toString`.
*Acceptance:* `MutationApply.hpp` declares no `apply*Mutation` symbol other than the two `applyMutation` overloads; full build + suite green (proves no hidden external caller).

### Exit test

A grep gate (add as a checked-in guard test or CI grep, mirroring the existing receipt key-order oracle discipline). All four must hold at HEAD:

1. **No raw document mutation from app/bridge helpers.** Outside `src/app/iggy3d/creative/Facade.cpp` and `tests/`, zero production hits for `applyDocumentMutation` or the `DocumentMutation` mutating free fns (`moveDocumentObject`, `resizeDocumentObject`, `rotateDocumentObject`, `renameDocumentObject`, `setDocumentObject*`, `assignDocumentObjectLayer`, `add/removeDocumentObjectTag`):
   ```
   grep -rn 'applyDocumentMutation\|moveDocumentObject\|resizeDocumentObject\|rotateDocumentObject' src/ apps/ \
     | grep -v 'creative/Facade.cpp' | grep -v 'document/DocumentMutation.'   # → 0 lines
   ```
2. **`documentForPersistence()` has no external *mutating* caller.** Its only production callers outside the Facade are read-only: `CreativeWorldOperations.cpp:368` (save request). Assert the grep set equals exactly `{CreativeWorldOperations.cpp}` (the two `StandalonePathEditing.cpp` hits are gone):
   ```
   grep -rln 'documentForPersistence' src/ apps/ | grep -v 'Facade.' | grep -v tests/   # → only CreativeWorldOperations.cpp
   ```
3. **Raw `CreativeDocument::removeDocumentObject` appears only in `Facade.cpp`, `Document.cpp`, and tests** (Slice 3):
   ```
   grep -rln '\.removeDocumentObject\|stagedDocument' src/app/iggy3d/creative/bridge/   # → 0 lines
   ```
4. **MutationApply header is closed** (Slice 4): `grep -c 'apply.*Mutation(' MutationApply.hpp` counts only the two `applyMutation` overloads; no per-verb handler is declared in the header. Full build success is itself proof no external caller depended on them.

Done when all four grep assertions pass and the suite is green — mutation has exactly one door (the Facade), and the internal verb handlers are structurally unreachable from app code.

---

## 2. Finish `apps/iggy3d_creative/main.cpp` decomposition — orchestration-only

### The doc's premise is partly stale at HEAD — correct it first

The debt doc claims a "~1651-line `main()` with a ~1160-line frame loop; stages inline, dozens of long-lived mutable locals." **All three numbers are wrong at HEAD, and most of the extraction is already done.** Verified against `apps/iggy3d_creative/main.cpp` (E230):

- The whole TU is **1086 lines** (`wc -l`), not 1651. `main()` starts at **line 145** (`int main`) and ends at **1086** — ~940 lines total.
- The frame loop is `while (window.isOpen())` at **line 351** through **line 1077** — **~726 lines**, not 1160.
- **`CreativeEditorState` already exists** (`apps/iggy3d_creative/CreativeEditorState.hpp`, 79 lines) and already owns nearly all the long-lived locals the doc wants moved: camera (`flyConfig`/`flyPos`/`yawDegrees`/`pitchDegrees`), place mode (`placeMode`/`brushPalette`/`placeBrush`/`placeCellSize`), gizmo drag anchors (`interactiveGrabbedAxis`/`interactiveGrabAnchorS`/`interactiveGrabCursorX/Y`/`interactiveGrabCenterScreen`/`interactiveGrabTipScreen`), path-move latches, key edge-latches (`prevKey1/2/3/B/F5/F6/F9/Delete/Backspace/Z`), `undoStack`, `captureScript`, and `frameIndex`. The "mutable-local soup" the doc describes was already drained into this struct.
- **6 of the 8 named stages already exist as free functions** over `CreativeEditorState` (verified signatures below). This item is NOT a greenfield decomposition — it is **finishing the last two stages** (move, overlay) plus a few residual locals.

So the item is real but ~75% complete. The finishable remainder is narrow and precise.

### Problem — the exact remaining leak

Two of the eight stages are still open-coded inline in the frame loop, and a handful of stage-scoped constants/locals still live in `main()` instead of on state:

1. **MOVE stage is fully inline** — `main.cpp:464–783` (~320 lines). Grep confirms **no** `applyCreativeEditorMove` / `runCreativeEditorMove` / `CreativeEditorMove*` helper exists anywhere in `apps/iggy3d_creative/`. This block hand-rolls: the capture-path axis-constrained move (`main.cpp:473–551`, keyed on `editor.frameIndex == 5/6/7`), the interactive path/path-point move (`main.cpp:589–660`), the `buildConstrainedDestination` lambda (`main.cpp:667–709`), and the interactive gizmo drag PRESS/MOVE/RELEASE latch machine (`main.cpp:711–762`). It is the single largest inline stage and the one that dispatches every `facade.dispatchToolInput(...)` + `dispatchMoveReleaseWithUndo(...)` call from `main`.

2. **OVERLAY stage is fully inline** — `main.cpp:785–1013` (~228 lines). Grep confirms **no** `buildCreativeEditorOverlay` / `CreativeEditorOverlay*` helper exists. This block builds, in one flat sequence: the inspector UI projection (`785–799`), the document wireframe + selection recolor (`801–823`), the combined gizmo/marker/path-handle wireframe vector `combinedWireLines` (`825–911`), the green placement ghost (`912–963`), the `combinedWireFrame` assembly (`964–968`), the dimension-label glyph merge (`970–998`), and the overlay attach to `frame.ui` / `frame.creativeWireframeDebug` (`1000–1012`). All of `documentWireLineCount` / `pointMarkerEdgeCount` / `lineMarkerEdgeCount` / `pathPointHandleEdgeCount` / `ghostEdgeCount` are computed here purely to feed the two `SDL_Log` diagnostics at `1015–1073`.

3. **Residual `main()`-scoped constants that belong to a stage, not to `main`** — `kGizmoAxisLength` / `kGizmoThickness` / `kGizmoHandleThresholdPx` at `main.cpp:317–321` are consumed only by the move and overlay stages (`main.cpp:443, 500, 689, 720, 908, 932, 959`). They are `main`-local constants read by would-be free functions, which forces the extracted stages to take them as parameters or re-declare them. They should be a shared `constexpr` header owned by the gizmo/overlay stage.

4. **The two big diagnostic `SDL_Log` blocks** (`main.cpp:1015–1038` "per-frame first-submit log" and `1040–1075` "FINAL frame log") are ~60 lines of formatting inside `main` that depend on overlay edge-counts. They read like a `submit` stage's receipt log but are wired to overlay-local counters.

The **selection** stage boundary is already clean (`resolveCreativeEditorSelectionFrame`, `CreativeEditorSelection.hpp:17`) but the **move** and **overlay** stages reach back into its outputs (`selection`, `selectedId`, `selected`, `hasSelection`, `selBoxMin/Max`, and the whole `gizmoFrame`) via loose locals — so extracting them cleanly requires passing the already-built `CreativeEditorSelectionFrame` + `CreativeEditorGizmoFrame` as inputs rather than the ~10 unpacked locals at `main.cpp:428–462`.

### The 8 stages as concrete function signatures

Six exist verbatim at HEAD; two are new. All are free functions in `namespace iggy3d_creative_app`, each in its own `.hpp/.cpp` pair matching the existing convention (`CreativeEditor<Stage>.{hpp,cpp}`).

**Already at HEAD (verified signatures — do not rewrite, just confirm they are the seam):**

1. **input** — `CreativeEditorFrameInput.hpp:22`
   `CreativeEditorFrameInputResult beginCreativeEditorFrameInput(SdlWindow& window, VulkanBackend& backend, CreativeEditorState& editor);`
2. **command** — `CreativeEditorCommandInput.hpp:12`
   `void applyCreativeEditorCommandInput(const bool* keys, bool captureMode, creative::CreativeAppState& appState, CreativeEditorState& editor, const std::filesystem::path& saveRoot, const std::string& saveId);`
3. **pick** — `CreativeEditorPickFrame.hpp:25`
   `[[nodiscard]] CreativeEditorPickFrame buildCreativeEditorPickFrame(...);`
4. **selection** — `CreativeEditorSelection.hpp:17`
   `[[nodiscard]] CreativeEditorSelectionFrame resolveCreativeEditorSelectionFrame(creative::CreativeFacade& facade);`
   (plus `applyCreativeEditorClickSelection`, `CreativeEditorClickSelection.hpp:14`, which the click half of this stage already uses)
5. **placement** — `CreativeEditorPlacementInput.hpp:11`
   `void applyCreativeEditorPlacementInput(SdlWindow& window, creative::CreativeAppState& appState, CreativeEditorState& editor, const Vec3& aimCellCenter, bool captureMode);`

**New — to add (the finishable work):**

6. **move** — new `apps/iggy3d_creative/CreativeEditorMove.{hpp,cpp}`:
```cpp
// Runs the entire Move stage: capture-path axis-constrained move (frames 5/6/7),
// interactive path / path-point move, and the interactive gizmo-drag PRESS/MOVE/
// RELEASE latch machine. Owns every facade.dispatchToolInput + dispatchMoveReleaseWithUndo
// call currently at main.cpp:464-783. All drag latches live on editor already.
void applyCreativeEditorMove(
    SdlWindow& window,
    creative::CreativeAppState& appState,
    const CreativeEditorSelectionFrame& selection,   // replaces loose selectedId/selected/hasSelection
    const CreativeEditorGizmoFrame& gizmo,           // replaces loose gizmoShafts/gizmoCenterScreen/gizmoTipScreen/gizmoAnchorS
    const FrameInput& frame,                          // for frame.camera (ground ray, ground point)
    const SdlDrawableExtent& extent,
    CreativeEditorState& editor,
    bool captureMode);
```

7. **overlay** — new `apps/iggy3d_creative/CreativeEditorOverlay.{hpp,cpp}`:
```cpp
// Builds all editor overlays for the frame and ATTACHES them to `frame`:
//   inspector UI rects+glyphs, document wireframe + selection recolor, the
//   combined gizmo/marker/path-handle wireframe vector, the placement ghost,
//   and the dimension-label glyphs. Returns the owned line/glyph vectors (they
//   must outlive submitFrame) plus the edge counts the submit log reads.
struct CreativeEditorOverlayResult {
  std::vector<RenderCreativeWireframeDebugLine> wireLines;  // MUST outlive submitFrame
  std::vector<DebugHudGlyphQuad> glyphs;                    // MUST outlive submitFrame
  RenderCreativeWireframeDebugFrame wireFrame;              // points into wireLines
  std::size_t documentWireLineCount = 0;
  std::size_t pointMarkerEdgeCount = 0;
  std::size_t lineMarkerEdgeCount = 0;
  std::size_t pathPointHandleEdgeCount = 0;
  std::size_t ghostEdgeCount = 0;
};

[[nodiscard]] CreativeEditorOverlayResult buildCreativeEditorOverlay(
    creative::CreativeAppState& appState,
    const CreativeEditorSelectionFrame& selection,
    const CreativeEditorGizmoFrame& gizmo,
    FrameInput& frame,                    // non-const: attaches ui + creativeWireframeDebug
    const SdlDrawableExtent& extent,
    const Vec3& aimCellCenter,
    const CreativeEditorState& editor);
```
Lifetime note (load-bearing): the returned `wireLines`/`glyphs` vectors, and the `wireFrame`/`frame.ui.*` pointers into them, must stay alive until `backend->submitFrame(frame)` returns. `main` holds the `CreativeEditorOverlayResult` by value in the loop body so it outlives submit — exactly as the current inline `combinedWireLines`/`glyphs` locals do at `main.cpp:832, 973`.

8. **submit** — new `apps/iggy3d_creative/CreativeEditorSubmit.{hpp,cpp}`:
```cpp
// Runs frustum cull, submits the frame, and emits the first-submit + FINAL
// diagnostic logs (main.cpp:1009-1075). Returns the submit result so main can
// decide loop exit.
struct CreativeEditorSubmitResult { RenderSubmitResult submit; bool didFinalLog = false; };

CreativeEditorSubmitResult submitCreativeEditorFrame(
    VulkanBackend& backend,
    FrameInput& frame,
    const CreativeEditorSelectionFrame& selection,
    const CreativeEditorOverlayResult& overlay,
    const StandaloneRoomBakePreviewScene& roomBakePreview,
    CreativeEditorState& editor,
    std::uint64_t maxFrames);
```

### New CreativeEditorState fields (the last locals it must own)

`CreativeEditorState` (`CreativeEditorState.hpp`) already owns almost everything. Only these frame-loop locals currently live in `main()` and must migrate so the extracted stages don't take them as loose parameters:

- **Nothing move-specific is missing** — every move latch (`moveDragButtonDown`, `interactiveGrabbedAxis`, `interactiveGrab*`, `interactivePath*`) is already a field. Move needs **only** the gizmo constants (below) to stop being `main`-local.
- **`lastWidth`/`lastHeight`** (`CreativeEditorState.hpp:75-76`) exist but are currently unread in `main`'s loop — confirm the input stage owns resize detection or delete them (dead-field check belongs to the exit test).

The gizmo constants at `main.cpp:317-321` should NOT become `CreativeEditorState` fields (they are compile-time constants, not per-frame state). Put them in a new shared header:

```cpp
// apps/iggy3d_creative/CreativeEditorGizmoConstants.hpp
namespace iggy3d_creative_app {
inline constexpr float kGizmoAxisLength = 1.5F;
inline constexpr float kGizmoThickness = 0.05F;
inline constexpr float kGizmoHandleThresholdPx = 35.0F;
}
```
The move stage, overlay stage, and the already-extracted `buildCreativeEditorGizmoFrame` (`CreativeEditorGizmoFrame.cpp`, which currently takes `kGizmoAxisLength` as a param at `main.cpp:443`) all include it; `main` no longer declares them.

### Slices (builder-sized, in order)

**Slice 2a — extract the OVERLAY stage.** Add `CreativeEditorGizmoConstants.hpp` and `CreativeEditorOverlay.{hpp,cpp}`. Move `main.cpp:785–1012` verbatim into `buildCreativeEditorOverlay`, returning `CreativeEditorOverlayResult`. Replace the inline block in `main` with one call + hold the result by value. Rewire the two `SDL_Log` blocks (`main.cpp:1015–1073`) to read `overlay.documentWireLineCount` etc.
*Acceptance:* `apps/iggy3d_creative/main.cpp` shrinks by ~225 lines; capture run (`--capture`) produces a byte-identical PNG to pre-slice and identical `combinedWireLines=`/`gizmoLines=`/`ghostEdges=` values in the FINAL log. (Overlay is extracted first because it has no control flow — pure build-and-attach — so it is the safest cut and de-risks the constants header.)

**Slice 2b — extract the MOVE stage.** Add `CreativeEditorMove.{hpp,cpp}`. Move `main.cpp:464–783` into `applyCreativeEditorMove`, taking `CreativeEditorSelectionFrame` + `CreativeEditorGizmoFrame` instead of the unpacked locals at `main.cpp:430–462`. The `buildConstrainedDestination` lambda becomes a file-local `static` helper in the `.cpp`.
*Acceptance:* `main.cpp` shrinks by ~320 more lines; capture run's `BEFORE`/`AFTER`/`RELEASE` move-dispatch logs and the floor's post-move `AFTER` position are unchanged; interactive Alt+drag gizmo move still commits one Move mutation (manual smoke or existing standalone test).

**Slice 2c — extract the SUBMIT stage + collapse `main`.** Add `CreativeEditorSubmit.{hpp,cpp}` wrapping `main.cpp:1009–1075` (frustum cull + `submitFrame` + both diagnostic logs). After this, the frame-loop body of `main` is: `input → command → build scene/frame → pick → selection → placement → move → gizmoFrame → overlay → submit`, each one call. Delete now-dead `using`-declarations from the `main.cpp` anonymous namespace (`main.cpp:88–142`) that only the extracted stages needed.
*Acceptance:* the `while (window.isOpen())` loop body in `main.cpp` is ≤ ~40 lines of stage calls (no `for`/`if`/lambda control flow except the frame-skip guard); `main()` total ≤ ~450 lines; full standalone capture proof + suite green.

### Exit test

The item is done when all four grep/count assertions pass at HEAD:

1. **All 8 stages are named free functions.** Each of these greps returns ≥1 hit in `apps/iggy3d_creative/`:
   `grep -rl 'beginCreativeEditorFrameInput'`, `'applyCreativeEditorCommandInput'`, `'buildCreativeEditorPickFrame'`, `'resolveCreativeEditorSelectionFrame'`, `'applyCreativeEditorPlacementInput'`, **`'applyCreativeEditorMove'`**, **`'buildCreativeEditorOverlay'`**, **`'submitCreativeEditorFrame'`**. (The last three are the new ones; the first five already pass.)

2. **No stage logic is inline in `main`.** These currently-inline symbols no longer appear in `main.cpp`:
   `grep -c 'buildConstrainedDestination\|combinedWireLines\|dispatchToolInput\|dispatchMoveReleaseWithUndo\|documentWireLineCount' apps/iggy3d_creative/main.cpp` → **0**. (All are >0 today.)

3. **The frame-loop body is orchestration-only.** The line span from `while (window.isOpen())` to its closing brace is ≤ ~60 lines, and contains no `if (editor.frameIndex ==` , no `[&](` lambda, and no `for (` over document objects:
   `awk '/while \(window.isOpen/,/^  }$/' apps/iggy3d_creative/main.cpp | grep -c 'frameIndex ==\|\[&\](\|for (const creative'` → **0**.

4. **Gizmo constants left `main`.** `grep -c 'constexpr float kGizmo' apps/iggy3d_creative/main.cpp` → **0** (they live in `CreativeEditorGizmoConstants.hpp`).

Plus the behavioral gate: `iggy3d_creative --capture <path>` produces the same FINAL-frame log line values (`selectedTarget`/`selectedKind`/`combinedWireLines`/`gizmoLines`/`ghostEdges`/`placed`/`objectCount`) as pre-decomposition, and the standalone/ctest suite stays green.

*(Depends on #1 — the move stage's `dispatchToolInput`/`dispatchMoveReleaseWithUndo` calls and the path-move `movePathObjectWithUndo`/`movePathPointWithUndo` calls at `main.cpp:624,647` should already route through Facade commands before this item locks, or 2b re-homes raw-mutation calls into a new TU and inherits the leak.)*

---

## 3. Stabilize save/delete/undo/redo ownership — ONE command + ONE receipt per op

*Verified against HEAD `138db2b0`. The debt-lane doc's premise is directionally right but its specifics are stale in two ways, corrected below: (a) **there is no redo at all** in the creative-document lane — "redo added late" is false; the only `redo` in the tree is the unrelated legacy `room_editor/AuthoringController` lane. (b) The real incoherence is not "the sd-series left multiple save entries" — save has one codec owner. It is that **the two apps drive undo by two different strategies**, and the undo-clear + mirror bookkeeping is smeared across five files.*

### Problem — the path is not single-coherent (file:line evidence)

**Two undo drivers with incompatible strategies.** The undo stack type is shared (`CreativeDocumentUndoStack`, `src/app/iggy3d/creative/CreativeAppState.hpp:59`), but *when a snapshot is pushed* differs per app:

- **Product app — inference/"recomputed" push.** `src/app/iggy3d/window/InputFrame.cpp:1312-1326` reconstructs "did a mutation happen?" *after* the whole input frame by diffing `revisionBefore`/`revisionAfter` against the same `documentId`, then retroactively pushes the pre-frame snapshot (`pushCreativeUndoSnapshot`, line 1324) — but **only** when `!undoCommandApplied` (1321) and `documentId` is unchanged (1314-1315). This is exactly the "recomputed undo" the doc names. Consequence: any mutation that swaps the document (a full `installDocument`) is invisible to undo, because the guard requires an equal `documentId`.
- **Standalone app — explicit per-op wrappers.** Each mutating op pushes *before* it runs and rolls back on no-op: `deleteSelectedObject` (`apps/iggy3d_creative/StandaloneDelete.cpp:46` push, `:48` remove), `movePathObjectWithUndo` (`StandalonePathEditing.cpp:87` push, `:95` discard-on-noop), `movePathPointWithUndo` (`:168` push, `:176` discard), `dispatchMoveReleaseWithUndo`. These are wired directly from the frame loop (`apps/iggy3d_creative/main.cpp:542,624,647,757`) and the key handler (`CreativeEditorCommandInput.cpp:64,69`).

So "delete then undo" runs through **two entirely different code paths** depending on which binary you launch, and the product path can silently fail to record an undoable step that the standalone path records fine.

**Two command entry points per op, no shared dispatcher.**
- *Delete*: product routes through the bridge dispatcher `handleDeleteSelectedObject` (`src/app/iggy3d/creative/bridge/UiCommandFrame.cpp:236-263`, emitting `ProductCreativeUiCommandFrameReceipt`); standalone routes through `deleteSelectedObject` (`StandaloneDelete.cpp:15`, emitting a bare `CreativeDocumentRemoveReceipt`). Two entries, two receipt shapes.
- *Undo*: product = `handleUndoLastDocumentChange` (`UiCommandFrame.cpp:224-234`); standalone = `undoLastSnapshot` (`StandaloneUndo.hpp:44`). Both ultimately call `applyLastCreativeUndoSnapshot` (`CreativeAppState.hpp:126`), but each re-wraps it in its own receipt/log.
- *Save*: both call the same codec owner `saveCreativeWorld`, but via **two command wrappers** — `saveProductCurrentCreativeWorld` (`CreativeWorldOperations.cpp:325`, reached from the pause flow `save/Flow.cpp:98` ← `ActionHandlers.cpp:324,331`) and `saveStandaloneScene` (`StandalonePersistenceProof.cpp:90`, reached from `CreativeEditorCommandInput.cpp:76`). Save is single-owner at the codec, **two-owner at the command**.
- *Load*: `openCreativeWorld` + `installDocument` is duplicated — product `CreativeWorldOperations.cpp:272,290` vs standalone `StandalonePersistenceProof.cpp:117,126`.

**Undo-clear + mirror bookkeeping is smeared across 5 files.** The "reset undo + mirror" triple (`clearCreativeUndoStack` + `creativeAuthoring.creativeUndo.available=false` + `.depth=0`) is copy-pasted at `CreativeWorldOperations.cpp:230-232, 298-300, 394-396`, `menu/ActionHandlers.cpp:341-343`, and mirror-only at `InputFrame.cpp:1338-1344`. There is no single "undo store owner" that owns clear-on-save/clear-on-load/clear-on-new; every op site open-codes it.

**One incoherence worth flagging: the staged-install delete bypasses undo entirely.** The room-shell remove path stages a document copy, removes objects on the copy, and `installDocument`s it (`UiCommandFrame.cpp:343-366`). Because it swaps the document (not an in-place revision bump on the same `documentId`), the product's inference-based undo guard (`InputFrame.cpp:1314-1315`) never records it — this multi-object removal is **not undoable** in the product app today. This is precisely the kind of op area-select/batch will multiply.

### Fix — ONE command + ONE receipt + ONE owner per op

1. **Make undo push a Facade responsibility, not an app inference.** Add a single owner that pushes the pre-mutation snapshot *inside* every Facade mutation command (delete / move / resize / path-edit / staged-install), so the snapshot is captured at the mutation, identically for both apps. Delete the inference block at `InputFrame.cpp:1312-1326` and the per-op `*WithUndo` wrappers (`StandaloneDelete.cpp`, `StandalonePathEditing.cpp`) once the Facade owns it. Net: `pushCreativeUndoSnapshot` has exactly one caller (the Facade command layer), zero app callers.
2. **One command dispatcher for both apps.** Route the standalone key handler (`CreativeEditorCommandInput.cpp` delete/undo/save/load) through the *same* `ProductCreativeUiCommand*` dispatch used by `UiCommandFrame.cpp`, so `handleDeleteSelectedObject` / `handleUndoLastDocumentChange` are the single delete/undo entries and emit the single `ProductCreativeUiCommandFrameReceipt`. Retire `StandaloneDelete.{hpp,cpp}`, the `*WithUndo` helpers, and the bespoke `undoLastSnapshot` logging path in `StandaloneUndo.hpp`.
3. **One save command, one load command.** Collapse `saveStandaloneScene`/`loadStandaloneScene` (`StandalonePersistenceProof.cpp:90,111`) to thin adapters over `saveProductCurrentCreativeWorld` / a new `loadCreativeWorldIntoApp` in `CreativeWorldOperations.cpp` (which already owns `openCreativeWorld`+`installDocument` at :272,290). One codec call, one save receipt, one load receipt.
4. **One undo-store owner for the reset+mirror.** Add a single `resetCreativeUndoStore(creativeApp, window)` (owner: the creative undo store next to `CreativeAppState.hpp`) that does clear-stack + mirror-write in one place; replace all five smeared sites (`CreativeWorldOperations.cpp:230,298,394`; `ActionHandlers.cpp:341`; `InputFrame.cpp:1338-1344`) with a call to it.
5. **Fix the staged-install undo gap** by having the Facade command in step 1 push the snapshot around `UiCommandFrame.cpp:343-366`, so the room-shell/batch remove becomes undoable (and area-select inherits it for free).
6. **Redo is out of scope but name the socket.** There is no creative redo today; do not add it in this item. Reserve the redo cursor on `CreativeDocumentUndoStack` (a `redoDocuments` vector or a stack index) so a later redo slice has a single owner instead of re-deriving one — mirroring, but not copying, the legacy `room_editor` redo.

### Slices (builder-sized, in order)

- **S1 — Single undo-store owner (mirror + reset).** Add `resetCreativeUndoStore(creativeApp, window)` and route all clear+mirror sites through it. *Acceptance:* `grep -rn 'creativeAuthoring.creativeUndo.available *=' src/` returns exactly the owner body; `clearCreativeUndoStack` has ≤1 direct caller outside that owner; suite green.
- **S2 — Move undo push into the Facade mutation commands.** Push/rollback snapshot inside delete/move/resize/path/staged-install Facade commands; delete `InputFrame.cpp:1312-1326` and the standalone `*WithUndo` wrappers. *Acceptance:* `pushCreativeUndoSnapshot` + `discardCreativeUndoSnapshot` have callers only under `creative/` (no `apps/`, no `window/InputFrame.cpp`); a new test proves the room-shell staged-remove is undoable.
- **S3 — One dispatcher for delete + undo across both apps.** Standalone delete/undo keys call the shared `ProductCreativeUiCommand` dispatch; retire `StandaloneDelete.{hpp,cpp}` and the bespoke `undoLastSnapshot`. *Acceptance:* `grep -rn 'deleteSelectedObject\|undoLastSnapshot' apps/` = 0; both apps' delete/undo produce a `ProductCreativeUiCommandFrameReceipt`.
- **S4 — One save + one load command.** `saveStandaloneScene`/`loadStandaloneScene` become adapters over the product save/load commands. *Acceptance:* `grep -rn 'saveCreativeWorld(' src/ apps/` shows a single owner call each; standalone F5/F9 and product pause-save share the same receipt type; persistence tests green.

### Exit test (grep/assert that proves it done)

```
# ONE undo push owner (Facade only), zero app/inference callers:
grep -rn 'pushCreativeUndoSnapshot\|discardCreativeUndoSnapshot' src/ apps/ \
  | grep -v 'src/app/iggy3d/creative/' ; test $? -eq 1   # no matches outside creative/

# ONE delete + ONE undo command entry (no standalone alternates):
grep -rn 'deleteSelectedObject\|undoLastSnapshot' apps/ ; test $? -eq 1

# undo-clear + mirror lives in ONE owner (no smeared triples):
grep -rn 'clearCreativeUndoStack' src/ apps/ | grep -v 'resetCreativeUndoStore\|CreativeAppState.hpp' ; test $? -eq 1

# save has ONE codec call per command wrapper, ONE load owner:
grep -rn 'saveCreativeWorld(' src/ apps/    # each site is an adapter over the one command
grep -rn '\.installDocument(' apps/ ; test $? -eq 1   # standalone load/new go through the shared command
```
Plus an assert-level test: after `delete` then `undo` in *both* the product frame path and the standalone path, `objectCount` returns to the pre-delete value and the receipt is a `ProductCreativeUiCommandFrameReceipt` with `commandKind == UndoLastDocumentChange` — i.e. one path, one receipt, provable from a single test fixture. And a regression test that the room-shell staged-remove (`UiCommandFrame.cpp:343-366`) is undoable.


---

## 4. Validation + indexing seams

> **Verified against HEAD `138db2b0`.** The doc's premise is *half wrong at HEAD* — see the correction note under Problem. The item is still real; its scope has narrowed.

### Problem — the exact leak

Two seams are missing where the doc claims two are missing, but the truth at HEAD is asymmetric:

**(a) There is no `CreativeValidator`.** Confirmed: `grep -rn CreativeValidator src apps` → 0 hits; the only validators in the tree are `content/PackageValidator` and `render/vulkan/DebugValidation` (unrelated). So every consumer answers "is this object selectable / editable / bakeable?" with its own hand-rolled predicate, and **the three answers disagree**:

- **World-space pick** (`apps/iggy3d_creative/CreativeEditorPickFrame.cpp:19-22`): a candidate is pickable iff `obj.visible`. Nothing else — a locked object is fully selectable; a `Room` metadata object is selectable; an `isEditorOnly` descriptor is selectable.
- **Bake** (`src/app/iggy3d/creative/adapters/RoomBake.cpp:463-516`, `classifyRoomBakeObject`): "bakeable" is a *different* predicate stack — `!visible && !includeHidden → SkipHidden`, `descriptor.isEditorOnly → SkipEditorOnly`, `kind==Room → SkipRoomMetadata`, then shape/anchor/`hasBounds`/`validBakeBounds` gates. None of this logic is reachable by the picker or the selection layer.
- **Editability** (`src/app/iggy3d/creative/mutation/MutationApply.cpp:402`): "editable" is yet a *third* predicate — `options.rejectLockedObjects && object.locked && kind != SetLocked → LockedObject`, plus `descriptorAllowsMutation(kind, mutationKind)` at line 406. This is the only place `locked` is honored; the picker (which feeds selection, which feeds the gizmo/move) never consults it. So the editor will happily select and drag-target a locked object, and the rejection only surfaces deep inside apply.

The three "visible?" gates are literally spelled three different ways in three files — proof there is no single seam:
```
world pick   CreativeEditorPickFrame.cpp:20   if (!obj.visible) continue;
grid project SpatialProjection request        includeAuthoringOnly flag (opt-in)
bake         RoomBake.cpp:467                  if (!object.visible && !includeHidden) SkipHidden
```

**(b) An AABB spatial index already EXISTS — the doc is stale here.** `src/core/spatial/AabbGridIndex.{hpp,cpp}` is a real broadphase (grid-of-cells, `query`/`queryChecked` returning a superset, `insert`/`remove`/`rebuildFrom`, `AabbGridItem`, stats). Its own header comment (`AabbGridIndex.hpp:49`) already prescribes the intended lifecycle: *"insert()/remove() on dirty-flag deltas, rebuildFrom() [at snapshot boundaries]."* **But it is wired nowhere durable.** `grep -rln AabbGridIndex src apps tests` → only `core/spatial/*` (self), `tests/unit/aabb_grid_index_tests.cpp`, and `apps/iggy3d_creative/StandalonePicking.cpp`. And that one consumer misuses it: `StandalonePicking.cpp:99` constructs a **local** `AabbGridIndex index;` and `:108` calls `index.rebuildFrom(indexedItems)` **from scratch on every single pick**, over an `indexedItems`/`candidates` list that was *already* produced by a full `document.objects()` linear scan in `buildCreativeEditorPickFrame` (`CreativeEditorPickFrame.cpp:19`). So the index saves nothing — the O(n) scan already happened one layer up, then we build a throwaway index to broadphase the same n items for one ray. `insert`/`remove` (the incremental path the header promises) have **zero callers outside the index's own tests**.

So the real state at HEAD:
- **No index at all** on the two paths that most need it: **bake** re-scans `document.objects()` every bake (`RoomBake.cpp:698`), and the **grid-projection pick** re-projects every object every click (`projectObjectsToGrid`, `bridge/ViewportPickFrame.cpp`). Neither touches `AabbGridIndex`.
- **A throwaway index** on the world-space pick that doesn't amortize because it's rebuilt per-frame after a full scan.
- **No persistent, document-owned, mutation-maintained index anywhere.** `grep -niE 'aabbgrid|gridindex|spatialindex|occupancyindex' Facade.hpp Document.hpp` → 0 hits. Neither `Facade` nor `CreativeDocument` owns a spatial index. (`CreativeDocument` *does* own `objectIndex_` — an `unordered_map<CreativeObjectId, size_t>` at `Document.hpp:233` — but that is an **id→slot** map for `findObject`, `Document.cpp:499-514`; it is not spatial and answers no bounds query.)

**Net:** the doc's "(a) add a validator" is fully valid. The doc's "(b) add an AABB index (the occupancy-index spine)" is **already-built-but-unadopted**, not missing — the card is *adopt + persist + fan out the existing `AabbGridIndex`*, not *write a new one*.

### Fix

**(a) Introduce `creative::CreativeValidator`** — new `src/app/iggy3d/creative/CreativeValidator.{hpp,cpp}`. A stateless (or document-borrowing) seam whose questions are the union of the three predicate stacks above, each returning a reasoned status enum so callers can message uniformly. The questions it answers, per `const CreativeObject&` (+ its `describeObject(kind)` descriptor):

- `SelectableStatus selectability(obj)` — `Ok` / `Hidden` (`!obj.visible`) / `Locked` (`obj.locked`) / `MetadataOnly` (`kind==Room`) / `EditorOnly` (`descriptor.isEditorOnly`). This centralizes what `CreativeEditorPickFrame.cpp:20` does by hand.
- `EditableStatus editability(obj, mutationKind)` — wraps the exact `MutationApply.cpp:402-408` logic: `Ok` / `Locked` / `UnsupportedMutation` (`!descriptorAllowsMutation`). `MutationApply` then *calls the validator* instead of re-inlining the checks (the inline checks in `applyMutation` become one call; keep the same reject receipts/messages so tests don't move).
- `BakeableStatus bakeability(obj, includeHidden)` — lift `classifyRoomBakeObject`'s **eligibility** front-half (`RoomBake.cpp:467-501`: Hidden / EditorOnly / RoomMetadata / UnsupportedAnchor / UnsupportedShape / NoBounds). RoomBake keeps its geometry/role computation but sources the *decision* from the validator so "what bakes" has one definition.

Keep it pure facts-in → status-out; it *reads* the descriptor registry (item #5) and never mutates. It must not include Vulkan/render headers (item #6).

**(b) Adopt + persist the existing `AabbGridIndex` as a document-owned spatial spine.** Do **not** write a new index — wire `src/core/spatial/AabbGridIndex`.

- Give the ownership to the document/facade layer: a `creative::CreativeSpatialIndex` wrapper (thin, in `creative/spatial/`) holding one `AabbGridIndex` keyed by `CreativeObjectId`, plus a `rebuildFrom(document)` and the `insert`/`remove` deltas the header already promises.
- Maintain it off the **mutation receipt** already flowing through the Facade (create/move/resize/remove change bounds; the receipts carry `objectId` + before/after — e.g. `Facade.cpp:271,299`): on applied create/move/resize call `insert`/`remove`; on load/undo/redo snapshot boundaries call `rebuildFrom`. This finally exercises the `insert`/`remove` path that currently has no non-test caller.
- **Fan out to all three consumers** so none carries its own bounds scan:
  - **World pick**: `StandalonePicking.cpp` queries the *document's* index (via facade) instead of building a local one at `:99`/`:108`; the full `document.objects()` scan in `CreativeEditorPickFrame.cpp:19` is replaced by a ray-broadphase query → narrow.
  - **Grid-projection pick**: `bridge/ViewportPickFrame.cpp` broadphases against the index before `projectObjectsToGrid`, projecting only candidates near the pointer ray rather than the whole document.
  - **Bake**: `RoomBake.cpp:698` keeps iterating for the full bake (bake is all-objects by nature), but the index becomes the source for *area/region* bake queries and reachability neighbor lookups — the point is the query API exists so a future area-select bakes a region without a new scan.

### Slices (build order)

**S4.1 — `CreativeValidator` seam + selectability adoption.**
Add `creative/CreativeValidator.{hpp,cpp}` with `selectability()`, `editability()`, `bakeability()` returning reasoned enums. Route the **selection/pick** gate through `selectability()`: replace `CreativeEditorPickFrame.cpp:20`'s bare `!obj.visible` with `validator.selectability(obj) == Ok`, and make selection reject locked / metadata / editor-only up front.
*Acceptance:* new `tests/unit/creative_validator_tests.cpp` pins all three question functions against every `CreativeObjectKind`; a locked object no longer becomes the active selection (picker skips it); existing pick tests stay green.

**S4.2 — Fold editability + bakeability into their owners.**
`MutationApply.cpp:402-408` calls `validator.editability(...)` instead of inlining the locked/`descriptorAllowsMutation` checks (same reject receipts/messages). `RoomBake.cpp:467-501`'s eligibility front-half calls `validator.bakeability(...)`; the `RoomBakeObjectDecision` mapping stays but is derived from the validator status.
*Acceptance:* `creative_document_mutation_tests` + `creative_document_room_bake_tests` unchanged and green; grep shows the locked/editor-only/`isEditorOnly` string literals now live once (in the validator), not re-spelled in MutationApply and RoomBake.

**S4.3 — Document-owned `CreativeSpatialIndex` (persistent, receipt-maintained).**
Add `creative/spatial/CreativeSpatialIndex.{hpp,cpp}` wrapping `core/spatial/AabbGridIndex`; give `Facade` (or `CreativeDocument`) an instance; maintain it via `insert`/`remove` on applied create/move/resize receipts and `rebuildFrom` on load/undo/redo.
*Acceptance:* new test asserts the index tracks the document across a create→move→delete→undo sequence (query returns the object where its bounds are, and stops after delete); `insert`/`remove` now have a non-test production caller.

**S4.4 — Fan the index out to pick paths; delete the per-frame throwaway.**
`StandalonePicking.cpp` broadphases the document index (remove the local `AabbGridIndex index; index.rebuildFrom(...)` at `:99`/`:108`); `bridge/ViewportPickFrame.cpp` broadphases before projecting.
*Acceptance:* `StandalonePicking.cpp` no longer constructs a local `AabbGridIndex`; world-pick and grid-pick tests green; a pick over N objects issues one broadphase query, not one full `document.objects()` bounds scan (assert candidate count < object count in a spread-out fixture).

### Exit test

```
# 1. Validator is the single seam — no consumer carries its own selectable/editable/bakeable predicate:
grep -rn "CreativeValidator" src/app/iggy3d/creative apps/iggy3d_creative | grep -qE "selectability|editability|bakeability"   # must hit pick + mutation + bake
grep -rn "!obj.visible\|!object.visible" apps/iggy3d_creative/CreativeEditorPickFrame.cpp   # must be 0 (gate moved into validator)

# 2. The AABB index is document-owned and receipt-maintained, not per-frame throwaway:
grep -rn "AabbGridIndex index;" apps/iggy3d_creative/StandalonePicking.cpp   # must be 0 (local rebuild gone)
grep -rn "\.insert(\|\.remove(" src/app/iggy3d/creative/spatial/CreativeSpatialIndex.cpp   # must be >0 (incremental path live)
grep -niE "spatialindex|aabbgrid" src/app/iggy3d/creative/Facade.hpp        # must be >0 (facade owns it)
```
Plus: `ctest` passes with the four new/updated suites (`creative_validator_tests`, `creative_spatial_index_tests`, mutation, room_bake). Item is done when pick, selection, mutation, and bake all answer "selectable/editable/bakeable" via `CreativeValidator` and all query one document-owned `AabbGridIndex` — with **zero** ad-hoc `!visible`/locked predicates and **zero** local per-frame index rebuilds left in the tool/pick/bake code.


---

## 5. Separate registry facts from mutation behavior

*Verified against HEAD `138db2b0`. The doc's premise is correct but its owner-file list is slightly off: the bleed is **not** symmetric. `Mutation.cpp` reading registry facts is CLEAN (that is the intended direction). The actual leak is the reverse — two mutation-**policy** functions live inside the registry TU `ObjectDescriptor.cpp`, and `MutationApply.cpp` is not a source of bleed, only a consumer. `MutationApply.cpp` in the doc's grep list contains none of the offending definitions.*

### Problem — the exact leak

The registry translation unit `document/ObjectDescriptor.cpp` owns two functions that are pure **mutation policy**, not facts. Both take a `CreativeMutationKind` and encode behavioral rules:

- **`descriptorAllowsMutation(objectKind, mutationKind)`** — `ObjectDescriptor.cpp:2012-2036`. This is the mutation-legality rule. It calls `canMutate()` (`Mutation.cpp:768`), `isTransformMutation()` / `isShapeMutation()` (`Mutation.cpp:659,663`), and hard-codes editorial policy including the TD-2 special case verbatim in a comment + branch: *"Move stays legal for bounds-only kinds because it places the corner anchor"* (`ObjectDescriptor.cpp:2019-2033`, e.g. `if (mutationKind != CreativeMutationKind::Move || !descriptor.hasBounds) return false;`). A tool decision ("may I apply this verb?") is answered from the noun table.

- **`dirtyFlagsForMutation(objectKind, mutationKind)`** — `ObjectDescriptor.cpp:1926-1968`. This maps a verb to invalidation channels. It dispatches on `isTransformMutation`/`isShapeMutation`/`isLogicMutation`/`isNavigationMutation`/`isTestingMutation`/`isSensoryMutation`/`isGameplayMutation` (all defined in `Mutation.cpp:659-697`) and hard-codes per-verb policy (`ObjectDescriptor.cpp:1963`: `SetVisible`/`SetLocked`/`Rename` → `Identity|Preview`).

Concrete evidence the two lanes are cross-included in a cycle:
- `mutation/Mutation.cpp:5` — `#include ".../document/ObjectDescriptor.hpp"` (mutation reads registry — **correct direction**: `allowedMutations` at `Mutation.cpp:757` and `appendDescriptorProfileMutations` at `Mutation.cpp:188` dispatch on `descriptor.profile`/`category`/`shapeKind`/`canOwnChildren`, all pure facts).
- `document/ObjectDescriptor.hpp:5` — `#include ".../mutation/Mutation.hpp"` (registry pulls the verb vocabulary in so it can host policy — **the leak**).
- The header even advertises the leak as intent: `ObjectDescriptor.hpp:18-19` claims the file defines *"which mutation rules should apply."* It should not.

Consumers of the leaked policy that prove it is behavior, not facts:
- `MutationApply.cpp:35` calls `dirtyFlagsForMutation(...)` to build the applied receipt.
- `MutationApply.cpp:406` calls `descriptorAllowsMutation(...)` gated on `options.validateDescriptorRules` — i.e. this IS the apply-time validation gate.
- The policy is even pinned by the **registry's own** test file: `tests/unit/creative_object_descriptor_tests.cpp:401-458` (allow/deny assertions) and `:907-922` (dirty-flag assertions) — mutation behavior asserted from a descriptor test.

Net: `ObjectDescriptor.{hpp,cpp}` is 2038 lines of noun table plus two policy functions welded on; the enum↔string tables and default bounds are genuinely facts and stay. The circular include (`Mutation.hpp` ↔ `ObjectDescriptor.hpp`) is the structural symptom.

### Fix — the clean split

Registry = pure const facts with **no `CreativeMutationKind` in its signatures**. Mutation policy = behavior that **reads** the registry. Introduce a policy TU that owns the two leaked functions; keep the fact accessors where they are.

1. **Create `mutation/MutationPolicy.{hpp,cpp}`** (new TU, register in `CMakeLists.txt` beside line 61-63). Move, verbatim, the bodies of `descriptorAllowsMutation` (`ObjectDescriptor.cpp:2012-2036`) and `dirtyFlagsForMutation` (`ObjectDescriptor.cpp:1926-1968`) into it, plus their private helpers that are policy-only: `systemDirtyFlagsForOccupancy` (`ObjectDescriptor.cpp:1730`), `transformSpatialDirtyFlags` (`:1760`), `shapeSpatialDirtyFlags` (`:1774`), and `commonAuthoredDirtyFlags` (`:15`) if not needed by any surviving fact accessor (verify: it is also used by descriptor rows at `:1046+`, so **copy** the constexpr into the policy TU rather than moving it, or expose it from a shared fact header). `MutationPolicy.hpp` includes both `ObjectDescriptor.hpp` (facts) and `Mutation.hpp` (verb predicates) — the policy layer is the one place allowed to know both.
2. **Delete the two declarations from `ObjectDescriptor.hpp`** (lines 226 `dirtyFlagsForMutation` and 240 `descriptorAllowsMutation`) and **delete `#include ".../mutation/Mutation.hpp"` from `ObjectDescriptor.hpp:5`**. This breaks the include cycle: registry no longer depends on the verb vocabulary. Fix the header doc-comment (`ObjectDescriptor.hpp:18-19`) to stop claiming it owns mutation rules.
3. **Repoint consumers** to `MutationPolicy.hpp`: `MutationApply.cpp` (add the include; it already transitively saw these), and the four test files (`creative_object_descriptor_tests.cpp`, `creative_document_dirty_tests.cpp`, `creative_document_mutation_tests.cpp`, `creative_document_path_tests.cpp`). Move the policy assertions out of `creative_object_descriptor_tests.cpp:401-458,907-922` into a new `creative_mutation_policy_tests.cpp` so the registry test file asserts only facts.
4. **Single-source the enum/string tables** (doc's refactor #12): the `toString(...)`/`categoryOf(...)` tables that are genuine facts stay in `ObjectDescriptor.cpp`/`Mutation.cpp` respectively — but confirm no verb-string table is duplicated across the two TUs after the split (grep below). This item is scoped to the fact/behavior cut; the string-table dedup is a follow-on only if the exit grep finds duplication.

**Owner files.** MOVE-FROM: `document/ObjectDescriptor.{hpp,cpp}`. NEW: `mutation/MutationPolicy.{hpp,cpp}`, `tests/unit/creative_mutation_policy_tests.cpp`. EDIT: `mutation/MutationApply.cpp` (include), `CMakeLists.txt` (register TU + test), and the 4 test files above. `Mutation.cpp` is **untouched** (its registry reads are already the correct direction — the doc listing it as a fix target is stale).

### Slices (in order)

- **S5.1 — Stand up the policy TU (mechanical move, no behavior change).** Create `MutationPolicy.{hpp,cpp}`; move `descriptorAllowsMutation` + `dirtyFlagsForMutation` + their three spatial-flag helpers into it; register in CMake. Keep the old declarations in `ObjectDescriptor.hpp` as thin `[[deprecated]]` forwarders temporarily so nothing else has to change yet.
  *Acceptance:* full build + `ctest` green with zero test edits; `grep -c "descriptorAllowsMutation\|dirtyFlagsForMutation" src/app/iggy3d/creative/document/ObjectDescriptor.cpp` == 0.
- **S5.2 — Cut the include cycle.** Delete `#include ".../mutation/Mutation.hpp"` from `ObjectDescriptor.hpp:5`; delete the two forwarder declarations from `ObjectDescriptor.hpp`; repoint `MutationApply.cpp` and the 4 test files to `MutationPolicy.hpp`; fix the `ObjectDescriptor.hpp:18-19` doc-comment.
  *Acceptance:* build green; `grep -n "Mutation.hpp\|CreativeMutationKind" src/app/iggy3d/creative/document/ObjectDescriptor.hpp` == 0 hits; `ObjectDescriptor.cpp` no longer references any `is*Mutation`/`canMutate` symbol.
- **S5.3 — Relocate the policy tests.** Move the allow/deny + dirty-flag assertions out of `creative_object_descriptor_tests.cpp:401-458,907-922` into new `creative_mutation_policy_tests.cpp`; register in CMake.
  *Acceptance:* the descriptor test file contains no `descriptorAllowsMutation`/`dirtyFlagsForMutation` reference; the new policy test file holds them; suite count unchanged (assertions moved, not dropped) and green.
- **S5.4 (optional, gated on exit grep) — Dedup any verb-string table** if S5.3's exit surfaces a duplicated enum↔string table across the two TUs.
  *Acceptance:* one definition per enum-string map; `toString` round-trip test green.

### Exit test

The item is done when the registry TU is fact-only and the include cycle is gone:

```sh
# 1. No mutation policy in the registry TU:
grep -nE 'descriptorAllowsMutation|dirtyFlagsForMutation|canMutate|is(Transform|Shape|Logic|Navigation|Testing|Sensory|Gameplay)Mutation' \
  src/app/iggy3d/creative/document/ObjectDescriptor.cpp        # expect: 0 lines

# 2. Registry header no longer knows the verb vocabulary (cycle broken):
grep -nE 'mutation/Mutation\.hpp|CreativeMutationKind' \
  src/app/iggy3d/creative/document/ObjectDescriptor.hpp        # expect: 0 lines

# 3. Policy lives in exactly one place:
grep -rln 'descriptorAllowsMutation|dirtyFlagsForMutation' src/  # expect: only mutation/MutationPolicy.{hpp,cpp} and MutationApply.cpp

# 4. Registry test asserts only facts:
grep -nE 'descriptorAllowsMutation|dirtyFlagsForMutation' \
  tests/unit/creative_object_descriptor_tests.cpp              # expect: 0 lines
```

All four green + full `ctest` suite green (assertion count preserved via the moved policy tests) proves the registry is pure facts and mutation policy reads it from its own TU.

