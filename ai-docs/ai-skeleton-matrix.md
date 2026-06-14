# AI Skeleton Matrix

This is a cross-game map of where AI data lives and how it moves through each
game. It is meant as a learning skeleton, not a design prescription.

## Fast Matrix

| Skeleton part | OpenXcom | Warzone 2100 | NetHack | DevilutionX | KeeperFX | re3 Miami |
|---|---|---|---|---|---|---|
| Static definition | `Unit` rules | component stats, templates | `permonst` table | `MonsterData` tables | creature model config | `CPedType`, `CPedStats` |
| Live actor | `BattleUnit` | `DROID` | `monst` | `Monster` | `Thing` + `CreatureControl` | `CPed` |
| Durable intent | AI mode / target memory | `DroidOrder` | monster goals/strategy bits | `MonsterGoal` | creature job/state | `eObjective` |
| Executor state | battle action states | `DROID_ACTION` | movement/combat branch | `MonsterMode` | `CreatureStates` | `PedState` |
| Movement state | pathfinding + walk action | `MOVE_CONTROL` | `mfndpos` candidates | path/position fields | movement state/jobs | `eMoveState`, path nodes |
| Map authority | `SavedBattleGame` tiles/routes | map/path services | level arrays | dungeon/solid/LOS arrays | slab/room/dungeon data | `CPathFind`, world sectors |
| Interaction sites | route nodes/objectives | structures/features/orders | doors, shops, items | doors/items/quests | rooms/jobs/workplaces | attractors |
| Global/director AI | low in battlescape | player/group systems | mostly absent | limited monster waves/quests | computer keeper | traffic/ped ambient systems |
| Save boundary | battle save | game/order/action save | level save/restore | monster save/load | chunk saves | world/ped/vehicle saves |

## Repeated Skeleton

All six games have this broad shape:

```text
definition/profile -> live actor -> objective/order/goal -> state/action/mode
                   -> movement/path query -> runtime execution -> save/load
```

The names vary, but the split repeats:

- definition data says what a thing is allowed to be
- live actor data says what this instance is now
- objective/order data says what it is trying to do
- state/action/mode data says what executor step is active
- map/path/world data answers legality and environment questions
- save/load keeps authoritative state and rebuilds temporary links/caches

## Per-Game Notes

### OpenXcom

OpenXcom has the smallest clean loop:

- `BattleUnit` owns live state.
- `AIModule` decides.
- `BattleAction` carries the proposed action.
- `BattlescapeGame` executes the action.
- `SavedBattleGame` owns the map/session services.

Useful anchors:

- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.h:36`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.h:43`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/BattleUnit.h:55`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/SavedBattleGame.h:42`

### Warzone 2100

Warzone is the clearest order/action/movement split:

- `DroidOrder` is durable intent.
- `DROID_ACTION` is active executor state.
- `MOVE_CONTROL` is movement/path-follow scratch.
- `DROID` stores the live unit and points to current order/action/move state.

Useful anchors:

- `/Users/kogaryu/iggy/warzone2100-master/src/droiddef.h:101`
- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:41`
- `/Users/kogaryu/iggy/warzone2100-master/src/actiondef.h:27`
- `/Users/kogaryu/iggy/warzone2100-master/src/movedef.h`
- `/Users/kogaryu/iggy/warzone2100-master/src/droid.cpp:927`

### NetHack

NetHack is table-and-flag driven:

- `permonst` defines monster type.
- `monst` stores live monster state.
- `mextra` stores optional role-specific extras.
- `mfndpos` stores movement candidate scratch.
- monster turn code branches through movement, combat, item use, pets, and
  special cases.

Useful anchors:

- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/permonst.h:56`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:96`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mextra.h:205`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mfndpos.h:33`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/rm.h:476`

### DevilutionX

DevilutionX is compact ARPG monster AI:

- static monster tables define capability and behavior id
- live `Monster` stores mode, goal, position, target, animation, and path data
- `AiProc` dispatches behavior by AI type
- `ProcessMonsters` runs the phase
- path/LOS helpers answer movement and attack legality

Useful anchors:

- `/Users/kogaryu/iggy/DevilutionX-master/Source/tables/monstdat.h:98`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:75`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:120`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:3091`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4257`

### KeeperFX

KeeperFX has the richest work-site skeleton:

- `Thing` is the live world object.
- `CreatureControl` is the creature control extension.
- creature model/job configs define capabilities and work behavior.
- room and dungeon structures own work sites, capacity, ownership, and economy.
- state dispatch tables run creature behavior.
- computer keeper tasks provide director AI.

Useful anchors:

- `/Users/kogaryu/iggy/keeperfx-master/src/thing_data.h:120`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:72`
- `/Users/kogaryu/iggy/keeperfx-master/src/config_creature.h:74`
- `/Users/kogaryu/iggy/keeperfx-master/src/room_data.h:50`
- `/Users/kogaryu/iggy/keeperfx-master/src/dungeon_data.h:143`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:309`

### re3 Miami

re3 Miami is strongest for open-world actor layering:

- `CPedType` and `CPedStats` hold profile/threat data.
- `CPed` owns live actor state.
- `eObjective` is durable intent.
- `PedState` is active behavior.
- `eMoveState` is movement intensity/style.
- attractors are world-owned interaction destinations.
- `CPathFind` owns path graph queries.

Useful anchors:

- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:190`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:280`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:353`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:367`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.h:28`
- `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.h:203`

## Grep Commands

See `/Users/kogaryu/iggy/ai-docs/research-grep-commands.md` for the exact
commands used to find these anchors.

