# Save Load UX Contract v0.1

## Objective

Define the product save/load UX before implementation expands beyond the
current runtime save stack.

The save/load system must let the player create, identify, load, overwrite,
delete, recover, and diagnose worlds without ambiguity.

Every save shown to the player must answer:

```text
what world is this
when was it saved
where was I
what mode or state was active
is it safe to load
```

This is a plan only. It does not authorize source edits.

## Current Baseline

The repo already has a real runtime save/load foundation:

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

Current capabilities:

- encoded runtime save envelope;
- runtime save/load round trip into `Session`;
- compatibility checks for package, scenario, schema, and runtime version;
- save file list/read/write/delete;
- authored room section persistence;
- frontend save slot previews;
- frontend save browser model;
- sidecar snapshot path detection;
- snapshot fallback presentation;
- corrupt save rows remain visible and disabled.

Current limitations to solve:

- save writes use direct final-path output, not temp/validate/atomic rename;
- delete is hard `filesystem::remove`, not soft delete;
- save listing scans a flat folder, not world groups;
- save index/catalog cache does not exist;
- product `world_id` and user-facing `save_id` identity are not modeled yet;
- manual/autosave/quicksave save types are not modeled yet;
- `user_title` and generated `auto_title` are not modeled yet;
- UTC timestamps are not stored as product save metadata yet;
- Continue newest-valid-save policy is not modeled yet;
- snapshot capture does not exist yet, only sidecar presentation detection.

## Source-Fit Notes For Builders

These notes describe the current source behavior that a later implementation
packet must work with. Do not infer product save UX from the existing low-level
helpers alone.

- `SaveEnvelopeMetadata` currently stores schema/runtime/package/scenario/tool
  and saved hash fields. Product identity fields such as `world_id`, `save_id`,
  `save_type`, `user_title`, `auto_title`, and UTC timestamps do not exist yet.
- `writeSessionSaveFile` currently encodes the save and writes it directly to
  the final `*.iggy3d.save` path with an output file stream. It does not write a
  temp file, decode/validate the temp file, or atomically rename into place.
- `deleteSaveFile(path)` currently calls `std::filesystem::remove`. It is a hard
  delete primitive. Starter Delete Save must not call it as the primary product
  action; product delete starts as soft delete/recover, with hard delete reserved
  for an explicit permanent-delete route.
- `scanProductSaves` currently delegates to `buildSaveSlotList`; there is no
  product catalog/index cache and no save mutation policy in `SaveBridge` yet.
- `SaveSlotModel` already keeps corrupt rows visible and disabled, and already
  derives snapshot sidecar status from `<save_id>.snapshot.png`. That snapshot
  state is presentation-only and must not decide whether a save can load.
- `SaveBrowser` currently maps save previews into selector rows and can expose a
  selected corrupt/disabled row for delete. Product delete mode must preserve
  that visibility but route to soft delete, not hard remove.
- Current filename-derived ids are compatibility preview ids only. Product
  `save_id` and `world_id` must become explicit durable identity before menus
  depend on them for Continue, delete, recovery, or world grouping.

## User Decisions

Save identity:

- each save has optional `user_title`;
- each save has generated `auto_title`;
- display `user_title` first when present;
- display `auto_title` as fallback or subtitle.

Snapshot:

- snapshot is a sidecar image next to the save file;
- snapshot captures active gameplay, tactical, or editor camera view;
- pause/menu UI must not appear in the snapshot;
- missing/corrupt snapshot does not block load.

World creation:

- new world creates an initial save immediately;
- no orphan worlds;
- first world setup fields are world name, seed, difficulty, starting scenario.

Delete:

- delete is soft first;
- permanent delete only exists inside deleted-save/deleted-world surfaces.

Continue:

- Continue loads newest valid non-deleted loadable save.

## Save Truth Model

Runtime save file truth stays in:

```text
src/runtime/save/SaveEnvelope.*
src/runtime/save/SaveCodec.*
src/runtime/save/SaveLoad.*
src/runtime/save/SaveFileStore.*
```

The `.iggy3d.save` file is authoritative for loadable gameplay state.

The save selector, save index, thumbnails, and receipt summaries are not
authoritative gameplay truth.

If a selector index is missing or stale, the app must be able to rebuild save
summaries by scanning save files.

If a snapshot is missing or corrupt, the save can still be valid and loadable.

## Product Save Identity

Every product save should expose:

```text
save_id
world_id
save_type
user_title
auto_title
created_at_utc
updated_at_utc
timezone_at_save
game_version
schema_version
minimum_supported_schema
world_name
world_seed
difficulty
scenario
playtime_seconds
world_day
location_or_state
player_position
camera_state
snapshot_path
snapshot_status
integrity_status
load_state
deleted_state
```

Minimum v0.1 product summary:

```text
save_id
world_id
save_type
user_title
auto_title
updated_at_utc
world_name
world_seed
schema_version
snapshot_path
snapshot_status
load_state
deleted_state
```

Display rule:

```text
if user_title exists:
  title=user_title
  subtitle=auto_title
else:
  title=auto_title
  subtitle=none
```

Initial new-world save title:

```text
auto_title={WorldName} - Beginning
```

Later manual save title:

```text
auto_title={WorldName} - Day {WorldDay} - {LocationOrState}
```

Rename Save edits only `user_title`. It must not rewrite generated
`auto_title`.

## Save Types

Supported product save types:

```text
manual
autosave
quicksave
```

V0.1 exposed types:

```text
manual
autosave
```

Reserved but not exposed:

```text
quicksave
```

Caps:

```text
manual saves = unlimited
autosaves per world = 3
quicksaves per world = 1 later
```

Manual saves must never be silently deleted by rotation.

Autosave rotation may delete or archive only older autosaves inside the same
world group.

## World Grouping

Frontend save browsing groups saves by world:

```text
World
  Manual Saves
  Autosaves
  Quicksaves
```

The first product implementation may still store files in a simple folder, but
the model must expose world grouping semantics so the future storage layout can
change without rewriting UI rules.

Recommended future storage shape:

```text
<save-root>/
  worlds/
    <world_id>/
      saves/
        save_0007.iggy3d.save
        save_0007.snapshot.png
      deleted/
        save_0003.iggy3d.save
        save_0003.snapshot.png
```

The exact folder layout may be implemented in a later storage packet. The v0.1
contract requires the model to distinguish active saves from deleted saves and
world group ownership.

## New World Flow

Starter flow:

```text
Main Menu
  -> New World
  -> World Setup
  -> Create
  -> Loading / Generating
  -> Create initial manual save
  -> Gameplay
```

V0.1 world setup fields:

```text
World Name
Seed
Difficulty
Starting Scenario
Create
Back
```

Defaults:

```text
world_name=New World
seed=generated_editable
difficulty=standard
starting_scenario=training_ground
```

Back behavior:

- Back discards draft world setup state;
- Back writes no save;
- Back creates no world folder;
- Back returns to starter.

Create behavior:

1. create `world_id`;
2. create world metadata;
3. create initial manual save;
4. update save summaries/index cache if present;
5. enter gameplay only after the initial save succeeds.

Default policy:

- initial save is required for `New World -> Gameplay`;
- if the initial save fails, remain on world setup or loading error route;
- do not enter gameplay with an orphan world;
- v0.1 has no non-fatal unsaved-world startup exception. If a later packet wants
  an unsaved-world mode, it must define that as a separate explicit product
  mode, not as a fallback inside New World.

## Continue Behavior

Continue is available only when at least one valid non-deleted loadable save
exists.

Continue selection rule:

```text
newest updated_at_utc among valid non-deleted loadable saves
```

Do not prioritize manual/autosave/quicksave ahead of recency for v0.1.

If two saves have the same timestamp, use deterministic tie-break:

```text
world_id ascending
save_id ascending
```

Continue must not choose:

- deleted saves;
- corrupt saves;
- unsupported saves;
- saves with missing required world data;
- rows that are visible only for recovery/diagnosis.

## Save Selector UX

Starter Load Save opens a save selector.

Starter Delete Save opens the same selector in delete mode.

The selector shows:

```text
world title
save title
save subtitle
timestamp
snapshot or fallback
save type
load state
delete/recovery state
```

V0.1 primary actions:

```text
Load
New World
Delete
Back
```

Later secondary actions:

```text
Rename
Duplicate
Show Details
Recover Deleted
Permanent Delete
```

Save selector must use the reusable vertical selector model. It must not own
load/delete execution.

## Snapshot Sidecar

Snapshot policy is defined further in:

```text
docs/plan_bucket/save_snapshot_contract_v1.md
```

Default sidecar path:

```text
save_0007.iggy3d.save
save_0007.snapshot.png
```

Recommended thumbnail:

```text
format=png
aspect_ratio=16:9
size=512x288
fallback=default_world_thumbnail
```

Snapshot statuses:

```text
available
missing
failed
empty
unavailable
outdated
```

Snapshot failure is non-fatal for save write.

Required receipt distinction:

```text
save_written=true
save_snapshot_written=false
save_snapshot_status=failed
reason_code=snapshot_failed
```

## Write Safety

Current `writeSessionSaveFile` writes directly to the final save path. The
product save path must upgrade to a durable sequence before overwrite,
autosave, or product save UI depends on it.

Required product write sequence:

```text
build save envelope
encode save envelope
write temp save file
read/validate temp save file
atomic rename temp save to final save
attempt snapshot sidecar write
update or rebuild save index cache
emit receipt
```

Rules:

- never destroy the previous valid save before replacement succeeds;
- snapshot failure does not fail the save;
- index update failure does not make the save unloadable;
- failed save keeps current session running;
- failed save must not update selector as if it succeeded;
- failed save must emit a stable reason code.

Overwrite semantics:

- overwrite is allowed only when action explicitly targets an existing save;
- overwrite must preserve previous valid save until final atomic replacement;
- failed overwrite leaves the old save loadable.

## Save Index And Catalog

The save index/catalog is a product cache for fast menus.

It may contain:

```text
world_id
save_id
save path
snapshot path
title fields
timestamps
save type
load state
deleted state
last selected world
last selected save
```

It must not be the only source of load truth.

If the index is missing:

```text
scan save files
decode summaries
rebuild index
show selector
```

If the index is corrupt:

```text
ignore index
rebuild index
emit index_rebuilt receipt
```

Runtime save code must not depend on the index.

## Soft Delete And Recovery

Delete is soft first.

Save delete:

```text
active save -> deleted saves
```

World delete:

```text
active world -> deleted worlds
```

Deleting a world soft-deletes all child saves and snapshots.

Recovery restores the save or world group to active browsing.

Permanent delete is available only from deleted-save or deleted-world surfaces.

First delete prompt:

```text
Move this save to Deleted Saves?
```

Permanent delete prompt:

```text
Permanently delete this save? This cannot be undone.
```

V0.1 may implement soft delete by moving files into a deleted folder. It should
not be implemented as a hard remove from the active selector.

Existing `deleteSaveFile(path)` is hard delete and must not become the product
starter Delete Save behavior without a specific permanent-delete route.

## Load Failure

If load fails:

- do not crash;
- do not destroy the current running session unless replacement already
  succeeded;
- return to save selector or save error route;
- mark save as failed, incompatible, corrupt, or missing data;
- show readable user message;
- log technical reason;
- emit receipt.

User-facing messages:

```text
This save was created by a newer version of the game.
This save file appears to be damaged.
This save is missing required world data.
```

Internal reason codes:

```text
schema_too_new
schema_too_old
missing_world_file
checksum_failed
deserialization_failed
decode_failed
compatibility_failed
replacement_failed
unknown_error
```

## Compatibility States

Product load states:

```text
loadable
needs_migration
unsupported_newer_version
unsupported_older_version
corrupt
missing_data
deleted
unknown
```

V0.1 exposed states:

```text
loadable
unsupported
corrupt
missing_data
deleted
unknown
```

Existing runtime compatibility statuses should be mapped into product load
states without changing runtime compatibility semantics.

## Autosave Rules

Autosave should trigger only at safe points.

Good autosave moments:

```text
after world creation
after major world event
after entering or exiting important area
after fixed safe interval
before quitting to menu
```

Bad autosave moments:

```text
during combat resolution
mid-command transaction
while generation is incomplete
while world data is partially loaded
while save/load is already active
```

Autosave must not interrupt gameplay.

Autosave UI:

```text
Saving...
```

Corner-only, temporary, no modal.

## Quicksave Rules

Quicksave is reserved for the data model but not exposed in v0.1.

Later defaults:

```text
F5=quicksave
F9=quickload
controller_quicksave=no_default
controller_quickload=no_default
```

If controller quicksave is added later, it must be a deliberate chord.

Quickload must not be bound to an accidental single button.

## Routing Surfaces

Stable route names:

```text
main_menu
new_world
world_setup
save_selector
save_details
deleted_saves
deleted_worlds
loading_save
saving
save_error
gameplay
```

Parent stack:

```text
main_menu
  -> save_selector
    -> save_details
    -> deleted_saves
  -> world_setup
    -> loading_save
      -> gameplay
```

If a parent surface disappears:

```text
return to nearest valid parent
```

If no valid parent exists:

```text
return to main_menu
```

## Data Ownership

| Domain | Owns | Must not own |
| --- | --- | --- |
| Runtime save envelope | gameplay/session state | menu state, thumbnails, deleted browser state |
| Runtime save codec | encode/decode format | product selector behavior |
| Runtime save/load | session replacement and validation | starter route behavior |
| Runtime file store | raw save file read/write/list primitives | product soft-delete policy unless explicitly extended |
| Product save bridge | save orchestration, summaries, safe writes, load requests | renderer backend resources |
| Product save catalog | rebuildable save/world summaries | authoritative gameplay truth |
| Product snapshot helper | sidecar path, capture request/result, thumbnail status | save codec schema |
| Frontend save selector | row presentation and selected row state | file mutation or session replacement |
| Product frontend router | route requests and parent returns | save file encoding |
| Receipt builder | deterministic proof fields | gameplay mutation |

## Owned Files For Later Implementation

Existing files to extend carefully:

```text
src/runtime/save/SaveFileStore.hpp
src/runtime/save/SaveFileStore.cpp
src/app/iggy3d/SaveBridge.hpp
src/app/iggy3d/SaveBridge.cpp
src/app/frontend/SaveSlotModel.hpp
src/app/frontend/SaveSlotModel.cpp
src/app/frontend/SaveBrowser.hpp
src/app/frontend/SaveBrowser.cpp
```

Likely new product files:

```text
src/app/iggy3d/ProductSaveCatalog.hpp
src/app/iggy3d/ProductSaveCatalog.cpp
src/app/iggy3d/ProductSaveBridge.hpp
src/app/iggy3d/ProductSaveBridge.cpp
src/app/iggy3d/ProductSaveSnapshot.hpp
src/app/iggy3d/ProductSaveSnapshot.cpp
src/app/frontend/WorldSetupModel.hpp
src/app/frontend/WorldSetupModel.cpp
```

Do not add renderer/Vulkan dependencies to runtime save files.

## No-Go Files

Do not touch these for save/load UX unless a later packet explicitly expands
scope:

```text
src/render/**
src/render/vulkan/**
docs/vulkan/**
fixtures/** package schemas
apps/iggy3d_visual_demo/**
```

Do not add JSON or a new external machine-contract format.

## Receipt Fields

Receipts must remain deterministic key-value lines.

Save created:

```text
receipt_type=save_created
save_id=<id>
world_id=<id>
save_type=manual|autosave|quicksave
save_title=<title>
save_auto_title=<title>
save_user_title_present=true|false
save_updated_at_utc=<iso8601-or-none>
save_success=true|false
reason_code=<reason>
```

Save loaded:

```text
receipt_type=save_loaded
save_id=<id>
world_id=<id>
route_before=<route>
route_after=<route>
load_success=true|false
load_state=<state>
reason_code=<reason>
```

Save deleted:

```text
receipt_type=save_deleted
save_id=<id>
world_id=<id>
delete_type=soft|permanent
recoverable=true|false
delete_success=true|false
reason_code=<reason>
```

World created:

```text
receipt_type=world_created
world_id=<id>
world_name=<name>
seed=<seed>
difficulty=<difficulty>
scenario=<scenario>
initial_save_id=<id-or-none>
success=true|false
reason_code=<reason>
```

Snapshot:

```text
receipt_type=snapshot_created
save_id=<id>
snapshot_path=<path-or-empty>
snapshot_width=512
snapshot_height=288
snapshot_success=true|false
snapshot_status=<status>
reason_code=<reason>
```

Index/catalog:

```text
receipt_type=save_catalog
save_catalog_status=ready|rebuilt|missing|corrupt|failed
save_catalog_world_count=<count>
save_catalog_save_count=<count>
save_catalog_deleted_count=<count>
reason_code=<reason>
```

## Reason Codes

Stable reason codes:

```text
ok
disk_write_failed
snapshot_failed
metadata_write_failed
index_update_failed
index_rebuilt
save_not_found
world_not_found
schema_too_new
schema_too_old
checksum_failed
corrupt_data
decode_failed
compatibility_failed
replacement_failed
permission_denied
storage_full
soft_delete_failed
recover_failed
permanent_delete_failed
unknown_error
```

## Test Plan

Existing tests already cover:

- runtime save/load round trip;
- save file write/list/read/delete;
- authored room save persistence;
- frontend slot preview;
- snapshot sidecar fallback;
- save browser selector state.

New unit tests should cover:

- product save title display rule;
- generated auto title;
- UTC timestamp parse/display summary;
- Continue chooses newest valid non-deleted loadable save;
- corrupt saves are visible but not loadable;
- index missing triggers scan/rebuild summary;
- snapshot failure does not fail save;
- soft delete removes from active selector and appears in deleted selector;
- recovery returns a save to active selector;
- permanent delete is separate from soft delete;
- failed overwrite preserves previous valid save;
- temp write failure does not update selector as successful.

No-window smokes should prove:

- new world creates initial save;
- save appears in selector;
- Continue routes to newest valid save;
- manual save emits receipt;
- autosave emits receipt at safe point;
- soft delete emits receipt;
- recovery emits receipt;
- corrupt/missing save remains visible and disabled.

Window proof is not required by default. Snapshot image capture requires a
later explicit visual packet.

## Acceptance Gate

The contract is ready for implementation slicing when:

- save truth, index cache, snapshots, selector, and router ownership are
  distinct;
- current direct write and hard delete limitations are named;
- v0.1 defaults are explicit;
- receipt fields are deterministic key-value text;
- no runtime save schema changes are required for the first frontend slices;
- no builder must invent Continue, delete, title, or snapshot semantics.

## First Implementation Order

Safe order:

1. product save identity and summary structs;
2. title/timestamp/load-state mapping from existing save records;
3. Continue selection policy;
4. world setup model with draft defaults;
5. initial save request model, no AppShell migration yet;
6. safe write helper with temp/validate/atomic rename;
7. soft delete/recover helper;
8. save catalog/index cache, rebuildable from files;
9. autosave policy model;
10. route integration through product frontend router;
11. no-window receipts;
12. visual snapshot capture packet.

Do not implement all of this in one build slice.

## Stop Rules

Stop before implementation if a slice requires:

- changing renderer/Vulkan;
- changing package fixture schemas;
- adding JSON;
- making snapshots required for load;
- hard-deleting saves from the starter delete flow;
- entering gameplay with an orphan new world;
- storing image bytes inside `.iggy3d.save`;
- making the save catalog the only source of load truth;
- launching a window by default;
- broad AppShell migration and save execution in the same slice.

## Open Questions

No blocking user decisions remain for v0.1 planning.

Deferred product flavor decisions:

- exact world-name validation rules;
- exact generated seed format;
- exact difficulty list;
- exact starting scenario list after more content exists;
- final date display localization;
- final thumbnail art for fallback;
- whether permanent delete requires typed confirmation later.

## Long-Term Fit

This contract keeps save/load durable by separating:

- runtime save truth;
- rebuildable save catalog;
- frontend selection;
- snapshot sidecars;
- product routing;
- receipts.

That separation lets the project grow from current flat save files into world
groups, autosaves, soft delete, recovery, thumbnails, and future migration
without turning menus into save logic or runtime saves into UI state.
