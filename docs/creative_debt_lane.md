# Creative Debt Lane — pre-feature architecture debt

**Doctrine: feature-paused except debt that improves feature-readiness.** Do NOT add area-select, multi-object
edit, duplicate, grouping, or batch ops until the feature-start line (bottom) holds — otherwise every new tool
compounds debt. Pay the stack in order. Owner-files from this session's reviews; **re-anchor line/symbol names
at card time** (repo has moved past those refs).

---

## 1. Seal Facade leaks — no raw document mutation from app helpers

**Problem.** App/editor helpers reach past the Facade: raw `CreativeDocument` mutation via
`documentForPersistence()`, direct calls to the `DocumentMutation` free functions
(`moveDocumentObject`/`resizeDocumentObject`/`removeDocumentObject`) and `MutationApply` handlers, and path
editing done by hand. The Facade isn't the only door.
**Fix.** Every mutation goes through a **Facade-owned command method**.
- Grep every *mutating* caller of `documentForPersistence` / the `DocumentMutation` free fns / `applyMutation`
  that lives **outside** `creative/Facade.cpp` (app helpers, `iggy3d_creative/main.cpp`). Route each through a
  Facade command.
- Add Facade commands for **path editing** (path-point add/move/delete) — currently raw.
- Make `MutationApply` handlers **file-local** (header exposes only `applyMutation` + status predicates;
  refactor #5) so app code *can't* call an internal handler.
- Persistence read stays; persistence-adjacent *mutation* moves behind a command.
**Owner files.** `creative/Facade.{hpp,cpp}`, `creative/mutation/MutationApply.{hpp,cpp}`,
`creative/document/DocumentMutation.{hpp,cpp}`, every app/editor helper + `main.cpp` touching the above.
**Exit.** `grep` for document mutation outside the Facade = **0**; `documentForPersistence` has no external
*mutating* caller.

## 2. Finish `iggy3d_creative/main.cpp` decomposition — orchestration-only

**Problem.** ~1651-line `main()` with a ~1160-line frame loop; stages inline, dozens of long-lived mutable locals.
**Fix.** Extract the frame loop into **named stages** — `input, command, pick, selection, placement, move,
overlay, submit` — as free functions over a `CreativeEditorState` that owns the currently-free locals (camera,
tool/place mode, gizmo drag anchors, key edge-latches, save paths). `main()` = `parse → build → seed → while(open)
runFrame`. (Not necessarily an `EditorFrame.hpp` yet — but the **seams must be obvious and stable**.)
**Owner files.** `apps/iggy3d_creative/main.cpp` (+ new stage TUs).
**Exit.** `main()` is orchestration-only; each of the 8 stages is a named function; no mutable-local soup.
*(Depends on #1 — stages call Facade commands, not raw mutation.)*

## 3. Stabilize save/delete/undo/redo ownership — one command/receipt path

**Problem.** Save/load/delete/undo are feature-critical but the command/receipt path is not single-coherent
(multiple entry points across the sd-series; `creativeUndo` recomputed; redo added late). Area-select /
multi-object / batch will *hammer* this path.
**Fix.** **One command + one receipt per op** (save/load/delete/undo/redo), each with a single owner (the save
flow service + the creative undo store). No feature added until this is coherent.
**Owner files.** `save/Flow.cpp`, `save/*State`, `ProductCreativeUndoState` + the creative-undo apply path, the
save/delete command entry points.
**Exit.** Each of save/delete/undo/redo has **ONE** command entry + **ONE** receipt; no alternate mutation of
save/undo state anywhere.

## 4. Add validation + indexing seams

**Problem.** No real validator and no object/bounds index — so every tool invents its own
"is-this-selectable / editable / bakeable" check and its own bounds scan.
**Fix.** (a) A **`CreativeValidator`** seam: one place that answers selectable / editable / bakeable per object.
(b) An **object/bounds index** (AABB grid — the algorithms-map occupancy-index spine) so pick / select / bake
query **one** index, not ad-hoc O(n) scans.
**Owner files.** new `creative/CreativeValidator.{hpp,cpp}`; the AABB index TU (`creative/spatial/*` or new);
`RoomBake` + the pick path consume both.
**Exit.** Pick / select / bake all query the validator + index; no tool carries its own selectability or bounds
check. *(This is the seam that makes area-select cheap instead of a new scan.)*

## 5. Separate registry facts from mutation behavior

**Problem.** Object descriptors, enum/string tables, bounds, affordance semantics, and mutation policy bleed into
each other.
**Fix.** The **registry is pure facts** (const data: `ObjectDescriptor`/`kDescriptors`, enum↔string tables,
default bounds, affordance semantics) with **no behavior**; **mutation policy** lives separately and *reads* the
registry. Single-source the enum/string tables (refactor #12).
**Owner files.** `creative/document/ObjectDescriptor.{hpp,cpp}`, `Mutation.cpp`, `MutationApply.cpp`, the enum tables.
**Exit.** Registry/descriptor files contain no mutation logic; mutation reads the registry rather than embedding
facts.

## 6. Keep Vulkan behind render-frame contracts (guardrail — cheap)

**Problem.** Risk that new tools wire straight to Vulkan for ordinary editor rendering.
**Fix.** New tools render through the **debug/overlay/draw-list/render-frame** contracts (`PrimitiveDrawList` /
render bridge). Touch Vulkan **only** when the renderer genuinely lacks a primitive — and then add the primitive
to the render lane, not to the editor.
**Owner files.** `view/PrimitiveDrawList`, the render bridge, `FramePresenter`.
**Exit.** No editor feature includes a Vulkan header; all editor rendering goes through the draw-list/overlay path.

## 7. Pay down high-pressure non-creative hotspots (opportunistic)

**Problem.** `SaveCodec`, `InputFrame`, `FramePresenter`, automation, session/runtime hotspots — creative tooling
*will* traverse these.
**Fix.** Do the `refactor_targets.md` items on the tooling path: SaveCodec enum-table (#8), FramePresenter present
split (#13), Automation lookup/owner-guard dedup (#9/#10), and any `InputFrame` seam the tool needs. Only the ones
a creative tool actually crosses.
**Owner files.** `runtime/save/SaveCodec.cpp`, `window/InputFrame.cpp`, `window/FramePresenter.cpp`,
`automation/Automation.cpp`.
**Exit.** These sit below the complexity threshold where a new tool's traversal is safe.

---

## The feature-start line (ALL must hold before tooling resumes)

- No raw document mutation from app helpers (#1).
- Save/delete/undo/redo command ownership is clear — one entry, one receipt each (#3).
- `main.cpp` has only orchestration-level responsibility (#2).
- Validation / index / projector seams exist (#4/#5).
- A feature plan can name **exact owner files** without inventing new shortcuts.

**When these hold, feature work is additive, not compounding.**

## Sequencing

**Critical path: 1 → 2 → 3.** Sealing the Facade (#1) is what lets `main.cpp` (#2) call commands instead of raw
mutation, which is what lets save/undo (#3) become the single coherent path. **#4 → #5** are the seams tooling
queries (do after the path is clean). **#6** is a cheap guardrail, land anytime. **#7** is opportunistic — pay a
hotspot the moment the path above crosses it. **Serial on shared files** (`Facade`, `main.cpp`, the save path) —
same one-at-a-time discipline as the decomposition.

*Cut cards from #1 first — it's the root leak; everything downstream inherits it. Each card gets its own local
preflight (owner-grep + exit-criterion test) at build time.*
