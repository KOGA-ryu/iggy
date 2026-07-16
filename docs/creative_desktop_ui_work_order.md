# Creative Desktop UI - Current Claude Work Order

> This file holds one implementation batch. Build this post-state exactly.
> Do not redesign the desktop architecture, widen the batch, or start the next
> roadmap milestone. Report a STOP when a stated seam is false.

## Batch UI-4B: Authored Asset Library And History

**Required baseline:** `30107ec8` (`Improve creative object routing and
optimization infrastructure`) or a direct descendant approved by the user.

**Position:** desktop shell -> semantic dispatcher -> Play mode -> logic
authoring -> Project/Inspector -> **UI-4B (this batch)** -> Tool Settings and
general document tabs -> expanded runtime logic.

## Baseline Truth

Verify these facts before editing:

1. `EditorDesktopPanels.cpp` owns the menu, read-only toolbar, diagnostics,
   status bar, and top-level panel orchestration. It delegates Project and
   Inspector to `EditorDesktopOutliner.cpp` and
   `EditorDesktopInspector.cpp`.
2. `main.cpp` builds one `CreativeDesktopCommandFrame` and dispatches it once
   after all desktop widgets have emitted commands. Keep this exact ordering.
3. `CreativeEditorDesktopUiState` already has unwired `showAssetLibrary` and
   `showHistory` flags. The default dock layout currently docks only Project,
   Inspector, Diagnostics, and Toolbar.
4. `EditorDesktopCommands.*` already owns typed commands for Equip, Edit
   Source, Rename, Duplicate, Delete, Refresh Instances, and Update Asset From
   Instance. It does not yet own Create Asset From Selection.
5. The successful Rename/Duplicate/Delete/Update desktop command branches do
   not currently reconcile the authored definition with the catalog, tool
   wheel, hotbar, and held item. This is a known gap to repair in this batch,
   not accepted behavior.
6. The controller-first legacy asset-library flow remains in
   `EditorAssetLibraryFrame.cpp`. It owns input routing and overlay state and
   must remain behaviorally intact.
7. `CreativeDocumentHistory` stores at most 32 full-document snapshots in each
   undo/redo ring. Each snapshot already carries a `source` string. A desktop
   history projection must borrow those rings and copy only small row facts,
   never copy snapshot documents.
8. `activeCreativeEditorAppState(editor, mapAppState)` is the accepted seam for
   choosing the map or the isolated authored-asset edit workspace. Project and
   Inspector already receive that active state from `main.cpp`.
9. `EditorDesktopInspector.cpp` is over the accepted concern bound at roughly
   663 physical lines because general properties and logic authoring share one
   implementation owner. Its logic section can move without changing its
   command contract.
10. Imported `.glb` reload requires live renderer, scene-cache, and mutable
    catalog owners that are not present in `CreativeDesktopCommandContext`.
    Import is not part of this batch.

If any fact is false, stop before editing and report the exact file and symbol.

## Outcome

Deliver two real desktop panels over existing kernels:

1. `Asset Library` lists, searches, creates, equips, edits, renames,
   duplicates, and reference-safely deletes authored composite assets.
2. `History` shows the active document's undo and redo rings without copying
   snapshots and provides working Undo/Redo commands.
3. Asset mutations reconcile all editor references exactly once after durable
   success. No stale catalog, tool-wheel, hotbar, or held-item facts remain.
4. An active authored-asset edit session is unmistakable and has working Save
   Asset and Discard actions. Map New/Open/Save As cannot run against the wrong
   document while that session is active.
5. Project/Inspector/Diagnostics remain the initially selected tabs in their
   existing dock areas. Asset Library shares the left dock; History shares the
   bottom dock.
6. Logic Inspector rendering moves to its own concern owner with no behavior
   change.

## Non-Negotiable Laws

- **Widgets emit; dispatcher mutates.** ImGui code may read definitions,
  history, and summaries, but may not mutate them.
- **One dispatch per frame.** Do not add another queue or dispatch call.
- **IDs, not pointers.** Desktop state retains asset ids and object ids only.
  Never retain definition pointers, document pointers, spans, or history-row
  pointers across frames.
- **Durability before reconciliation.** Rename, duplicate, delete, create, and
  edit-save must update catalog/hotbar references only after their durable
  kernel reports success.
- **History is a projection.** Do not expose mutable snapshots, copy documents,
  add jump-to-index behavior, or invent selective undo.
- **Active-document truth.** History, Project, and Inspector project the same
  active `CreativeAppState` in a frame.
- **Play is read-only.** Asset/history mutations are disabled in widgets and
  still rejected by the dispatcher during Play.
- **Linear panel work.** Asset filtering is O(asset count). Reference summary
  is computed once for the selected asset, never once per row. History
  projection is O(undo depth + redo depth).
- **No branch ladder.** Commands remain closed-enum switch dispatch. Asset
  display filtering uses a small pure model, not repeated widget conditionals.
- **No false affordances.** Do not show Import, thumbnails, history jumping, or
  batch actions that do not exist.
- **No persistence changes.** Panel selection, search, drafts, and derived rows
  remain transient desktop state.
- **Controller parity.** Do not remove or bypass the legacy controller asset
  library. Shared reconciliation extraction must leave its behavior intact.

## Interaction Contract

### Asset Library Panel

- Keep the dock window name exactly `Asset Library`.
- Render a dense operational list, not cards.
- Place one compact search field above the list.
- Search is ASCII case-insensitive over authored asset label and asset id.
- Preserve `editor.authoredAssets.definitions` order.
- Keep selection by `assetId`. If the selected id disappears, select the first
  filtered row; if no row exists, clear selection.
- Each row shows label, compact asset id, and stored object count.
- A row click changes only transient panel selection. It emits no editor
  command.
- Do not display fake image thumbnails. The engine has no desktop thumbnail
  product yet.

Selected-asset actions:

- `Equip` emits `EquipAsset`.
- `Edit Source` emits `EditAssetSource` with phase `Begin`.
- `Rename` opens an in-app modal with a resizable string draft and emits
  `RenameAsset` only on confirmation.
- `Duplicate` emits `DuplicateAsset` immediately.
- `Delete` opens an in-app confirmation modal. Show map-instance and authored
  dependency counts from `summarizeCreativeEditorAuthoredAssetReferences`.
  Disable confirmation when either count is nonzero. The dispatcher/kernel is
  still the final reference-safety authority.
- Put Rename/Duplicate/Delete in one compact option menu rather than three
  persistent wide buttons.

Creation:

- Provide `Create Asset From Selection` as a clear command above the list.
- It opens an in-app label modal and emits the new semantic command
  `CreateAssetFromSelection` using `CreativeDesktopAssetOpPayload::name`.
- Disable it when there is no valid selection, during Play, during an authored
  asset edit session, or when the authored-asset root is unavailable.
- The dispatcher calls `saveCreativeEditorSelectionAsAuthoredAsset`, then on
  success refreshes catalog references and equips the created asset using the
  same post-steps as `SaveSelectionAsAsset` in `EditorObjectActions.cpp`.
- One accepted create changes the asset library and held item but does not add
  a document-history entry.

Empty/error states:

- Empty library: `No authored assets` plus the working Create action when
  eligible.
- Empty search: `No matching assets`.
- Surface the latest dispatcher message in the existing status bar; do not
  create a second notification system.

### Authored-Asset Edit Session

When `editor.assetEdit.active`:

- The toolbar identifies `ASSET: <label>` and whether the edit is dirty.
- Provide working `Save Asset` and `Discard` commands in the toolbar or Asset
  Library panel. They emit `EditAssetSource` phases `Save` and `Cancel`.
- Disable Create, Equip, Rename, Duplicate, Delete, and beginning another Edit
  Source session.
- The File menu hides New, Open, Save As, and Import. Its Save item becomes
  `Save Asset` and emits `EditAssetSource` phase `Save`.
- Discard must require a confirmation modal when the edit session is dirty.
- Do not invent multiple simultaneous asset-edit tabs. The existing one-session
  workspace remains the owner.

Outside an asset-edit session, the File menu keeps New/Open/Save/Save As.
Remove the currently disabled Import item entirely until a renderer-aware
semantic import command exists.

### History Panel

- Keep the dock window name exactly `History`.
- Show compact Undo and Redo buttons at the top. They emit the existing
  `Undo` and `Redo` command ids.
- Disable buttons when the corresponding ring is empty or during Play.
- Show current document id, revision, object count, and ring depths.
- Render undo entries newest-first. Mark the first row as `NEXT UNDO`.
- Render redo entries in actual application order (`redoSnapshots.back()` is
  first). Mark the first row as `NEXT REDO`.
- Each row shows a humanized source label, snapshot revision, and object count.
- Preserve the exact source string in a hover tooltip for diagnostics.
- Rows are read-only. Do not make them Selectable or clickable.
- Empty rings show `Nothing to undo` / `Nothing to redo`.
- Saving or replacing a document may clear history exactly as the existing
  dispatcher already does; the panel must reflect that immediately.

### Docking And View Menu

- Add real View-menu toggles for `Asset Library` and `History` using the
  existing state flags.
- Dock Asset Library and Project into the existing left node.
- Dock History and Diagnostics into the existing bottom node.
- On Reset Layout, Project and Diagnostics remain the initially selected tabs.
  Dock the secondary window first and the accepted primary window last if that
  is sufficient under the pinned ImGui version.
- Do not change the central viewport, pointer policy, HiDPI conversion,
  renderer scissor, or status-bar reserve.

## Required Post-State Ownership

### Create: Pure Secondary-Panel Model

- `apps/iggy3d_creative/EditorDesktopWorkspaceModel.hpp`
- `apps/iggy3d_creative/EditorDesktopWorkspaceModel.cpp`
- `tests/unit/creative_desktop_workspace_model_tests.cpp`

This owner is ImGui-free and mutation-free. It owns:

- small asset rows copied from live definitions;
- deterministic ASCII search/filtering;
- selected-asset-id resolution after filter/library changes;
- small history rows copied from snapshot metadata only;
- newest-first undo and application-order redo projection;
- human-readable history source labels.

The history model may copy `source`, revision, object count, and ids. It may not
contain `CreativeDocument`, `CreativeDocumentHistorySnapshot`, or pointers to
either.

Target physical size: 180-400 lines.

### Create: ImGui Widget Owners

- `apps/iggy3d_creative/EditorDesktopAssetLibrary.cpp`
- `apps/iggy3d_creative/EditorDesktopHistory.cpp`
- `apps/iggy3d_creative/EditorDesktopLogicInspector.cpp`

`EditorDesktopAssetLibrary.cpp` owns only authored-asset list/detail rendering,
modal drafts, and conversion of user intent into typed commands.

`EditorDesktopHistory.cpp` owns only history rendering and Undo/Redo emission.

`EditorDesktopLogicInspector.cpp` receives the existing logic section,
diagnostic navigation, new-link controls, and runtime logic monitor moved
source-equivalently from `EditorDesktopInspector.cpp`.

Target physical sizes after formatting:

- `EditorDesktopAssetLibrary.cpp`: 250-550 lines.
- `EditorDesktopHistory.cpp`: 100-260 lines.
- `EditorDesktopLogicInspector.cpp`: 250-420 lines.
- `EditorDesktopInspector.cpp`: 300-500 lines after extraction.
- `EditorDesktopPanels.cpp`: below 450 lines.

These are concern bounds, not permission to add neutral whitespace or compress
readability. Stop when a concern cannot fit its destination.

### Modify

- `apps/iggy3d_creative/EditorDesktopUi.hpp`
  - Add transient asset query, selected asset id, create/rename drafts, and
    one-shot modal state. Keep it ImGui-free and non-persistent.
- `apps/iggy3d_creative/EditorDesktopWidgets.hpp`
  - Declare the Asset Library, History, and extracted logic section builders.
  - Remain one internal cluster header; do not create micro-headers per widget.
- `apps/iggy3d_creative/EditorDesktopPanels.cpp`
  - Add View toggles and top-level panel orchestration.
  - Add active asset-edit toolbar/File-menu behavior.
- `apps/iggy3d_creative/EditorDesktopUi.cpp`
  - Add the two secondary windows to the accepted dock nodes only.
- `apps/iggy3d_creative/EditorDesktopCommands.hpp`
  - Add `CreateAssetFromSelection` and update command comments.
- `apps/iggy3d_creative/EditorDesktopCommands.cpp`
  - Route the create command to existing kernels.
  - Add successful asset-operation reconciliation post-steps.
  - Preserve one closed command dispatcher.
- `apps/iggy3d_creative/EditorDesktopCommandPayloads.hpp`
  - Reuse `CreativeDesktopAssetOpPayload::name` for create; do not add another
    string payload type.
- `apps/iggy3d_creative/EditorAuthoredAssets.hpp/.cpp`
  - Expose one shared deletion reconciliation helper for catalog/tool-wheel/
    hotbar cleanup.
- `apps/iggy3d_creative/EditorAssetLibraryFrame.cpp`
  - Replace its anonymous deletion cleanup body with that shared helper.
  - No other legacy flow change is allowed.
- `apps/iggy3d_creative/EditorDesktopInspector.cpp`
  - Remove the moved logic implementation and call the extracted section.
- `apps/iggy3d_creative/main.cpp`
  - Only the minimum menu signature threading and existing command-result/UI
    cache assignment needed by this batch are allowed.
- `CMakeLists.txt` and `cmake/iggy3d_tests.cmake`
  - Register new owners and the new app-linked model test exactly once.

## Command Reconciliation Matrix

| Command | Existing kernel | Required successful post-step |
|---|---|---|
| CreateAssetFromSelection | `saveCreativeEditorSelectionAsAuthoredAsset` | refresh definition references, equip, sync held item |
| EquipAsset | existing equip helper | no extra mutation |
| EditAssetSource Begin | `beginCreativeEditorAuthoredAssetEdit` | active document invalidation remains in main |
| EditAssetSource Save | `saveCreativeEditorAuthoredAssetEdit` | retain its existing reference refresh |
| EditAssetSource Cancel | `cancelCreativeEditorAuthoredAssetEdit` | restore map state through existing kernel |
| RenameAsset | `renameCreativeEditorAuthoredAsset` | refresh catalog entry by unchanged asset id |
| DuplicateAsset | `duplicateCreativeEditorAuthoredAsset` | add/refresh new catalog entry |
| DeleteAsset | `deleteCreativeEditorAuthoredAsset` | remove catalog/tool-wheel/hotbar references |
| RefreshInstances | existing refresh kernel | preserve history behavior |
| UpdateAssetFromInstance | existing update kernel | refresh catalog entry and held item |
| Undo / Redo | existing history commands | no panel-side post-step |

If a required post-step cannot be represented by these named owners, stop and
report the missing seam. Do not duplicate controller cleanup logic in the
desktop widget or dispatcher.

## Scope Firewall

Do not modify:

- renderer, Vulkan, frame input, viewport, picking, camera, or pointer code;
- Creative document, mutation, history, persistence, or save-format owners;
- input bindings, controller routing, or legacy overlay presentation;
- terrain, placement, moving-platform, physics, AI, or runtime logic owners;
- imported static-mesh loading/reload kernels;
- capture outputs, screenshots, or golden files.

No Import command, thumbnails, drag/drop, asset folders/tags, history jumping,
selective undo, multi-document generalization, or tool-settings panel is part
of UI-4B.

## Headless Proof

### New Workspace Model Tests

`creative_desktop_workspace_model_tests.cpp` must pin:

1. Empty asset library.
2. Definition-order asset projection.
3. ASCII case-insensitive label and asset-id filtering.
4. Stable selection by asset id across filtering and vector reordering.
5. Missing selection fallback to the first filtered asset.
6. Empty filtered result clears selection.
7. Empty history rings.
8. Undo rows newest-first with exactly one next-undo marker.
9. Redo rows in real application order with exactly one next-redo marker.
10. History rows preserve source/revision/object-count facts without storing a
    document or snapshot type.
11. Humanized source labels are deterministic; exact source remains available.

### Extend Desktop Command Tests

`creative_desktop_ui_command_tests.cpp` must additionally pin:

1. Create Asset From Selection performs a durable save, refreshes the catalog,
   equips the asset, and records no document history.
2. Rename refreshes the existing catalog label.
3. Duplicate adds exactly one catalog entry.
4. Referenced delete rejects without removing catalog/hotbar facts.
5. Unreferenced delete removes definition, catalog, tool-wheel assignment, and
   hotbar references only after durable success.
6. Update From Instance refreshes catalog/held facts.
7. Edit Source Begin/Save/Cancel preserve map/workspace isolation.
8. Play rejects every asset and history mutation.
9. Wrong payload alternatives remain explicit no-op failures.

### Required Regression Set

Build and run:

- `i3dc`
- `creative_desktop_workspace_model_tests`
- `creative_desktop_model_tests`
- `creative_desktop_ui_command_tests`
- `creative_viewport_layout_tests`
- `creative_authored_asset_tests`
- `creative_editor_asset_reload_tests`
- `creative_editor_group_tests`
- `creative_editor_logic_link_tests`
- `creative_editor_play_mode_tests`
- `creative_volume_tests`

Required commands:

```sh
cmake -S . -B build
cmake --build build --target i3dc creative_desktop_workspace_model_tests creative_desktop_model_tests creative_desktop_ui_command_tests creative_viewport_layout_tests creative_authored_asset_tests creative_editor_asset_reload_tests creative_editor_group_tests creative_editor_logic_link_tests creative_editor_play_mode_tests creative_volume_tests -j 4
ctest --test-dir build -R '^(creative_desktop_workspace_model_tests|creative_desktop_model_tests|creative_desktop_ui_command_tests|creative_viewport_layout_tests|creative_authored_asset_tests|creative_editor_asset_reload_tests|creative_editor_group_tests|creative_editor_logic_link_tests|creative_editor_play_mode_tests|creative_volume_tests)$' --output-on-failure
git diff --check
```

Do not run a broad CTest loop and do not launch a window automatically.

## Manual Visual Checkpoint

After headless verification, stop and ask the user to launch `./build/i3dc`.
The user should verify:

1. Project remains the initial left tab and Asset Library is readable beside
   it.
2. Diagnostics remains the initial bottom tab and History is readable beside
   it.
3. Asset search and explicit selection feel predictable.
4. Create/Equip/Edit/Rename/Duplicate/Delete each produce one visible result.
5. Reference-blocked deletion explains why it cannot proceed.
6. Asset-edit mode is unmistakable; Save Asset and Discard return safely to
   the map.
7. Undo/Redo buttons and rows update immediately after edits and saves.
8. General Inspector and all logic controls remain unchanged.
9. Fly-look, free cursor, central viewport, and panel docking remain stable.

Do not commit an interactive repair until the user reports the observed issue.

## Stop Conditions

Stop without improvising when:

- baseline facts do not match;
- Create Asset From Selection cannot reuse the existing durable save kernel;
- a desktop asset operation cannot reconcile catalog/hotbar state without
  duplicating controller logic;
- active asset-edit Save/Discard would require mutating from a widget;
- a history model would need to copy a `CreativeDocument`;
- Project/Inspector/History resolve different active documents in one frame;
- preserving the legacy controller library requires a behavior rewrite;
- the Inspector logic extraction changes any command, label, diagnostic, or
  runtime-monitor behavior;
- a new TU cannot fit its stated concern bound;
- configure, build, a focused test, or `git diff --check` fails.

## Completion Brief

Report:

- commit hash;
- exact files created/modified;
- Asset Library interactions delivered;
- History interactions delivered;
- asset reconciliation behavior and proof;
- active asset-edit safety behavior;
- Inspector split line counts;
- commands/tests run and exact results;
- manual visual checkpoint requested;
- deviations, STOPs, and remaining risks.

Do not start Tool Settings, general document tabs, imported-asset UI, or the
next roadmap milestone after this batch.
