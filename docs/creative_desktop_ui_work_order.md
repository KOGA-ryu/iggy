# Creative Desktop UI - Current Claude Work Order

> This file holds one implementation batch. Build this post-state exactly.
> Do not redesign the desktop architecture, widen the batch, or start the
> next roadmap milestone. Report a STOP when a stated seam is false.

## Batch UI-5: Tool Settings And Document Tabs

**Required baseline:** the merge of `claude/ui-4b-integration` (`a195acb0`)
with the current `origin/creative-only` tip. Construct it exactly:
`git fetch origin`, branch `claude/ui-5-tool-settings` from
`claude/ui-4b-integration`, then `git merge origin/creative-only`. The merge
is expected clean (verified by merge-tree on 2026-07-16 — the branches share
EditorDesktopUi.hpp/.cpp, EditorAuthoredAssets.cpp, and main.cpp but all
hunks merge cleanly); if ANY conflict appears, STOP and report the files. Run the full Required Regression Set on
the merge commit before writing code; it must be green at baseline.

**Position:** desktop shell -> semantic dispatcher -> Play mode -> logic
authoring -> Project/Inspector -> UI-4B (landed, integrated at `a195acb0`)
-> **UI-5 Tool Settings and document tabs (this batch)** -> UI-6 trust
repairs (undo survives save, unsaved-changes guard, real keyboard dispatch —
field survey §2) -> UI-7 command palette + repeat/duplicate-translate (field
survey §3) -> UI-8 overlays and receipts (field survey §5, engine survey
§2/§4) -> expanded runtime logic; imported-asset UI stays parked until the
renderer-aware import seam exists. Roadmap sources:
`docs/creative_desktop_ui_field_survey.md`,
`docs/creative_engine_tools_industry_survey.md`.

## Baseline Truth

Verify these facts before editing:

1. The tool-options system is three layers. Engine:
   `creative::CreativeToolSettings` (`src/app/iggy3d/creative/tools/Tools.hpp:262-373`,
   all-enum, static-asserted trivially copyable + standard layout, defaulted
   `operator==`); a 61-entry descriptor table `kToolOptionDescriptors`
   (`src/app/iggy3d/creative/tools/ToolOptions.cpp:63-314`) where each
   `CreativeToolOptionDescriptor{id,label,valueKind,applicableHeldItems}`
   carries a held-item bitmask; `creativeToolOptionsForHeldItem(kind,
   settings)` (ToolOptions.cpp:339) filters into a bounded
   `CreativeToolOptionList` (capacity 8, `capacityExceeded` flag);
   `adjustCreativeToolOption(settings, option, ±1, palette)`
   (`src/app/iggy3d/creative/tools/ToolSettings.cpp:262`) is the sole
   engine-layer option-value mutation kernel and returns a receipt (app code
   also writes a few fields directly — catalog shape selection, terrain
   paint material, and commitOptions' whole-struct assignment).
2. App layer: `creativeEditorToolOptionsForEntry`
   (`apps/iggy3d_creative/EditorToolOptions.cpp:45-98`) wraps the engine
   builder with the MaterialPlacement filter and INJECTS the
   AssetPlacementMode/AssetScatter* family (those descriptors carry
   `applicableHeldItems == 0` in the engine table and are invisible to the
   raw engine builder). Desktop code must call the app-layer builder, never
   the engine builder directly.
3. The in-game overlay (`apps/iggy3d_creative/EditorToolOptionsPanel.cpp`)
   edits a DRAFT copy (`state.draft = editor.toolSettings` on open) and
   commits atomically via `commitOptions` (EditorToolOptionsPanel.cpp:216-240)
   with four side effects: terrain-seed-mode entry resets terrain selection;
   `storeSelectedCreativeMaterialBrushPreset` persists per-hotbar-slot brush
   settings; `editor.placeCellSize` resyncs from
   `creativeSnapIncrementMeters(snapIncrement)`;
   `syncCreativeEditorQuickEdit` rebuilds the quick-edit ring. Value-label
   projection is `creativeToolOptionValueLabel(settings, id)`
   (`src/app/iggy3d/creative/tools/ToolSettingLabels.cpp:161`).
4. The desktop shell reads none of this today. The only hook is the dormant
   `bool showToolSettings = true;` view flag
   (`apps/iggy3d_creative/EditorDesktopUi.hpp:97` on the merged baseline),
   referenced nowhere else. `CreativeDesktopCommandId`
   (EditorDesktopCommands.hpp, 36 real ids on the baseline — UI-4B added
   CreateAssetFromSelection — plus the Count sentinel) has zero tool-option
   entries and never touches `editor.toolSettings`.
5. The live tool is the hotbar entry: current held item, object kind, and
   asset id come from the editor's hotbar state, and per-tool presentation
   profiles live in `EditorToolCapabilities.hpp`
   (`CreativeEditorToolOptionFilterProfile{Standard, MaterialPlacement}`).
   The desktop toolbar deliberately has no tool buttons.
6. The active-document seam is `activeCreativeEditorAppState(editor,
   mapAppState)` (`apps/iggy3d_creative/EditorAssetLibrary.cpp:611-621` on
   the baseline): returns `editor.assetEdit.workspace` when a session is
   active, else the map state. `dispatchOne`
   (EditorDesktopCommands.cpp:~117-128) resolves it once; edit mutations run
   against it; document-lifecycle and library/instance ops stay on the map.
7. The asset-edit session is `CreativeEditorAuthoredAssetEditSession`
   (`apps/iggy3d_creative/EditorState.hpp:53-69`): `active`, `assetId`,
   `label`, a second full `CreativeAppState workspace`, `dirty`, saved map
   camera, `mapDocumentState`. It is driven exclusively through
   `CreativeDesktopCommandId::EditAssetSource` with
   `CreativeDesktopAssetEditPhase{None,Begin,Save,Cancel}`
   (EditorDesktopCommandPayloads.hpp:97-110). Cancel restores the map camera
   and transient document state. Note: EditAssetSource is exclusive only
   WITHIN the desktop shell — the in-game controller flow
   (EditorAssetLibraryFrame.cpp:185/:229/:234/:362) and the shutdown path
   (main.cpp:806) call the same begin/save/cancel kernels directly, which is
   precisely why tabs must be a per-frame projection of session state, never
   a mirror of dispatched commands.
8. UI-4B already ships the tab-adjacent machinery: File-menu lockdown during
   a session (EditorDesktopPanels.cpp:196-225 — only "Save Asset" visible),
   the amber toolbar session banner with Save Asset/Discard and the one-shot
   `Discard Asset Edit##desktop` modal armed by
   `desktopUi.assetEditDiscardConfirmRequested`
   (EditorDesktopPanels.cpp:96-181), and scene-cache invalidation on session
   flips in main.cpp (~398-404 and ~450-452). Play mode forces the map
   appState even mid-session.
9. The central dock node is passthru and hosts NO window — the 3D scene
   renders through it; the offscreen scene target is explicitly deferred
   (plan §7). The 44px `Toolbar##desktop` strip above the central node is
   where `appendToolbarHeader` already renders the de facto proto-tab
   (document name / asset banner).
10. main.cpp builds one `CreativeDesktopCommandFrame` and dispatches it once
    after all desktop widgets have emitted. During Play the dispatcher
    accepts only None/Play/SelectObjects/ClearSelection.

If any fact is false, stop before editing and report the exact file and
symbol.

## Outcome

Deliver two real desktop surfaces over existing kernels:

1. A `Tool Settings` desktop panel (dock: right node, tabbed with
   `Inspector`; window name exactly `Tool Settings`) showing OPTION ROWS for
   the current hotbar entry: label, current value label, and [-]/[+] adjust
   controls operating on a transient desktop DRAFT copy, with Apply and
   Revert. Rows come from `creativeEditorToolOptionsForEntry` projected
   through a new pure model; value labels via
   `creativeToolOptionValueLabel`. Rebuild the row list against the draft
   after EVERY accepted adjust — do not copy the overlay's 7-option rebuild
   trigger list (EditorToolOptionsPanel.cpp:201-208), which has a
   pre-existing stale-row behavior for terrainPaintMode and
   assetPlacementMode changes; unconditional rebuild is cheap and strictly
   more correct.
2. A new typed desktop command `SetToolSettings` whose payload carries a
   whole `CreativeToolSettings` by value. The dispatcher validates the
   struct the same way `commitOptions` does and, on accept, assigns
   `editor.toolSettings` and performs the SAME four side effects as
   `commitOptions` (Baseline Truth 3) — but never the overlay's panel-close;
   the four-effect list is commitOptions' post-assignment effect set, not
   its full body. If that validation logic is inline in the overlay panel,
   extract it to one shared function both callers use — do not duplicate it.
   Rejected during Play. Tool settings are editor preferences: the command
   creates NO document history entry and changes NO save output.
3. A document tab strip inside the existing `Toolbar##desktop` window,
   replacing the current document-name text and asset banner: a `Map` tab
   plus, when a session is active, one asset tab (session label, `*` when
   dirty, and an `x` close affordance). Save Asset / Discard buttons remain
   when a session is active; device and PLAY status remain on the right.
4. Tab behavior is a pure projection of existing state and emits only
   existing commands: the active tab is whichever document
   `activeCreativeEditorAppState` resolves to; clicking the asset tab when
   active is a no-op; clicking `Map` (or `x`) with a CLEAN session emits
   `EditAssetSource`/Cancel; with a DIRTY session it arms
   `desktopUi.assetEditDiscardConfirmRequested` and defers to the existing
   modal. No new session state, no new command ids for tabs, no
   focus-existing-session concept.
5. The View menu's `Tool Settings` toggle is wired to the dormant
   `showToolSettings` flag; the panel participates in Reset Layout.
6. Explicit non-goal: the overlay's COMMAND ROWS (object actions,
   moving-platform edits) do not appear in the desktop panel — those verbs
   already exist as desktop commands reachable from Inspector/menus, and the
   in-game overlay keeps them for the controller flow.

## Non-Negotiable Laws

- **Widgets emit; dispatcher mutates.** The panel reads the hotbar entry,
  tool capabilities, and `editor.toolSettings`, and edits only its own
  draft; the ONLY mutation path is the `SetToolSettings` payload through the
  one dispatch. The tab strip may only push `EditAssetSource` phases or arm
  the existing confirm flag.
- **One dispatch per frame.** No new queue, no second dispatch call.
- **App-filter law.** All desktop option rows come from
  `creativeEditorToolOptionsForEntry`. Calling
  `creativeToolOptionsForHeldItem` directly from desktop code is a defect
  (it silently loses the asset-scatter family — Baseline Truth 2).
- **Draft/apply atomicity.** The draft is transient desktop state (the No
  persistence changes law carries over); Apply emits exactly one command;
  Revert re-snapshots from `editor.toolSettings`; Escape cancels the draft
  without dispatching.
- **Tool settings are not document state.** No undo/history entries, no
  document-revision change, no save-codec change, byte-identical save output
  before and after any `SetToolSettings`.
- **Tabs are a projection.** Tab rendering derives from
  `editor.assetEdit` + the active-document seam each frame; no retained tab
  list, no tab state that can disagree with the session.
- **IDs, not pointers.** No definition/document/row pointers retained across
  frames.
- **Play is read-only.** `SetToolSettings` is rejected by the dispatcher
  during Play; the panel's controls and the tab strip render disabled.
- **Capacity honesty.** The 8-slot `CreativeToolOptionList` bound and its
  `capacityExceeded` flag are kept; on exceeded, render a one-line bounded
  notice and NO option rows, never a partial silent list. (Do not go
  looking for an existing notice pattern — the in-game overlay's behavior
  is to silently refuse to open, EditorToolOptionsPanel.cpp:684-685; the
  desktop panel is deliberately more honest.) Do not widen the capacity in
  this batch.
- **No false affordances.** No Import item anywhere; no disabled
  placeholder rows for command verbs.

## Interaction Contract

### Tool Settings Panel

- Zero state (no hotbar entry with options): a one-line explanation, no
  empty chrome.
- Header: current tool label (held-item display name; asset name when the
  hotbar entry carries an asset id).
- Option rows: label left, value label right, [-]/[+] SmallButtons; adjust
  operates on the draft via `adjustCreativeToolOption` with
  `editor.brushPalette` for MaterialOrAny options; a rejected adjustment
  (receipt) leaves the draft untouched.
- Dirty indication when draft != `editor.toolSettings`; Apply enabled only
  when dirty; Revert enabled only when dirty; both disabled during Play.
- Hotbar-entry change while dirty: re-snapshot the draft (matching the
  overlay's open semantics) — do not carry a draft across tools.
- Status line after Apply comes from the dispatch result message (existing
  status-bar path).

### Document Tab Strip

- Rendered inside `Toolbar##desktop` before the right-aligned device/PLAY
  block. `Map` tab always present; asset tab only while
  `editor.assetEdit.active`.
- Asset tab shows `label` + `*` when dirty; `x` on the tab closes via the
  same path as clicking `Map` (clean -> Cancel; dirty -> confirm modal).
- During Play both tabs render as inert labels (the session cannot change
  during Play; the seam forces the map).
- The existing `Discard Asset Edit##desktop` modal is the ONLY discard
  confirmation; do not add a second modal.

### Docking And View Menu

- `Tool Settings` docks into the right node created in
  `buildDefaultDesktopLayout`, tabbed with `Inspector`; `Inspector` stays
  the initially-selected tab. Under the pinned ImGui 1.92.8 the LAST-docked
  window becomes the initially-selected tab (see the UI-4B comment at
  EditorDesktopUi.cpp:90-92), so dock `Tool Settings` FIRST and `Inspector`
  LAST in that node.
- View menu gains the `Tool Settings` toggle wired to `showToolSettings`;
  Reset Layout re-docks it.

## Required Post-State Ownership

### Create: apps/iggy3d_creative/EditorDesktopToolSettings.cpp

Owns the Tool Settings panel: draft handling, row rendering, Apply/Revert
emission. Target physical size: 160-340 lines.

### Extend: apps/iggy3d_creative/EditorDesktopWorkspaceModel.hpp/.cpp

Pure, ImGui-free projections (existing borrow-only discipline): a
tool-settings row model (`buildCreativeDesktopToolSettingsModel`: rows of
{optionId, label, valueLabel} from a hotbar entry + a settings struct) and a
tab-strip model (`buildCreativeDesktopDocumentTabsModel`: tab rows with
label/dirty/active/closable from the session state + play flag). Target
addition: 80-200 lines across both files.

### Modify

- apps/iggy3d_creative/EditorDesktopCommands.hpp
  - add `SetToolSettings` to `CreativeDesktopCommandId`.
- apps/iggy3d_creative/EditorDesktopCommandPayloads.hpp
  - add `CreativeDesktopToolSettingsPayload{creative::CreativeToolSettings}`
    to the payload variant.
- apps/iggy3d_creative/EditorDesktopCommands.cpp
  - `SetToolSettings` dispatch case: validate, assign, four side effects;
    Play rejection; mismatched payload = no-op failure like every other id.
- apps/iggy3d_creative/EditorToolOptionsPanel.cpp
  - ONLY if extracting the shared draft-validation function (Outcome 2);
    no behavior change to the overlay.
- apps/iggy3d_creative/EditorDesktopPanels.cpp
  - tab strip inside `appendToolbarHeader`; View-menu toggle.
- apps/iggy3d_creative/EditorDesktopUi.cpp / EditorDesktopUi.hpp
  - dock the new window in `buildDefaultDesktopLayout`; construct the panel
    behind `showToolSettings`.
- apps/iggy3d_creative/main.cpp
  - panel build call in the existing widget-emission block (before the one
    dispatch).
- cmake/iggy3d_tests.cmake / CMakeLists.txt
  - register the new TU; extend test targets as below.

## Command Reconciliation Matrix

| Command | Existing kernel | Required successful post-step |
| --- | --- | --- |
| SetToolSettings | shared draft validation + assignment to `editor.toolSettings` | terrain-seed selection reset when entering seed mode; `storeSelectedCreativeMaterialBrushPreset`; `placeCellSize` resync; `syncCreativeEditorQuickEdit`; document revision UNCHANGED |
| EditAssetSource (Cancel via tab) | `cancelCreativeEditorAuthoredAssetEdit` | map camera + transient state restored; scene cache invalidated by the existing main.cpp flip handling |

If a kernel is missing or refuses in a way not listed, STOP and report.

## Scope Firewall

Do not modify:

- capture outputs, screenshots, or golden files;
- the save codec, document schema, or undo/history kernels;
- `CreativeToolSettings`, the descriptor table, `adjustCreativeToolOption`,
  or `creativeToolOptionsForHeldItem` (the engine layer is read-only this
  batch);
- the in-game overlay's behavior (only the validation extraction touches
  that TU);
- keyboard accelerator routing (UI-6), command palette (UI-7), overlays
  (UI-8), imported-asset UI.

No multi-session tabs, no focus-existing-session command, no history
jumping, no per-option desktop commands (one whole-struct command only), no
capacity widening, and no Import command is part of UI-5.

## Headless Proof

### New Tool Settings Command Tests (creative_desktop_ui_command_tests)

1. setToolSettingsAppliesAndResyncsPlaceCellSize — dispatch with a changed
   snapIncrement; assert `editor.toolSettings` equals the payload and
   `placeCellSize == creativeSnapIncrementMeters(...)`.
2. setToolSettingsLeavesDocumentUntouched — document revision, undo ring
   sizes, and save bytes identical before/after an accepted dispatch.
3. setToolSettingsRejectedDuringPlay — accepted==false, settings unchanged.
4. setToolSettingsMismatchedPayloadIsNoOpFailure.
5. setToolSettingsInvalidDraftRejected — an out-of-range enum value is
   refused by the shared validation; settings unchanged.

### New Workspace Model Tests (creative_desktop_workspace_model_tests)

6. toolSettingsRowsMatchAppFilterForHeldItem — rows for ObjectMove include
   the MoveConstraint/RotationStep/SnapIncrement family; row labels match
   descriptor labels; value labels match `creativeToolOptionValueLabel`.
7. toolSettingsRowsInjectAssetScatterFamilyForMaterialEntryWithAssetId —
   the entry must set `assetPlacementMode == Scatter` explicitly and use an
   objectKind passing `descriptorSupportsBrushPlacement` (with the default
   Single mode only the AssetPlacementMode row appears — asserting the
   Single case too is encouraged).
8. toolSettingsRowsRespectSettingsDependentVisibility — arrayMode
   Linear/Radial flips the Array* row set.
9. documentTabsModelMapOnly — no session: one active Map tab.
10. documentTabsModelSessionActiveAndDirty — two tabs, asset tab active,
    dirty flag mirrored, close affordance present.
11. documentTabsModelPlayForcesInert — the play flag renders both tabs
    inert.

### Required Regression Set

- creative_desktop_workspace_model_tests
- creative_desktop_ui_command_tests
- creative_editor_logic_link_tests
- creative_authored_asset_tests
- creative_editor_moving_platform_preview_tests
- creative_editor_placement_tests

Required commands:

```sh
cmake -S . -B build
CCACHE_DISABLE=1 cmake --build build --target i3dc \
  creative_desktop_workspace_model_tests \
  creative_desktop_ui_command_tests \
  creative_editor_logic_link_tests \
  creative_authored_asset_tests \
  creative_editor_moving_platform_preview_tests \
  creative_editor_placement_tests -j 4
ctest --test-dir build -R '^(creative_desktop_workspace_model_tests|creative_desktop_ui_command_tests|creative_editor_logic_link_tests|creative_authored_asset_tests|creative_editor_moving_platform_preview_tests|creative_editor_placement_tests)$' --output-on-failure
git diff --check
```

Do not run a broad CTest loop and do not launch a window automatically.

## Manual Visual Checkpoint

After headless verification, stop and ask the user to launch ./build/i3dc.

1. Tool Settings appears tabbed behind Inspector in the right dock;
   Inspector is the initially-selected tab.
2. With ObjectMove held, rows show Move Constraint / Rotation Step / Snap
   Increment with correct current values.
3. [+] on Snap Increment changes the value label and marks the panel dirty;
   Apply commits (status bar reports it) and placement uses the new cell
   size; Revert after another change restores.
4. Switching hotbar tools re-snapshots the panel (no stale draft).
5. The toolbar shows a Map tab; Edit Source on an authored asset adds the
   asset tab, active, with the session banner buttons intact.
6. Editing the asset marks the tab `*`; clicking Map opens the existing
   Discard Asset Edit modal; Keep Editing returns; Discard lands on Map with
   the camera restored.
7. `x` on a clean asset tab closes the session directly.
8. During Play both tabs and the panel controls are inert.
9. View > Tool Settings hides/shows the panel; Reset Layout restores it.

Do not commit an interactive repair until the user reports the observed
issue.

## Stop Conditions

Stop without improvising when:

- the baseline merge conflicts or the baseline gate is red;
- a Baseline Truth is false (report the exact file and symbol);
- the draft-validation logic cannot be shared without changing overlay
  behavior;
- the four `commitOptions` side effects cannot be reproduced through the
  dispatcher without new kernel work;
- the payload variant or command frame cannot carry `CreativeToolSettings`
  by value within existing size/discipline constraints;
- tab behavior would require a command or session state that does not
  exist.

## Completion Brief

Report: commit hash; exact files created/modified with line counts;
interactions delivered; reconciliation proof for both matrix rows; commands
and tests run with exact results; manual visual checkpoint requested;
deviations, STOPs, and remaining risks.

Do not start UI-6 trust repairs, the command palette, overlays, or
imported-asset UI after this batch.
