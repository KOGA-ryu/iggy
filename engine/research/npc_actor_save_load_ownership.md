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

## Not In This Lane

This lane does not change:

- `RuntimeSessionSnapshot`
- session snapshot chunk codecs
- save file envelopes
- save slots
- disk serialization

Gameplay snapshot disk persistence should be designed as a separate chunk/file
codec lane after the data packet and validator are accepted.
