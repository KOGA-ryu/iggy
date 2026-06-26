# Product Shell Consolidation Audit v0.1

## Objective

Record the current `AppShell.cpp` boundary after the product module consolidation
work and decide whether the next lane should keep consolidating files or return
to building the dungeon editor loop.

## Current Boundary

Measured on `ff9d7e2 Move automation app adapter out of AppShell`:

- `src/app/iggy3d/AppShell.cpp`: 159 lines.
- Branch shape: 7 `if` statements, 0 `else if`, 0 `while`, and 3 settings
  mapping `switch` statements.
- Product file count under `src/app/iggy3d`: 127 files recursively, including
  109 files directly under `src/app/iggy3d` and 18 files under
  `src/app/iggy3d/product`.

Raw file count is not the goal. The goal is stable ownership boundaries that
keep runtime/data hot paths, product orchestration, frontend routing, and proof
receipt assembly separate enough to change safely.

## AppShell Responsibilities

`AppShell.cpp` is now acceptable as a shell. It still owns:

- CLI option parsing and help/error receipt printing.
- Translating product options into frontend settings defaults.
- Product startup composition: world template, save scan, frontend state,
  world setup draft, starter transition, optional New World launch, scripted
  gameplay smoke, automation control invocation, gameplay tape option flow,
  window loop invocation, no-window projection refresh, and final receipt print.
- The final process return code policy for requested but uncreated windows.

`AppShell.cpp` no longer owns:

- Automation command parsing, registry, control-file sequencing, command
  dispatch, or app-command bridge callbacks.
- Gameplay tape parse/run proof copying.
- Live SDL window creation, loop timing, input frame processing, frame
  projection assembly, frame presentation, renderer lifecycle, or projection
  metrics refresh.
- Menu surface action behavior, menu input routing, room-editing automation,
  save-browser automation, gameplay automation, system automation, world setup
  automation, or world-create automation.

## Consolidation Candidates

| Group | Evidence | Classification | Recommendation |
| --- | --- | --- | --- |
| `ProductWindowLoop.*`, `ProductWindowInputFrame.*`, `ProductWindowFramePresenter.*`, `ProductWindowRendererLifecycle.*` | 740 total lines across 8 files. Loop, input, presentation, and renderer lifecycle are adjacent but distinct orchestration boundaries. | Keep split | Do not merge now. These files compose each other cleanly and isolate SDL loop, input, draw/submit policy, and renderer setup/shutdown. |
| `ProductGameplayProjectionRefresh.*` | 522 total lines. Owns no-window and live-window projection frame/metric assembly. | Keep split | Keep separate from window presentation and render bridge. It is a projection/proof assembly boundary, not a window lifecycle file. |
| `src/app/iggy3d/product/Automation*.{hpp,cpp}` | 3,668 total lines across parser/registry/control/dispatch/execution layers. `Automation.cpp` is large at 1,707 lines, while execution layers are already split by command surface. | Defer | Do not block building on further automation consolidation. Later, consider splitting parser/registry declarations from common execution only if a new automation change makes that file painful. |
| `product/ProductMenuActionHandlers.*` and `product/ProductMenuInputRouter.*` | 854 total lines across 4 files. Handlers own surface action execution; router owns owner routing/action recording/surface dispatch. | Keep split | Keep current split. Combining router and handlers would recreate a menu pile. |
| Small room editor/product room files: `ProductRoomEditorActionController.*`, `ProductRoomEditorCursor.*`, `ProductRoomEditorOverlay.*`, `ProductRoomEditingState.*`, `ProductRoomAuthoringController.*`, `ProductRoomGeometryOptimization.*` | 1,755 total lines across 12 files. Action, cursor, overlay, editing state, authoring, and geometry optimization are related but not the same owner. | Defer | Do not consolidate before editor-building work. Revisit only if the next editor slices repeatedly edit the same two files for one concept. |

## Future Consolidation Rule

- Consolidate by ownership boundary, not by file-count vanity.
- Avoid one-shot files when a durable owner already exists.
- Do not merge hot runtime/data paths just to reduce count.
- Keep product orchestration files cold and explicit unless a real maintenance
  problem appears.
- SoA belongs in hot data paths later, not in cold product orchestration by
  default.
- A consolidation slice should name the duplicated policy or incoherent owner it
  removes before moving code.

## Recommendation

Stop AppShell extraction for now and return to building. The shell is short,
branch-light, and no longer owns the behaviors that previously made it risky.

The next lane should be map authoring, render/editor usability, and proof for
the dungeon editor loop. Do not restart NPC behavior work in this lane.

## Next 5 Build Slices

1. Visual/editor tool HUD polish and hotkey receipts.
   Existing proof already covers room editor cursor/tool/place receipts and
   overlay readiness in `product_ascii_map_smoke`; the next slice should make
   the live tool state easier to inspect and verify.

2. Room editor delete, undo, and redo through real UI/input proof.
   Automation proof exists in `product_room_editing_automation_smoke`, but the
   next useful build proof is live/editor input parity for delete/undo/redo
   behavior, not another automation-only path.

3. Save edited room after editor input proof.
   `product_ascii_map_smoke` already covers save/continue for edited custom
   drafts and live-edited active rooms. The next slice should tighten the
   user-facing save action and receipts around edits made through the editor
   controls.

4. Continue/load edited room visual or render proof.
   Existing no-window receipt proof verifies authored room persistence and
   active room reload. The next build proof should make the loaded edited room
   visible through the render/projection path.

5. Floor/wall render optimization proof only where it improves editor
   usability.
   Geometry optimization should follow observed editor/render friction, not
   precede the editing loop as speculative cleanup.

## Stop Rules

- Do not start another AppShell extraction unless a new concrete behavior makes
  `AppShell.cpp` grow again.
- Do not consolidate product files solely because the recursive file count is
  127.
- Do not merge automation, menu, window, projection, and room-editor modules
  into broader catch-all files.
- Keep future proof focused: targeted builds, no-window smokes, and narrow CTest
  filters before any manual/window check.

## Open Questions

None for this audit. The recommendation is to return to building unless a later
slice reveals a specific low-risk, high-value consolidation candidate.
