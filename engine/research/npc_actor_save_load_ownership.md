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

## Not In This Lane

This lane does not change:

- `RuntimeSessionSnapshot`
- save file envelopes
- save slots
- disk serialization

Gameplay snapshot disk persistence remains a later file/save-slot lane after
the archive packet boundary is accepted.
