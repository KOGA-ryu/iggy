# NPC Actor Save/Load Ownership Boundary

This note records the first save/load ownership boundary for the newer
`scene/npc` actor and control registries.

## Current Boundary

`RuntimeSessionSnapshot` remains session-only. It captures the session-owned
runtime state:

- level runtime state
- tick index
- player state and player presence

`RuntimeGameplaySnapshot` is the future gameplay-level packet. It is separate
from the existing session snapshot and is intended to hold:

- a `RuntimeSessionSnapshot`
- command queue state
- interaction state
- inventory state
- `NpcActorState2DRegistry`
- `NpcActorControlState2DRegistry`

The new NPC actor/control registries are gameplay-owned. They must not be added
to `RuntimeSessionState` or `RuntimeSessionSnapshot` in this lane.

## Legacy NPC Data

Existing `LevelRuntimeState::npcAgents` and the older `modules/npc_ai` save path
are legacy/session-level level data. They should not be conflated with the newer
`scene/npc` actor/control registries.

The new registries model actor identity/current actor facts and actor-carried
control facts. They are intentionally owned beside runtime gameplay state, not
inside the session snapshot.

## Unsaved Derived Facts

Derived facts remain unsaved and rebuildable:

- NPC actor occupancy
- movement plans
- route targets
- navigation requests
- path reports
- path steps
- occupancy filters
- refresh work
- movement frame reports
- render packets
- debug packets

These should be regenerated from authoritative actor/control/map/session state.

## Archive-Level Gameplay Codec

`RuntimeGameplaySnapshotChunkCodec` is the first archive-level codec for the
gameplay snapshot packet. It composes the existing session snapshot chunks and
then adds gameplay-owned chunks for:

- command queue state
- interaction state
- inventory state
- `NpcActorState2DRegistry`
- `NpcActorControlState2DRegistry`

This is still an in-memory `RuntimeSaveChunkArchive` boundary. It does not
connect gameplay snapshots to save file envelopes, save slots, disk paths,
format policy, or runtime frame behavior.

The codec does not move NPC actor/control state into `RuntimeSessionState` or
`RuntimeSessionSnapshot`. The nested session snapshot remains session-owned and
is decoded through the existing session snapshot chunk decoder.

## Gameplay Snapshot File Boundary

`RuntimeGameplaySnapshotSaveLoad` adds a gameplay-specific file/envelope IO
boundary on top of the gameplay snapshot chunk codec. It serializes a
`RuntimeGameplaySnapshot` through:

1. `RuntimeGameplaySnapshotChunkCodec`
2. `RuntimeSaveChunkArchiveCodec`
3. `RuntimeSaveFileEnvelope`
4. `RuntimeSaveFileIO`

This provides gameplay snapshot bytes and direct file IO only. It does not
replace or modify `RuntimeSessionSaveLoad`, and it does not add gameplay save
slots, save metadata menus, migrations, automatic runtime snapshotting, or slot
store behavior.

## Gameplay Save Slots

`RuntimeGameplaySaveSlotStore` is a separate engine-only named slot API around
`RuntimeGameplaySnapshotSaveLoad`. It uses the existing slot path policy for
slot-name safety and directory layout, but writes gameplay snapshot files through
the gameplay snapshot saver/loader.

This does not replace `RuntimeSaveSlotStore`, which remains the existing
session-slot API. Gameplay slots are not wired to UI, editor menus, runtime
autosave, command queues, migrations, or cross-format detection in this lane.

The gameplay slot store also owns engine-only slot management helpers for future
UI/editor callers:

- inspect/existence/readable facts for one safe slot path
- deterministic listing of valid gameplay slot files by configured extension
- delete of one resolved gameplay slot file
- no-overwrite rename and copy between safe gameplay slot names
- full-load summary facts derived from `RuntimeGameplaySnapshotLoader`

These helpers still operate only on gameplay snapshot files. They do not add
save-browser UI, metadata menus, old/new format detection, runtime autosave, or
session slot replacement.

## Not In This Lane

This lane does not change:

- `RuntimeSessionSnapshot`
- `RuntimeSessionSaveLoad`
- `RuntimeSaveSlotStore`
- save file envelopes
- existing session save slots
- existing session disk serialization

Gameplay save-slot UI, metadata browsing, and migration behavior remain later
lanes after this engine-only slot API is accepted.
