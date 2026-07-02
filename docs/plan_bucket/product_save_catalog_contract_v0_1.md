# Product Save Catalog Contract v0.1

## Objective

Define the next foundation layer after durable save/load, soft delete, and
recovery: a product save catalog that gives the starter menu, Continue, Load
Save, Delete Save, and Recover Save one consistent view of save identity.

The catalog is not a new gameplay authority. It is a rebuildable product index
derived from `.iggy3d.save` files, snapshot sidecars, and compatibility checks.

This is a plan only. It does not authorize source edits.

## Locked User Decisions

World title:

- world creation asks the user for a world title or chapter name;
- code stores the canonical field as `worldTitle`;
- UI may label the field `World Title` or `Chapter Name`;
- v0.1 does not create a separate `chapterTitle`;
- the initial save default title is exactly `worldTitle`;
- manual saves use `worldTitle` as the default display title until user rename
  exists later.

Save list shape:

- Load Save shows one active compatible/incompatible/corrupt save list for now;
- rows carry `worldId` so future grouping by world can be added without changing
  save identity;
- deleted saves live in a separate deleted/recover surface.

Continue:

- Continue selects the newest compatible, non-deleted, loadable save;
- newest means greatest `savedAtUtc`;
- ties are resolved by highest lexicographic `saveId`;
- deleted saves never count for Continue.

Time:

- product save metadata stores UTC strings;
- receipts print raw UTC strings;
- local-time display formatting is a later view concern.

Snapshots:

- snapshot sidecars remain next to save files;
- snapshot capture/write failure does not fail the save;
- missing, empty, corrupt, or unavailable snapshot sidecars produce fallback
  presentation only.

Deleted saves:

- Delete Save is recoverable soft delete;
- Recover Save rejects active target collisions;
- no overwrite-on-recover behavior exists in v0.1;
- permanent delete remains out of product scope until a separate deleted-save
  cleanup surface is approved.

Format:

- do not add JSON;
- do not add an on-disk catalog cache in v0.1;
- if a cache is needed later, it must be explicitly approved as a separate
  engine-owned text format and remain rebuildable.

## Current Baseline

Existing files that already solve part of this:

```text
src/runtime/save/SaveEnvelope.hpp
src/runtime/save/SaveCodec.hpp
src/runtime/save/SaveCodec.cpp
src/runtime/save/SaveFileStore.hpp
src/runtime/save/SaveFileStore.cpp
src/app/iggy3d/SaveBridge.hpp
src/app/iggy3d/SaveBridge.cpp
src/app/iggy3d/ProductWorldCreation.hpp
src/app/iggy3d/ProductWorldCreation.cpp
src/app/frontend/SaveSlotModel.hpp
src/app/frontend/SaveSlotModel.cpp
src/app/frontend/SaveBrowser.hpp
src/app/frontend/SaveBrowser.cpp
```

Current useful behavior:

- `SaveEnvelopeMetadata` stores schema, runtime version, package id, scenario
  id, tool id, and save hash proof;
- `SaveFileStore` owns save paths, durable temp write/readback, final commit,
  soft delete, recovery, and snapshot sidecar path policy;
- `SaveBridge` owns product scan/write/load/soft-delete/recover adapters;
- `ProductWorldCreation` already carries world id, title, save type, and
  initial save title information as product-side proof. Any legacy source
  member name for the title must map to canonical `worldTitle` at the catalog
  boundary;
- `SaveSlotModel` decodes files into visible rows and keeps corrupt rows
  visible but disabled;
- `SaveSlotModel` already detects snapshot sidecars and labels missing, empty,
  unavailable, and available snapshot states;
- product smokes prove write, load, soft delete, recovery, and snapshot sidecar
  movement.

Current gaps:

- product identity fields are not persisted in `SaveEnvelopeMetadata`;
- `ProductSaveWriteRequest::worldId`, `saveType`, and `autoTitle` are proof-only
  bridge fields, not durable save metadata yet;
- save rows still fall back to filename-derived ids and file-time labels;
- Continue currently uses active save ordering, not an explicit saved-time
  policy;
- active and deleted save browser state still has AppShell-local pieces;
- there is no product catalog model that can be unit-tested independently.

## Data Ownership

Runtime save truth:

```text
.iggy3d.save
SaveEnvelope
SaveEnvelopeMetadata
SessionState
SaveAuthoredRoomSection
```

The save envelope remains the authoritative gameplay state and must contain the
product identity needed to rebuild catalog entries.

Product save catalog:

```text
ProductSaveCatalog
ProductSaveCatalogEntry
ProductSaveCatalogBuildResult
```

The catalog is an in-memory product summary built from active and deleted save
folders. It owns sorting, grouping-ready identity, Continue selection, row
eligibility, and browser summary fields. It does not mutate gameplay state.

Snapshot sidecars:

```text
<save-root>/<save-id>.snapshot.png
<save-root>/deleted/<save-id>.snapshot.png
```

Snapshots are presentation data. They move with soft delete and recovery when
present. They never decide whether a save is loadable.

Frontend save browser:

```text
SaveBrowserModel
VerticalSelectorState
Product save browser view model
```

Frontend models own row focus, visible selector entries, fade behavior, selected
row presentation, and user-visible disabled reasons. They do not parse save
files directly.

App shell:

`AppShell.cpp` should only compose product lifecycle:

- scan/build catalog;
- ask catalog policy for Continue or selected row;
- call product save bridge for load/delete/recover/write;
- update receipt state.

AppShell must not own catalog sorting, Continue policy, title fallback rules, or
snapshot status semantics.

## Durable Metadata Fields

The save envelope should grow product metadata in a controlled runtime-save
packet. Proposed fields:

```text
saveId
worldId
worldTitle
saveTitle
saveType
createdAtUtc
savedAtUtc
```

Semantics:

- `saveId` must match the active filename id for a healthy save;
- filename/save-id mismatch produces a disabled row reason, not silent repair;
- `worldId` identifies the world lineage;
- `worldTitle` is the user-entered title or chapter name from world setup;
- `saveTitle` is optional user override, empty until rename exists;
- display title is `saveTitle` when present, otherwise `worldTitle`;
- `saveType` is lower snake text: `initial`, `manual`, `autosave`,
  `quicksave`;
- `createdAtUtc` is the first creation timestamp for that save id;
- `savedAtUtc` updates on every successful write.

For v0.1, initial save uses:

```text
saveType=initial
worldTitle=<user-entered-title>
saveTitle=
displayTitle=<worldTitle>
```

Pause Save and Save And Exit use:

```text
saveType=manual
displayTitle=<saveTitle-or-worldTitle>
savedAtUtc=<current-utc>
```

Autosave and quicksave are deferred, but the enum/string space should reserve
them now.

## Catalog Entry Semantics

Suggested value type:

```text
ProductSaveCatalogEntry
  saveId
  path
  location=active|deleted
  worldId
  worldTitle
  saveTitle
  displayTitle
  saveType
  createdAtUtc
  savedAtUtc
  packageId
  scenarioId
  currentTick
  savedStateHashHex
  authoredFloorCount
  authoredWallCount
  snapshotPath
  snapshotStatus
  snapshotAvailable
  compatible
  loadable
  corrupt
  deleted
  recoverable
  disabledReason
```

Eligibility:

- `loadable=true` only for active, decoded, compatible, non-corrupt saves;
- `recoverable=true` only for deleted entries whose active target save id does
  not collide;
- corrupt active entries remain visible and deletable;
- corrupt deleted entries remain visible and may be recoverable as files, but
  not loadable until recovered and decoded successfully;
- missing snapshot sets `snapshotStatus=missing`, not `loadable=false`.

Legacy fallback:

- if product metadata is missing, row may still be visible;
- `displayTitle` falls back to filename-derived `saveId`;
- `savedAtUtc` falls back to the current `file_time_*` label only for legacy
  compatibility rows;
- legacy rows are not preferred by Continue when valid UTC metadata exists.

## Catalog Build Semantics

Suggested pure functions:

```text
buildProductSaveCatalog(request)
selectProductContinueSave(catalog, policy)
buildProductSaveBrowserRows(catalog, mode)
findProductSaveCatalogEntry(catalog, saveId, location)
canLoadProductSave(entry)
canSoftDeleteProductSave(entry)   # REMOVED as of sd6 (user decision: unknown intent, deleted)
canRecoverProductSave(entry)      # REMOVED as of sd6 (user decision: unknown intent, deleted)
```

Request inputs:

```text
saveRoot
expectedPackageId
expectedScenarioId
includeActive
includeDeleted
```

Result fields:

```text
ok
status
reasonCode
activeCount
deletedCount
compatibleActiveCount
recoverableDeletedCount   # REMOVED as of sd6 (user decision: unknown intent, deleted)
corruptCount
entries
```

Sorting:

- active Load Save rows sort by `savedAtUtc` descending, then `saveId`
  descending;
- deleted Recover rows use the same order within deleted entries;
- incompatible/corrupt rows remain visible in the same order but disabled;
- if metadata is missing, fallback time sorts behind valid UTC rows.

Compute costs:

- v0.1 catalog build is `O(n)` file scan plus `O(total save bytes)` decode;
- sorting is `O(n log n)`;
- snapshot checks are cheap `exists`/`file_size` calls per row;
- this is acceptable for the shippable demo and internal tools;
- if save counts grow large enough for scans to matter, add an explicit
  rebuildable cache later instead of weakening save truth now.

## Continue Policy

Continue must not depend on directory iteration order.

Policy:

```text
location == active
loadable == true
compatible == true
deleted == false
max(savedAtUtc)
tie-break max(saveId)
```

Result type:

```text
ProductContinueSelectionResult
  selected
  status
  reasonCode
  selectedSaveId
  selectedSavedAtUtc
  consideredCount
  compatibleCount
```

Reason codes:

```text
continue_save_selected
continue_no_active_saves
continue_no_compatible_saves
continue_catalog_unavailable
```

## Browser UX Model

The visual save browser should use catalog-backed rows, not raw filesystem
records.

Each row displays:

```text
displayTitle
savedAtUtc display label
saveType label
snapshot image or fallback
worldTitle
compatibility/disabled reason when disabled
```

V0.1 screens:

- Load Save: active entries only;
- Delete Save: active entries only, including corrupt disabled-load rows;
- Recover Save: deleted entries only;
- Continue: no list, uses policy result.

World grouping is deferred. The row already carries `worldId` and `worldTitle`
so grouping can be added as a presentation mode later.

## Coding Methods

Use typed model-first code:

- new catalog model files own catalog entries, policy, sorting, and row
  eligibility;
- source tests build catalog inputs and assert policy results before AppShell
  wiring;
- AppShell only calls catalog helpers and records result fields;
- SaveBridge continues to own product IO adapters;
- SaveFileStore continues to own path, temp, final commit, soft delete, and
  recovery primitives.

Use policy functions instead of if/else chains:

- `selectProductContinueSave` contains Continue policy;
- `buildProductSaveBrowserRows` contains row filtering/sorting;
- `canLoadProductSave` contains load eligibility (`canSoftDeleteProductSave` and
  `canRecoverProductSave` were REMOVED as of sd6 — user decision: unknown intent, deleted;
  recover-collision policy stays enforced at the store level);
- menu routing consumes policy results instead of duplicating conditions.

Use result structs with reason codes:

- every operation returns `ok`, `status`, `reasonCode`, and proof fields;
- receipts report these fields directly;
- tests assert reason codes, not incidental UI text.

Why:

- save/load behavior becomes auditable without launching a window;
- future world grouping, autosave, quicksave, and chapters attach to the same
  identity model;
- AppShell stops accumulating save-browser policy branches;
- corrupt, incompatible, deleted, and legacy rows stay visible without becoming
  loadable by accident.

## Proposed File Paths

First model packet:

```text
src/app/iggy3d/ProductSaveCatalog.hpp
src/app/iggy3d/ProductSaveCatalog.cpp
tests/unit/product_save_catalog_tests.cpp
CMakeLists.txt
cmake/iggy3d_tests.cmake
```

Later metadata persistence packet:

```text
src/runtime/save/SaveEnvelope.hpp
src/runtime/save/SaveCodec.hpp
src/runtime/save/SaveCodec.cpp
tests/unit/save_codec_tests.cpp
tests/unit/save_file_store_tests.cpp
```

Later bridge/app packets:

```text
src/app/iggy3d/SaveBridge.hpp
src/app/iggy3d/SaveBridge.cpp
src/app/iggy3d/ProductWorldCreation.hpp
src/app/iggy3d/ProductWorldCreation.cpp
src/app/iggy3d/AppShell.cpp
src/app/iggy3d/ReceiptBuilder.hpp
src/app/iggy3d/ReceiptBuilder.cpp
src/app/frontend/SaveBrowser.hpp
src/app/frontend/SaveBrowser.cpp
tests/unit/product_save_bridge_tests.cpp
tests/unit/product_world_creation_tests.cpp
tests/unit/save_browser_tests.cpp
tests/smoke/product_automation_menu_smoke.cpp
tests/smoke/product_menu_transition_smoke.cpp
```

## Receipt Fields

Catalog receipts:

```text
save_catalog_status
save_catalog_reason_code
save_catalog_active_count
save_catalog_deleted_count
save_catalog_compatible_active_count
save_catalog_recoverable_deleted_count   # REMOVED as of sd6 (never implemented; user decision: deleted)
save_catalog_corrupt_count
save_catalog_policy
```

Continue receipts:

```text
continue_selection_status
continue_selection_reason_code
continue_selected_save_id
continue_selected_saved_at_utc
continue_selection_policy=newest_compatible
continue_considered_count
continue_compatible_count
```

Selected row receipts:

```text
save_browser_mode=load|delete|recover
save_browser_selected_save_id
save_browser_selected_world_id
save_browser_selected_world_title
save_browser_selected_display_title
save_browser_selected_save_type
save_browser_selected_saved_at_utc
save_browser_selected_snapshot_status
save_browser_selected_loadable
save_browser_selected_recoverable
save_browser_selected_disabled_reason
```

World creation receipts should eventually prove:

```text
world_creation_world_title=<user-title>
world_creation_initial_save_title=<same-title>
```

Receipts remain deterministic key-value text. Do not add JSON.

## Test Plan

Unit tests for `ProductSaveCatalog`:

- builds empty catalog;
- builds active catalog from compatible saves;
- builds deleted catalog separately from active catalog;
- missing snapshot does not disable load;
- corrupt save remains visible and disabled;
- incompatible package/scenario remains visible and disabled;
- display title prefers `saveTitle`, then `worldTitle`, then legacy `saveId`;
- initial save title defaults to `worldTitle`;
- Continue selects newest compatible active save by `savedAtUtc`;
- Continue tie-breaks by highest `saveId`;
- Continue ignores deleted saves;
- recovery eligibility rejects active target collision;
- row sort is deterministic.

Runtime save metadata tests:

- encode/decode preserves `saveId`, `worldId`, `worldTitle`, `saveTitle`,
  `saveType`, `createdAtUtc`, and `savedAtUtc`;
- old saves without product metadata remain readable;
- metadata missing fields produce catalog fallback statuses, not decode failure.

Product bridge/app smokes:

- New World with title creates initial save with matching display title;
- Continue chooses newest compatible save, not last scanned file;
- Load Save row receipt exposes title/time/snapshot status;
- deleted Recover view lists deleted entries separately;
- recover collision remains rejected;
- no-window receipts prove all of the above.

No window proof is required by default.

## Implementation Slices

1. Product Save Catalog / Slice 1 - Model And Continue Policy
   - Add product catalog value types and pure policy tests.
   - No AppShell wiring, no save schema change.

2. Runtime Save Metadata / Slice 1 - Product Metadata Fields
   - Persist product identity in `SaveEnvelopeMetadata`.
   - Keep old saves readable.

3. Product Save Bridge / Catalog Metadata Write
   - Thread `worldTitle`, `saveType`, `createdAtUtc`, and `savedAtUtc` through
     product durable writes.
   - Initial save default title equals `worldTitle`.

4. Product Save Catalog / Slice 2 - Scan From Active And Deleted Saves
   - Build catalog from decoded saves and sidecar snapshot status.
   - Keep active/deleted counts separate.

5. Product App Continue / Catalog Selection
   - Replace scan-order Continue with catalog newest-compatible policy.

6. Product Save Browser / Catalog Rows
   - Feed Load/Delete/Recover selectors from catalog-backed rows.

7. Product Save Catalog / Receipts And Docs
   - Add stable receipt proof and update plan docs after implementation.

## Acceptance Gate

The catalog target is ready to build when the packet states:

- no JSON;
- no on-disk catalog cache in v0.1;
- catalog is rebuildable from save files and sidecars;
- `worldTitle` is the canonical product title;
- initial save default display title equals `worldTitle`;
- Continue uses newest compatible active save by UTC timestamp;
- deleted saves are separate from active saves;
- snapshot failure is non-fatal;
- AppShell does not own catalog policy;
- tests are unit-first, then no-window smokes.

## Stop Rules

Stop and ask for review if implementation requires:

- a new on-disk catalog/cache format;
- JSON;
- permanent delete UI;
- snapshot capture;
- world grouping UI;
- chapter metadata separate from `worldTitle`;
- broad AppShell refactor;
- renderer/window proof;
- changing save load compatibility in a way that breaks old saves.

## Open Questions

None for v0.1.
