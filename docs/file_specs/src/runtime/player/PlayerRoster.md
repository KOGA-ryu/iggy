# Player Roster

File:

- `/Users/kogaryu/iggy3d/src/runtime/player/PlayerSlot.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/player/PlayerRoster.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/player/PlayerRoster.cpp`

Verified at: `69514d40`

## Owns

- Runtime player slot packet shape and slot kind helpers.
- Player roster storage and validation for player-slot to actor bindings.
- Slot lookup, actor lookup by slot, slot-controls-actor checks, and roster clear.

## Does Not Own

- World entity storage or actor lifecycle.
- Command admission decisions beyond providing roster queries.
- Session seeding policy for slots.
- App player profile/UI ownership.

## Reads

- `PlayerSlot` id, kind, actor, and stable name.
- Existing roster slots for duplicate slot ids and actor double-binding.
- Core entity id validity.

## Writes / Mutates

- Mutates the private roster slot vector through `addSlot` and `clear`.
- Does not mutate world state or actor rows.

## Calls Out To / Wires Out To

- `Session` seeds `PlayerRoster` during session creation.
- `CommandAdmission` reads roster slots and actor bindings for command validation.
- Save/load reconstructs roster slots from persisted state.

## Called By / Entry Points

- `isValidPlayerSlotId(...)`
- `isPlayableSlotKind(...)`
- `isActorControllingSlotKind(...)`
- `PlayerRoster::addSlot`, `findSlot`, `actorForSlot`, `slotControlsActor`, `clear`, `slots`, `empty`, `size`.

## Invariants

- Invalid slot ids are rejected.
- Unknown slot kind is rejected.
- Actor-controlling slot kinds require a valid actor.
- Duplicate slot ids are rejected.
- One valid actor cannot be bound to multiple actor-controlling slots.
- Observer/non-controlling slots do not return an actor through `actorForSlot`.

## Tests / Proof Commands

- `rg -n "PlayerRoster|PlayerRosterStatus|slotControlsActor|actorForSlot" src/runtime tests/unit cmake/iggy3d_tests.cmake`
- Covered indirectly by `command_admission_tests`, `session_tick_tests`, `save_load_tests`, and session seeding paths.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/command/CommandAdmission.*`
- `/Users/kogaryu/iggy3d/src/runtime/session/Session.*`
- `/Users/kogaryu/iggy3d/src/runtime/save/SaveLoad.*`

## Update When

- Slot kind semantics, roster validation, actor binding policy, or public roster queries change.

## Do Not Update When

- Only command rejection order, session seed source data, or app player UI changes.
