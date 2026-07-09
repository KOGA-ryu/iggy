# World State

File:

- `/Users/kogaryu/iggy3d/src/runtime/world/EntityState.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/world/WorldState.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/world/WorldState.cpp`

Verified at: `00e97746`

## Owns

- Runtime entity packet shape: `EntityState`, `EntityKind`, `EntityTargeting`, `TargetAction`.
- Runtime world entity container and cursor: `WorldState`.
- Entity add, seed, upsert, lookup, transform mutation, active-flag mutation, clear, and baseline reset.
- Validation for stable names, duplicate ids/names, entity kind, transforms, and local bounds.

## Does Not Own

- Save/load persistence of world state.
- Combat, AI, inventory, ability, or command policy attached to entities.
- Content package entity seeding rules.
- App/editor object authoring.

## Reads

- `EntityState` id, stable name, kind, transform, local bounds, active flag, targeting, and interaction data.
- Core entity id helpers and transform/AABB validity helpers.

## Writes / Mutates

- Mutates the private `entities_` vector and `nextEntityId_` cursor.
- `updateTransform` mutates only entity transforms.
- `setActive` mutates only the active flag.
- `resetFromBaseline` copies entity rows and cursor from another `WorldState`.

## Calls Out To / Wires Out To

- Provides world entity rows to entity-hit queries, NPC perception/behavior, session command execution, save/load, and app product seeding.
- Keeps no app/window/render dependencies.

## Called By / Entry Points

- `WorldState::entities`, `nextEntityId`, `empty`, `size`.
- `WorldState::addEntity`, `seedEntity`, `upsertEntity`.
- `WorldState::findById`, `findByStableName`.
- `WorldState::updateTransform`, `setActive`, `clear`, `resetFromBaseline`.
- Inline helpers `isTargetActionSupported` and `isValidEntityKind`.

## Invariants

- Stable names must be non-empty and unique.
- Valid inserted entities must have valid kind, finite positive-scale transform, and valid local bounds.
- `addEntity` assigns the next id when the incoming id is invalid.
- `seedEntity` requires a valid explicit id.
- Explicit duplicate ids are rejected.
- The next-id cursor advances past inserted explicit ids and resets to `1` on clear.

## Tests / Proof Commands

- `rg -n "world_state_tests|WorldState|EntityState|addEntity|seedEntity|upsertEntity" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `cmake/iggy3d_tests.cmake` registers `world_state_tests`.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/save/SaveCodec.*`
- `/Users/kogaryu/iggy3d/src/runtime/session/Session.*`
- `/Users/kogaryu/iggy3d/src/runtime/collision/EntityHitQuery.*`

## Update When

- Entity validation, target action flags, mutation result semantics, or id cursor policy changes.

## Do Not Update When

- Only package seeding, save codec field order, app editing, or AI behavior policy changes.
