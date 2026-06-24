# Runtime Save Durability Contract v0.1

## Objective

Define the runtime save durability plan before product world creation,
manual save, autosave, overwrite, soft delete, or Continue depends on save
files as a product surface.

The immediate durability goal is:

```text
never replace a previous valid save with a partial, corrupt, or unvalidated
new save file
```

This v0.1 contract is about safe file-store behavior. It is not a full
power-loss guarantee yet. If the project later requires strict crash/power-loss
durability, that should become a separate platform-sync packet with explicit
file and directory sync behavior.

This is a plan only. It does not authorize source edits.

## Current Baseline

Existing save files:

```text
src/runtime/save/SaveEnvelope.hpp
src/runtime/save/SaveCodec.hpp
src/runtime/save/SaveCodec.cpp
src/runtime/save/SaveCompatibility.hpp
src/runtime/save/SaveCompatibility.cpp
src/runtime/save/SaveLoad.hpp
src/runtime/save/SaveLoad.cpp
src/runtime/save/SaveFileStore.hpp
src/runtime/save/SaveFileStore.cpp
src/app/iggy3d/SaveBridge.hpp
src/app/iggy3d/SaveBridge.cpp
src/app/frontend/SaveSlotModel.hpp
src/app/frontend/SaveSlotModel.cpp
src/app/frontend/SaveBrowser.hpp
src/app/frontend/SaveBrowser.cpp
```

Existing tests:

```text
tests/unit/save_load_tests.cpp
tests/unit/save_file_store_tests.cpp
tests/unit/save_slot_model_tests.cpp
tests/unit/save_browser_tests.cpp
tests/smoke/package_visual_editor_save_load_smoke.cpp
```

Existing capabilities:

- `SaveEnvelope` captures gameplay/session state;
- `SaveCodec` encodes and decodes key-value save envelopes;
- `SaveLoad` can save session state and load encoded saves into a session;
- `SaveCompatibility` checks package/scenario/schema/runtime compatibility;
- `SaveFileStore` lists, reads, writes, and deletes `.iggy3d.save` files;
- `SaveSlotModel` can detect snapshot sidecars as presentation data;
- corrupt saves are visible as disabled frontend rows.

Current limitations:

- `writeSessionSaveFile` writes directly to the final save path with
  `std::ofstream`;
- current write does not write a same-directory temp file first;
- current write does not re-read/decode/validate bytes from disk before commit;
- current overwrite can replace the final path directly;
- current delete uses hard `std::filesystem::remove`;
- current save file store has no soft-delete/recover primitive;
- save catalog/index is not implemented;
- snapshot capture is not implemented;
- snapshot sidecar detection is presentation-only and should stay non-blocking.

## Source-Fit Notes For Builders

These notes describe current source details that later durability slices must
fit without changing behavior accidentally.

- `SaveFileStore.cpp` already has private helpers for save id validation,
  generated ids, final save paths, file extension checks, and path-derived
  records. Slice 1 may promote or mirror path helpers, but it must not change
  current `writeSessionSaveFile`, `readSaveFile`, `listSaveFiles`, or
  `deleteSaveFile` behavior unless a later slice explicitly authorizes that.
- `readSaveFile(tempPath)` can be useful for structural temp validation, but
  its `SaveFileRecord.id` is derived from the temp filename. Temp validation
  must not treat that temp-derived id as the durable product `save_id`; the
  requested save id/final path remains authoritative.
- Existing `SaveFileWriteResult` and reason strings may remain as compatibility
  behavior while durable result structs are introduced beside them. Do not
  silently rename existing public reasons before the durable write path is
  actually migrated and tested.
- Current `SaveSlotModel` has a private snapshot sidecar path helper for
  presentation. A future runtime/file-store helper may centralize sidecar path
  derivation, but snapshot availability remains presentation-only and must not
  block load.

## Relationship To Other Contracts

This contract supports:

```text
docs/plan_bucket/save_load_ux_contract_v0_1.md
docs/plan_bucket/world_creation_ux_contract_v0_1.md
docs/plan_bucket/save_snapshot_contract_v1.md
```

Save/load UX depends on safe overwrite, soft delete, and recoverable failures.

World creation depends on the hard gate:

```text
create world -> create session -> write initial manual save -> enter gameplay
```

That gate is only trustworthy after initial-save writes use the durable write
sequence described here.

## Definitions

Save truth:

```text
.iggy3d.save file containing an encoded SaveEnvelope
```

Final path:

```text
<save-root>/<save-id>.iggy3d.save
```

Temp path:

```text
same directory as final path
hidden or suffixed temp name unique to the write attempt
```

Backup path:

```text
optional previous-save protection path used only if replace-in-place cannot be
made safe on the current platform
```

Snapshot sidecar:

```text
<save-root>/<save-id>.snapshot.png
```

Catalog/index:

```text
rebuildable product cache, never gameplay truth
```

## Durability Level

V0.1 guarantees:

- final save is written only after encoded bytes are written to temp;
- temp save is read and decoded before final commit;
- previous valid save is preserved if temp write or temp validation fails;
- final commit reports success only after the final path exists and can be
  read/decode-validated;
- snapshot failure does not fail the save;
- catalog/index failure does not fail the save.

V0.1 does not guarantee:

- survival of every possible power loss timing;
- directory entry durability after system crash;
- platform-specific fsync behavior;
- cloud sync conflict resolution;
- multi-process save contention.

Power-loss-grade durability requires a later platform packet.

## Coding Method

### Small File-Store Primitives

Keep filesystem durability in `SaveFileStore.*`.

Preferred helper shape:

```text
buildSaveEnvelopeForWrite(...)
encodeSaveForWrite(...)
writeTempSaveFile(...)
validateEncodedSaveFile(...)
commitTempSaveFile(...)
cleanupTempSaveFile(...)
```

Why:

- save safety belongs near file-store behavior;
- product UI should not know temp-file rules;
- tests can use isolated filesystem temp roots directly.

### Explicit Result Structs

Use result structs, not exceptions.

Possible result:

```text
DurableSaveWriteResult
  ok
  reason
  root
  id
  final_path
  temp_path
  backup_path
  encoded_bytes
  previous_existed
  previous_preserved
  temp_written
  temp_closed
  temp_validated
  committed
  final_validated
  snapshot_requested
  snapshot_written
  catalog_update_requested
  catalog_updated
```

Why:

- the repo already uses explicit save/result structs;
- receipts can copy fields directly;
- failure paths stay testable.

### Atomic Commit Pattern

Durable write sequence:

```text
build envelope
encode envelope
derive final path
derive same-directory temp path
write temp bytes
flush and close temp stream
read temp bytes from disk
decode and validate temp envelope
commit temp path to final path
read final bytes from disk
decode and validate final envelope
attempt snapshot sidecar
attempt catalog/index update
emit result
```

Why:

- old save survives failed encode/write/validation;
- final path never intentionally receives unvalidated bytes;
- save file truth is proven from disk, not just memory.

### Same-Directory Temp Files

Temp files must live in the same directory as the final save file.

Why:

- avoids cross-device rename failure;
- makes rename/replace behavior as atomic as the platform allows;
- keeps cleanup local to one save root.

Temp name should be deterministic enough for diagnostics and unique enough to
avoid collision:

```text
.<save-id>.iggy3d.save.tmp.<attempt-id>
```

Attempt id can be a counter, timestamp, or random suffix, but tests must be
able to inject a deterministic value.

### Path Helpers With Tests

Path helpers should be centralized:

```text
savePathFor(root, id)
tempSavePathFor(finalPath, attemptId)
snapshotSidecarPath(finalPath)
deletedSavePathFor(finalPath)
```

Why:

- avoids repeated filename logic;
- save, snapshot, soft-delete, and recovery paths stay consistent;
- tests can lock path behavior.

### Preserve Hard Delete As Permanent Primitive

Current `deleteSaveFile(path)` is hard delete.

Do not remove it. It remains useful for:

- tests;
- cleanup;
- future permanent delete from Deleted Saves;
- low-level file-store maintenance.

Add separate product/file-store primitives later:

```text
softDeleteSaveFile(...)
recoverDeletedSaveFile(...)
permanentlyDeleteSaveFile(...)
```

Why:

- starter Delete Save must not hard delete;
- permanent deletion remains explicit;
- recovery is a first-class operation.

### Validate By Re-Reading

Validation must read from disk after write.

Validation should use:

```text
readSaveFile(tempPath)
decodeSaveEnvelope(bytes)
optional expected metadata/hash checks
```

Why:

- proves the bytes on disk are parseable;
- reuses current codec behavior;
- catches failed/empty/truncated temp files before commit.

### Separate Durability From Compatibility

Durability validation:

```text
is this save file readable and structurally valid?
```

Compatibility validation:

```text
should this save load in the current package/scenario/runtime?
```

Keep them separate.

Why:

- a save can be durable but incompatible with the current package;
- frontend rows need to show incompatible saves without treating them as
  corrupt writes;
- save write should not depend on menu filtering.

### Catalog Is Cache

Catalog/index update is after final save commit.

If catalog update fails:

```text
save_written=true
catalog_updated=false
reason_code=catalog_update_failed
```

The save remains valid because the catalog is rebuildable from files.

## Proposed Durable Files

Existing files to extend carefully:

```text
src/runtime/save/SaveFileStore.hpp
src/runtime/save/SaveFileStore.cpp
tests/unit/save_file_store_tests.cpp
```

Possible later app/product files:

```text
src/app/iggy3d/ProductSaveBridge.hpp
src/app/iggy3d/ProductSaveBridge.cpp
src/app/iggy3d/ProductSaveCatalog.hpp
src/app/iggy3d/ProductSaveCatalog.cpp
src/app/iggy3d/ProductSaveSnapshot.hpp
src/app/iggy3d/ProductSaveSnapshot.cpp
```

Do not put renderer, UI, world setup, or starter-menu logic in
`SaveFileStore.*`.

## No-Go Files

Durability foundation slices must not touch:

```text
src/render/**
src/render/vulkan/**
docs/vulkan/**
fixtures/** package schemas
apps/iggy3d_visual_demo/**
src/app/iggy3d/AppShell.cpp
```

Do not add JSON or another external machine-contract format.

## Data Ownership

| Domain | Owns | Must not own |
| --- | --- | --- |
| `SaveEnvelope.*` | gameplay/session save shape | filesystem temp/rename policy |
| `SaveCodec.*` | encode/decode bytes | file replacement policy |
| `SaveLoad.*` | mapping session state to/from envelopes | save root scanning and delete UX |
| `SaveFileStore.*` | save file paths, temp writes, final commit, read/list/delete primitives | starter UI behavior |
| Product save bridge | orchestration, product receipts, save/write requests | low-level codec internals |
| Product save catalog | rebuildable summaries | gameplay truth |
| Product snapshot helper | sidecar request/result | save write success |
| Frontend save browser | row presentation | file mutation |

## Durable Write Control Flow

Write new save:

```text
request received
state pointer validated
envelope built
envelope encoded
root created
final path derived
temp path derived
temp file written
temp file closed
temp file read back
temp envelope decoded
temp envelope validated
temp committed to final
final file read back
final envelope decoded
result ok
```

Overwrite existing save:

```text
previous final exists
new temp written and validated
commit temp to final
if commit succeeds: final read/decode validated
if commit fails: previous final remains loadable
```

If the platform cannot safely replace an existing final path in one commit
operation, implementation must stop and split a platform-specific replacement
strategy. Do not silently use remove-then-rename as the product overwrite path.

## Commit Semantics

Preferred commit:

```text
same-directory atomic rename or replace temp -> final
```

Rules:

- commit must never deliberately remove the previous final path before the new
  temp file is validated;
- commit must not cross filesystem boundaries;
- commit must report whether a previous final existed;
- commit must report whether previous data is expected to be preserved after
  failure;
- commit success requires final read/decode validation.

If the standard library/platform behavior for replacing an existing file is not
clear enough, create a narrow platform helper rather than spreading replacement
logic through product code.

## Failure Semantics

Encode failure:

```text
temp_written=false
committed=false
previous_preserved=true
save_written=false
reason_code=encode_failed
```

Temp write failure:

```text
temp_written=false
committed=false
previous_preserved=true
save_written=false
reason_code=temp_write_failed
```

Temp validation failure:

```text
temp_written=true
temp_validated=false
committed=false
previous_preserved=true
save_written=false
reason_code=temp_validate_failed
```

Commit failure:

```text
temp_validated=true
committed=false
previous_preserved=true
save_written=false
reason_code=atomic_rename_failed
```

Final validation failure:

```text
committed=true
final_validated=false
save_written=false
reason_code=final_validate_failed
```

Snapshot failure:

```text
save_written=true
snapshot_written=false
reason_code=snapshot_failed
```

Catalog update failure:

```text
save_written=true
catalog_updated=false
reason_code=catalog_update_failed
```

## Soft Delete And Recovery

Starter Delete Save must not call hard delete directly.

Soft delete should move active save files to a deleted area:

```text
active save path -> deleted save path
active snapshot sidecar -> deleted snapshot sidecar if present
```

Recovery should move them back:

```text
deleted save path -> active save path
deleted snapshot sidecar -> active snapshot sidecar if present
```

Permanent delete should be a separate explicit operation:

```text
permanentlyDeleteSaveFile(...)
```

Rules:

- soft delete must not corrupt the save file;
- recover must validate target collision before moving back;
- permanent delete must not be reachable from the first Delete Save confirm;
- soft delete failure must leave the active save visible if possible.

Soft delete is not part of the first durability slice unless explicitly scoped.

## Snapshot Sidecars

Snapshot is sidecar presentation only.

Write order:

```text
commit save file first
attempt snapshot sidecar second
```

Rules:

- snapshot failure does not fail save;
- snapshot bytes are not stored in `.iggy3d.save`;
- save load must not require snapshot;
- snapshot path derives from final save path.

Snapshot capture/rendering is outside runtime save durability.

## Catalog/Index Cache

Catalog/index is product cache.

Rules:

- catalog update happens after final save commit;
- catalog update failure does not invalidate save;
- catalog can rebuild by scanning save files;
- runtime save code must not depend on catalog.

Durability slices may define status fields for catalog update, but should not
build the catalog unless separately scoped.

## Reason Codes

Stable reason codes:

```text
ok
state_missing
invalid_source_state
encode_failed
root_create_failed
invalid_save_id
temp_path_failed
temp_write_failed
temp_close_failed
temp_read_failed
temp_validate_failed
atomic_rename_failed
final_read_failed
final_validate_failed
previous_preserve_unknown
snapshot_failed
catalog_update_failed
permission_denied
storage_full
soft_delete_failed
recover_failed
permanent_delete_failed
unknown_error
```

Existing reasons may remain as compatibility aliases during migration, but new
product receipts should use stable reason codes.

## Receipt Fields

Receipts must remain deterministic key-value text.

Durable write receipt:

```text
save_write_requested=true|false
save_write_status=not_requested|ok|failed
save_write_reason_code=<reason>
save_write_id=<id-or-none>
save_final_path=<path-or-empty>
save_temp_path=<path-or-empty>
save_previous_existed=true|false
save_previous_preserved=true|false|unknown
save_encoded_bytes=<count>
save_temp_written=true|false
save_temp_closed=true|false
save_temp_validated=true|false
save_atomic_committed=true|false
save_final_validated=true|false
save_written=true|false
```

Snapshot receipt:

```text
save_snapshot_requested=true|false
save_snapshot_written=true|false
save_snapshot_status=available|missing|failed|skipped
```

Catalog receipt:

```text
save_catalog_update_requested=true|false
save_catalog_updated=true|false
save_catalog_status=not_requested|updated|failed|rebuilt
```

Soft delete receipt:

```text
save_delete_requested=true|false
save_delete_type=soft|permanent|none
save_delete_recoverable=true|false
save_delete_status=not_requested|ok|failed
save_delete_reason_code=<reason>
```

## Test Plan

Unit tests should use isolated temp directories.

Path tests:

- valid id maps to expected final path;
- temp path is same directory as final path;
- temp path does not equal final path;
- snapshot sidecar path matches save path;
- deleted path is separate from active path.

Write tests:

- missing state is rejected before file write;
- new save writes temp, validates temp, commits final;
- final file decodes after commit;
- invalid id is rejected;
- failed encode does not create final;
- temp validation failure does not replace old final;
- overwrite preserves old save when temp validation fails;
- final validation failure reports failure;
- temp cleanup does not remove final;
- deterministic ids still work.

Delete tests later:

- hard delete remains permanent primitive;
- soft delete moves save out of active listing;
- soft delete moves snapshot if present;
- recovery restores save and snapshot;
- permanent delete is separate.

No-window smokes later:

- manual save receipt reports durable write steps;
- initial new-world save receipt reports durable write steps;
- failed save write keeps current session running;
- snapshot failure does not fail save.

No window proof is required for durability foundation.

## First Implementation Slice

Builder-safe first slice:

```text
Runtime Save Durability / Slice 1 - Path And Result Primitives
```

Allowed files:

```text
src/runtime/save/SaveFileStore.hpp
src/runtime/save/SaveFileStore.cpp
tests/unit/save_file_store_tests.cpp
```

Allowed behavior:

- add path helper declarations/definitions if needed;
- add durable write result/request value types if needed;
- add temp path derivation;
- add snapshot sidecar path helper if it belongs in file store;
- add tests for path/result defaults;
- do not change existing write behavior unless the slice explicitly says so.

Explicitly not allowed in Slice 1:

- replacing `writeSessionSaveFile`;
- changing AppShell;
- changing ProductSaveBridge;
- changing save/load UX;
- adding soft delete execution;
- adding snapshot capture;
- changing renderer/Vulkan;
- launching a window;
- adding JSON.

Slice 1 acceptance:

```text
cmake --build build --target iggy3d_app
cmake --build build --target save_file_store_tests
ctest --test-dir build --output-on-failure -R '^save_file_store_tests$'
git diff --check
```

## Later Implementation Order

1. Path helpers and result structs.
2. Temp write helper with close/readback.
3. Temp decode/validation helper.
4. Commit helper with same-directory rename/replace policy.
5. Durable write path beside existing `writeSessionSaveFile`.
6. Migrate product save bridge to durable write path.
7. Add overwrite-preserves-previous tests.
8. Add soft delete/recover helpers.
9. Add catalog/index cache update hooks.
10. Add product receipts.

Do not combine path helpers, durable write replacement, soft delete, catalog,
and product UI wiring in one slice.

## Acceptance Gate

This contract is builder-slice ready when:

- current direct write and hard delete baseline is named;
- v0.1 durability level is defined;
- power-loss caveat is explicit;
- path helpers and result types are separated from behavior migration;
- previous-valid-save preservation is the primary rule;
- snapshot/catalog failure is non-fatal to save truth;
- first implementation slice is path/result only;
- no AppShell, renderer, Vulkan, or JSON work is required.

## Stop Rules

Stop before implementation if a slice requires:

- remove-then-rename overwrite behavior for product saves;
- hard delete as starter Delete Save;
- snapshot required for load;
- catalog/index as gameplay truth;
- renderer/Vulkan changes;
- AppShell/menu changes;
- save schema changes not separately planned;
- window launch by default;
- JSON or another external machine-contract format.

## Open Questions

No blocking user decisions remain for Slice 1.

Deferred engineering decisions:

- whether platform-specific file sync is required for power-loss durability;
- exact attempt id generator for temp filenames;
- whether durable replacement needs a platform abstraction for non-POSIX hosts;
- exact deleted-folder layout after product world grouping lands;
- whether hard delete should remain in runtime store or move behind a product
  permanent-delete wrapper later.

## Long-Term Fit

This contract keeps save durability small and boring:

- runtime save truth stays in `.iggy3d.save`;
- file-store code owns temp/write/commit mechanics;
- product UI requests saves but does not know filesystem details;
- snapshots and catalog remain non-authoritative;
- soft delete and recovery are explicit product behaviors;
- receipts prove each step without a window.
