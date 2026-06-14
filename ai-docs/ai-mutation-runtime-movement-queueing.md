# AI Mutation, Runtime, Movement, and Queueing

This document explains how the reference games separate "deciding something"
from "mutating the world." It focuses on four learning topics:

- new mutation: where authoritative state actually changes
- runtime: what exists only while the game/session is running
- actor movement: how intent becomes position changes
- queueing: how requests are delayed, ordered, or drained

## Core Boundary

Use this mental split:

```text
proposal/request
  says what should happen

validation
  checks whether it can happen

runtime execution
  mutates actor/map/session truth

derived update
  refreshes caches, visibility, render state, events, reports
```

AI code often creates proposals. Runtime code performs mutation.

## What Counts As Mutation

Mutation is any write to authoritative game/session state:

- actor position changes
- actor state/mode/objective changes
- HP/status/inventory changes
- map tile state changes
- door/rubble/fire/smoke/collision changes
- ownership/capture changes
- queue/order list changes
- save/session record changes
- cache invalidation flags

Not all mutation is equal. A local candidate list is temporary mutation. A
position write or map tile write is authoritative mutation.

## What Counts As Runtime

Runtime state exists to run the current session. It may be saved, rebuilt, or
discarded depending on the game.

Common runtime state:

- active actor lists
- current order/action/mode
- movement/path-follow state
- spatial grids
- visibility and LoS caches
- occupancy arrays
- queued commands/orders
- pending effects
- animation state
- temporary candidate lists
- post-load repaired pointers

The key reading question:

```text
Is this value source truth, execution state, or a rebuildable cache?
```

## Actor Movement Shape

Most references follow this shape:

```text
intent/order/objective
  -> movement candidate/path query
  -> movement state/path-follow state
  -> runtime position mutation
  -> occupancy/visibility/render update
```

Movement is not just changing `x,y`. It often touches:

- old tile occupancy
- new tile occupancy
- direction/facing
- movement points or time units
- collision response
- path queue/buffer
- post-move effects
- visibility/light recalculation
- animation/mode state

## Queueing Shape

Queues and lists appear when a game needs delayed or ordered work:

```text
input or AI request
  -> queue/list/order buffer
  -> runtime drains next request
  -> validates target still exists
  -> mutates current order/action/state
```

Queueing is used for:

- player command order lists
- pending build/move orders
- path direction buffers
- active state stacks
- dead/removal lists
- event/audio queues
- save/load migration lists
- actor update lists safe for mutation

## OpenXcom

OpenXcom has a clear proposal-to-execution boundary.

Observed shape:

- `AIModule::think` fills a `BattleAction`.
- `BattlescapeGame` turns `BattleAction` into walk, attack, use, or state
  execution.
- `Pathfinding` owns a path direction buffer and can dequeue directions.
- `SavedBattleGame` owns battle units, tiles, items, nodes, deleted items, and
  other session state.
- `Tile::openDoor` is an example of map/tile mutation with unit cost checks.

Useful anchors:

- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:137`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.cpp:226`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.cpp:255`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.cpp:270`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/Pathfinding.cpp:202`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/Pathfinding.cpp:610`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/Tile.cpp:338`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/SavedBattleGame.cpp:1092`

Learning point:

```text
AI proposal is not the mutation. Runtime battle states mutate the battle.
Pathfinding keeps execution scratch. The saved battle owns session truth.
```

## Warzone 2100

Warzone has the strongest queue/order example.

Observed shape:

- `DroidOrder` stores durable order payload.
- `DROID` owns current order, action, move state, and an order list.
- `orderUpdateDroid`, `actionUpdateDroid`, and `moveUpdateDroid` split runtime
  update phases.
- queued orders are stored in `asOrderList` with `listSize` and
  `listPendingBegin`.
- queue functions add, send, pop, erase, and clear orders.
- movement execution updates direction, speed, and position in `move.cpp`.

Useful anchors:

- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:171`
- `/Users/kogaryu/iggy/warzone2100-master/src/droid.cpp:927`
- `/Users/kogaryu/iggy/warzone2100-master/src/droid.cpp:939`
- `/Users/kogaryu/iggy/warzone2100-master/src/droid.cpp:945`
- `/Users/kogaryu/iggy/warzone2100-master/src/droid.cpp:950`
- `/Users/kogaryu/iggy/warzone2100-master/src/order.h:87`
- `/Users/kogaryu/iggy/warzone2100-master/src/order.h:94`
- `/Users/kogaryu/iggy/warzone2100-master/src/order.h:100`
- `/Users/kogaryu/iggy/warzone2100-master/src/order.cpp:2311`
- `/Users/kogaryu/iggy/warzone2100-master/src/order.cpp:2340`
- `/Users/kogaryu/iggy/warzone2100-master/src/order.cpp:2390`
- `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:1611`
- `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:2165`

Learning point:

```text
Order list is queued intent. Current order is active intent. Action state is
executor state. Move state performs physical runtime mutation.
```

## NetHack

NetHack shows turn-based mutation and delayed cleanup.

Observed shape:

- `movemon` iterates monsters.
- `dochug` handles the monster turn phases.
- `m_move` performs standard movement decision/execution.
- `mfndpos` builds candidate movement positions.
- monster movement can update map occupancy, post-move effects, light/vision,
  object pickup, door state, and monster track memory.
- dead monsters remain on the monster list until end-of-turn cleanup.

Useful anchors:

- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:102`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:111`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:170`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:213`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mfndpos.h:33`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/mon.c:1329`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:690`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1717`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1927`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:2051`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/mon.c:1343`

Learning point:

```text
Candidate movement and actual movement are entangled in older code, but the
boundary still exists: candidates are scratch; map occupancy and monster fields
are authoritative mutation.
```

## DevilutionX

DevilutionX ties runtime monster mutation to mode, path, animation, and dungeon
occupancy.

Observed shape:

- `ProcessMonsters` is the runtime phase.
- active monster ids are iterated.
- monster behavior can plan path or dispatch through `AiProc`.
- `MonsterGoal` and `MonsterMode` split intent from current execution.
- dungeon arrays store occupancy/query truth.
- save/load persists live monster records, then runtime state is resynced.

Useful anchors:

- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:75`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:120`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1861`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:3091`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4257`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4322`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/levels/gendung.h:149`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:663`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:1502`

Learning point:

```text
Monster runtime mutation is not isolated to AI. It crosses mode, animation,
pathing, occupancy, lighting, target memory, and save/load repair.
```

## KeeperFX

KeeperFX shows mutation through state transitions, jobs, rooms, and director
tasks.

Observed shape:

- `Thing` is common live object state.
- `CreatureControl` is runtime creature brain/control state.
- creature state dispatch runs through `process_func_list`.
- transition helpers set state and cleanup old side effects.
- job assignment validates creature/profile/room/world before entering state.
- rooms own work-site capacity and membership.
- computer keeper tasks mutate higher-level dungeon plans.

Useful anchors:

- `/Users/kogaryu/iggy/keeperfx-master/src/thing_data.h:120`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:72`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:309`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:4839`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:4873`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:5045`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_jobs.c:691`
- `/Users/kogaryu/iggy/keeperfx-master/src/room_data.h:50`
- `/Users/kogaryu/iggy/keeperfx-master/src/player_computer.h:57`

Learning point:

```text
State mutation is a lifecycle. Entering a new state must clean old room,
combat, spell, drag, summon, and job side effects.
```

## re3 Miami

re3 shows mutation through objectives, ped states, move states, path-follow
state, and attractor assignment.

Observed shape:

- `SetObjective` mutates durable objective and previous-objective memory.
- `ProcessObjective` maps objective into active behavior.
- `SetFollowPath` mutates path-follow state on the ped.
- `SetNewAttraction` assigns the ped to world-owned attractor/queue state.
- `CPed::Save` persists live ped state.
- vehicle movement has a separate autopilot runtime object.

Useful anchors:

- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:190`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:280`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:353`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp:131`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp:777`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:6208`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:9361`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:9544`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.h:28`
- `/Users/kogaryu/iggy/re3-miami/src/control/AutoPilot.h:62`

Learning point:

```text
Objective mutation, state mutation, movement mutation, and interaction-site
mutation are related but not identical. re3 stores many of them on one actor,
but the conceptual layers are separate.
```

## Search Commands Used

OpenXcom:

```bash
rg -n "BattleAction|think\\(|Pathfinding|dequeuePath|openDoor|pushState|popState|_deleted|_fallingUnits" \
  /Users/kogaryu/iggy/OpenXcom-master/src/Battlescape \
  /Users/kogaryu/iggy/OpenXcom-master/src/Savegame
```

Warzone 2100:

```bash
rg -n "DroidOrder|ModeQueue|orderUpdate|actionUpdate|moveUpdate|asOrderList|listSize|listPendingBegin|orderDroidList|moveUpdateDroidPos" \
  /Users/kogaryu/iggy/warzone2100-master/src/order* \
  /Users/kogaryu/iggy/warzone2100-master/src/droid.cpp \
  /Users/kogaryu/iggy/warzone2100-master/src/action* \
  /Users/kogaryu/iggy/warzone2100-master/src/move.cpp
```

NetHack:

```bash
rg -n "movement|mtrack|mstrategy|fmon list|movemon\\(|dochug|m_move\\(|mfndpos|remove_monster|place_monster|postmov|dmonsfree" \
  /Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h \
  /Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mfndpos.h \
  /Users/kogaryu/iggy/NetHack-NetHack-5.0/src/mon.c \
  /Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c
```

DevilutionX:

```bash
rg -n "MonsterGoal|MonsterMode|ProcessMonsters|AiProc|AiPlanPath|dMonster|LoadMonster|SaveMonster" \
  /Users/kogaryu/iggy/DevilutionX-master/Source/monster.h \
  /Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp \
  /Users/kogaryu/iggy/DevilutionX-master/Source/levels/gendung.h \
  /Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp
```

KeeperFX:

```bash
rg -n "struct Thing|struct CreatureControl|process_func_list|internal_set_thing_state|cleanup_current_thing_state|external_set_thing_state|send_creature_to_job|struct Room|struct ComputerTask" \
  /Users/kogaryu/iggy/keeperfx-master/src/thing_data.h \
  /Users/kogaryu/iggy/keeperfx-master/src/creature_control.h \
  /Users/kogaryu/iggy/keeperfx-master/src/creature_states.c \
  /Users/kogaryu/iggy/keeperfx-master/src/creature_jobs.c \
  /Users/kogaryu/iggy/keeperfx-master/src/room_data.h \
  /Users/kogaryu/iggy/keeperfx-master/src/player_computer.h
```

re3 Miami:

```bash
rg -n "SetObjective|ProcessObjective|SetFollowPath|SetNewAttraction|CPed::Save|enum eObjective|enum PedState|enum eMoveState|CPedAttractor|CAutoPilot" \
  /Users/kogaryu/iggy/re3-miami/src/peds \
  /Users/kogaryu/iggy/re3-miami/src/control \
  /Users/kogaryu/iggy/re3-miami/src/vehicles
```

## Code Review Questions

When reading code for these topics, ask:

```text
What request/proposal enters the system?
Where is it validated?
Where does authoritative mutation happen?
What runtime state changes?
What movement/query/cache state changes?
Is any mutation delayed through a queue/list?
What drains that queue/list?
Can entries become stale before execution?
What cleanup happens if the actor dies, target disappears, or map changes?
What is saved, and what is rebuilt?
```

## Common Failure Modes

- AI directly mutates position while still deciding.
- movement updates position but forgets occupancy or visibility.
- queues keep dead target pointers.
- queue drain happens in multiple places with different rules.
- state transition skips cleanup of old side effects.
- runtime caches are mutated as if they were source truth.
- save/load persists pending scratch without repair rules.
- actor movement and map mutation happen in the wrong order.
- command queues grow without bounds or clear ownership.

## Minimal Learning Summary

The cleanest architecture lesson across the references:

```text
AI decides.
Runtime validates.
Runtime mutates.
Movement updates actor + map occupancy.
Queues delay intent or effects.
Caches are rebuilt or invalidated after mutation.
Save/load persists truth, then repairs runtime links.
```

