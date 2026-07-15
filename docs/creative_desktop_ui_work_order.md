# Creative Desktop UI — Current Work Order

> **This file holds only the CURRENT Claude batch.** Replace it after acceptance.
> Governance: Ace owns product layout, interaction laws, command/kernel
> requirements, slice boundaries, stop conditions, acceptance tests, and diff
> review. Claude implements *only this order*, runs the specified headless
> tests, commits **one coherent batch of its own hunks**, and reports deviations
> instead of redesigning. Visual approval only at UI milestones (this batch is
> headless — no visual milestone).

## Batch: Step 3 — Desktop Command Expansion

**Position:** Foundation Closure ✅ (`1aa72188`) → Workspace Skeleton ✅
(`d228cb63`) → **Step 3 (this)** → Step 4 First Functional Panels.

**Goal.** Extend the headless dispatcher with **typed payloads** for the command
families the functional panels (Step 4+) will emit, so widgets can depend on a
stable, typed contract *before* any widget exists. Build and test it headlessly.

**Hard constraint (Ace).** Do NOT expand the current generic `std::string arg`
into an untyped catch-all. Payloads are typed per command family.

---

### Command contract (the typed payload design)

Today (`EditorDesktopCommands.hpp`): `CreativeDesktopCommand { CreativeDesktopCommandId id; std::string arg; }` in a bounded fixed-size `CreativeDesktopCommandFrame`.

Replace `std::string arg` with a **discriminated typed payload** — one small POD
struct per family, carried in the command entry (a `std::variant` of the payload
structs, or a tagged union; keep the frame fixed-capacity and copyable). The
dispatcher switches on the id and reads the matching payload; a mismatched
id/payload is a no-op returning an explicit failure result, never a
reinterpretation. Keep the existing `CreativeDesktopCommandResult` (extend with
per-family facts as needed for the status bar / tests).

Ids/payloads to add (kernels named; **[exists]** = reuse, **[create]** = new
kernel this batch):

| Family | Command id(s) | Payload | Kernel |
|---|---|---|---|
| Selection | `SelectObjects`, `ClearSelection` | object-id list + primary id | **[create K-1]** `Facade::selectTargets(span<TargetRef>, primary)` wrapping `setSelectedTargets` (tools/Select.hpp:59). **Resolve the `TargetRef.value` uint32 vs `CreativeObjectId` uint64 truncation at this boundary.** |
| Multi-delete | `DeleteObjects` | object-id list (or "current selection") | **[create K-2]** multi-object delete with history in `EditorEdits.*` (pattern: `cutSelectedObjectsToClipboard` minus clipboard). Handle `CreativeDocumentRemoveStatus::ParentHasChildren` for group roots. |
| Rename | `RenameObject` | object id + new name | **[create K-3]** history-wrapped wrapper in `EditorEdits.*` over `renameDocumentObject` (DocumentMutation.hpp:159, currently no app caller). |
| Visibility | `SetObjectsVisible` | object id(s) + bool | **[exists]** `toggleSelectedObjectVisibilityWithUndo` (EditorEdits.hpp:56) — extend to a set-visible form or drive from selection. |
| Lock | `SetObjectsLocked` | object id(s) + bool | **[exists]** `toggleSelectedObjectLockedWithUndo` (EditorEdits.hpp:62). |
| Absolute transform | `SetObjectTransform` | object id + position/rotation/scale + which-components mask | **[create K-7]** `makeScalePayload` / `makeSetTransformPayload` (Mutation.hpp — `makeMovePayload`/`makeRotatePayload` already exist at :355-356) → `applyDocumentMutation(facade.documentForPersistence(), …, SetTransform/…)` via the transaction pattern (precedent EditorPathEditing.cpp:90). |
| Asset ops | `EquipAsset`, `EditAssetSource`, `RenameAsset`, `DuplicateAsset`, `DeleteAsset` | asset id (+ new name for rename) | **[exists]** EditorAssetLibrary kernels (`begin/save/cancelCreativeEditorAuthoredAssetEdit`, `rename/duplicate/deleteCreativeEditorAuthoredAsset`) + **[create K-6]** equip helper extracted from EditorObjectActions.cpp:436-448. |
| Instance refresh | `RefreshInstances`, `UpdateAssetFromInstance` | instance root id + `CreativeAuthoredAssetRefreshMode` (SelectedInstance/SafeInstances/ForceAll) | **[exists]** `refreshCreativeEditorAuthoredAssetInstances` (EditorAuthoredAssets.hpp:128), `updateCreativeEditorAuthoredAssetFromInstance` (:122). |

All mutations run through the transaction discipline (`beginEditTransaction` →
receipt-returning kernel → `completeEditTransaction`) and stay history-wrapped.
Every dispatcher entry resolves `activeCreativeEditorAppState(editor, appState)`
first (asset-edit sessions must not corrupt the map document).

---

### Files

**Create:** none required (kernels land in their existing homes). If a payload
header grows large, `apps/iggy3d_creative/EditorDesktopCommandPayloads.hpp` is
acceptable.

**Modify:**
- `apps/iggy3d_creative/EditorDesktopCommands.hpp` / `.cpp` — typed payloads + dispatch for the new families.
- `src/app/iggy3d/creative/Facade.hpp` + `FacadeObjectCommands.cpp` — K-1 `selectTargets` (public setter; uint32/64 boundary).
- `apps/iggy3d_creative/EditorEdits.hpp` / `.cpp` — K-2 multi-delete, K-3 history-wrapped rename.
- `src/app/iggy3d/creative/mutation/Mutation.hpp` (+ `.cpp`) — K-7 `makeScalePayload` / `makeSetTransformPayload`.
- `apps/iggy3d_creative/EditorObjectActions.*` — K-6 equip helper (extract).
- `tests/unit/creative_desktop_ui_command_tests.cpp` — new command-family tests.

**Prohibited:**
- No widgets/panels/rendering (that is Step 4). No ImGui in this batch.
- No widget-side mutation; `EditorDesktopCommands.cpp` stays the sole dispatcher.
- **Never touch or stage Codex's asset/content lane** — `src/content/assets/StaticMeshAsset*`, `StaticMeshAuthoringMetadata*`, `src/app/iggy3d/creative/adapters/RoomBake*`, `src/app/iggy3d/creative/input/Catalog.cpp`, `tools/blender/*`, `assets/creative/homestead/*`, `docs/creative_assets/*`, `tests/unit/{static_mesh_asset,creative_catalog,creative_asset_room_bake}_tests.cpp`. Commit only Claude's own hunks (`git commit -- <explicit paths>`).
- Never stage the two dirty `.claude/worktrees/*` gitlinks.

---

### Tests (headless — the whole point of doing this before widgets)

Extend `creative_desktop_ui_command_tests.cpp`, one case per family:
- Selection round-trips (single + multi + clear), **incl. the uint32/uint64 id boundary** (K-1).
- Multi-delete removes every selected object (no survivors); group-root `ParentHasChildren` policy honored.
- Rename changes the name + records one history step.
- Visibility / lock toggle the flags with history.
- Absolute transform sets position/rotation/scale via the mutation pipeline; receipt reflects the change; component mask respected.
- Instance refresh: SelectedInstance / SafeInstances / ForceAll dispatch to the right mode; UpdateAssetFromInstance.
- Asset ops: equip, edit-source begin/save/cancel, rename, duplicate, reference-blocked delete.
- Payload discipline: a command with a mismatched payload no-ops with a failure result (no reinterpretation, no string catch-all).

**Gate:** `cmake --build build --target i3dc creative_desktop_ui_command_tests` + `ctest -R creative_desktop_ui_command_tests` + full `-L unit` + `-L smoke` + the T-0 `creative_capture_stability_smoke` (**hash MUST stay `556db015…` byte-identical** — this batch adds no rendering).

**Manual check:** none (fully headless). The panels that emit these commands land in Step 4, where the visual milestone happens.

---

### STOP conditions
Stop after the typed command families + their kernels + tests are green. Commit
one batch of Claude's own hunks. Present the diff for Ace's review. **Do NOT**
start Step 4 (First Functional Panels) or build any widget.
