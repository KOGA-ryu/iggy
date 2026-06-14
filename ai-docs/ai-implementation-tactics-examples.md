# AI Implementation Tactics Examples

This is the concrete programming-method view: the low-level tools the reference
games use to build AI systems.

## Pointers and References

All six repos use pointers or references between actors, targets, maps, rooms,
vehicles, orders, and resources.

Examples:

- OpenXcom `AIModule` stores references to battle/session and owner unit.
  - `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.h:40`
- Warzone `DROID` carries links to group/base/order/action/move state.
  - `/Users/kogaryu/iggy/warzone2100-master/src/droiddef.h:101`
- NetHack `monst` pointers appear throughout monster behavior and level lists.
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:96`
- re3 `CPed` connects to targets, objectives, paths, and vehicles.
  - `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:367`

## Raw IDs and Indexes

The older engines often combine pointers with ids, indexes, map coordinates,
or array handles.

Examples:

- NetHack level occupancy is a fixed coordinate grid.
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/rm.h:476`
- KeeperFX uses indexed things, rooms, dungeon/player data, and slab maps.
  - `/Users/kogaryu/iggy/keeperfx-master/src/thing_data.h:120`
  - `/Users/kogaryu/iggy/keeperfx-master/src/dungeon_data.h:143`
- Warzone uses per-player and per-object data structures with object links.
  - `/Users/kogaryu/iggy/warzone2100-master/src/droiddef.h:101`

## Arrays

Arrays appear in map grids, actor pools, path nodes, flags, save chunks, and
candidate buffers.

Examples:

- NetHack has a monster occupancy grid and level monster list.
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/rm.h:476`
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/rm.h:479`
- DevilutionX uses dungeon/map arrays and static monster tables.
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/tables/monstdat.h:98`
- KeeperFX uses arrays/tables for state dispatch and room/dungeon data.
  - `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:309`

## Vectors and Growable Lists

Newer C++ code uses growable containers for managers, active lists, and order
builders.

Examples:

- Warzone builds UI/order lists with `std::vector`.
  - `/Users/kogaryu/iggy/warzone2100-master/src/intorder.cpp:427`
- re3 attractor manager owns collections of attractors.
  - `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.h:28`
- OpenXcom uses collection-style ownership around units, items, and tiles in
  battle/session structures.
  - `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/SavedBattleGame.h:42`

## Linked Lists and Intrusive Lists

Older engines use linked lists or intrusive object lists for live objects and
map buckets.

Examples:

- NetHack has level monster lists and monster save chains.
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/rm.h:479`
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/save.c:894`
- KeeperFX uses indexed object pools and lists across rooms/things/dungeon data.
  - `/Users/kogaryu/iggy/keeperfx-master/src/thing_data.h:120`
  - `/Users/kogaryu/iggy/keeperfx-master/src/room_data.h:50`

## Bit Flags and Bitfields

Flags are one of the most common AI techniques.

Examples:

- NetHack monster capabilities are split across `M1_*`, `M2_*`, `M3_*` flags.
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/permonst.h:56`
- Warzone has secondary order/policy flags.
  - `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:88`
- KeeperFX uses flags for states, jobs, rooms, creatures, and player AI.
  - `/Users/kogaryu/iggy/keeperfx-master/src/config_creature.h:236`
- re3 uses enum and flag-style fields for ped states, objectives, and movement.
  - `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:190`

## Enums

Every repo uses enums for AI vocabulary.

Examples:

- OpenXcom: `AI_PATROL`, `AI_AMBUSH`, `AI_COMBAT`, `AI_ESCAPE`.
  - `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.h:36`
- Warzone: `DROID_ORDER`, `DROID_ACTION`, secondary policy enums.
  - `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:44`
  - `/Users/kogaryu/iggy/warzone2100-master/src/actiondef.h:27`
- DevilutionX: `MonsterGoal`, `MonsterMode`.
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:75`
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:120`
- re3: `eObjective`, `PedState`, `eMoveState`.
  - `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:190`
  - `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:280`
  - `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:353`

## Switches and Dispatch Tables

Large switch dispatch and function tables are common.

Examples:

- OpenXcom branches by AI mode inside the AI module.
  - `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:174`
- Warzone maps order/action enums through update and name dispatch.
  - `/Users/kogaryu/iggy/warzone2100-master/src/order.cpp:4235`
- DevilutionX dispatches monster behavior through `AiProc`.
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:3091`
- KeeperFX dispatches creature state behavior through `process_func_list`.
  - `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:309`
- re3 has objective and state branch logic in ped AI.
  - `/Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp:777`

## Command, Action, and Proposal Structs

Many systems separate decision data from execution.

Examples:

- OpenXcom `BattleAction` is filled by AI and executed by the battlescape.
  - `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.h:43`
- Warzone `DroidOrder` stores high-level intent before action/move execution.
  - `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:41`
- KeeperFX creature jobs and states request work from room/world systems.
  - `/Users/kogaryu/iggy/keeperfx-master/src/creature_jobs.c:691`
- re3 objectives request behavior that later becomes states, paths, and
  attractor assignments.
  - `/Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp:131`

## Save Buffers and Chunks

All repos preserve authoritative state and rebuild temporary runtime links,
indexes, or scratch data after load.

Examples:

- OpenXcom battle/session state lives under savegame structures.
  - `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/SavedBattleGame.h:42`
- Warzone save/load stores orders/actions and game state.
  - `/Users/kogaryu/iggy/warzone2100-master/src/game.cpp:5486`
  - `/Users/kogaryu/iggy/warzone2100-master/src/game.cpp:5792`
- NetHack saves monster chains and restores level state.
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/save.c:836`
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/save.c:894`
- DevilutionX has explicit monster save/load functions.
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:663`
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:1502`
- KeeperFX save/load is chunk-oriented.
  - `/Users/kogaryu/iggy/keeperfx-master/src/game_saves.c:87`
  - `/Users/kogaryu/iggy/keeperfx-master/src/game_saves.c:181`
- re3 has game, ped, path, and vehicle save entry points.
  - `/Users/kogaryu/iggy/re3-miami/src/save/MemoryCard.cpp:335`
  - `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:9544`
  - `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.cpp:1784`
  - `/Users/kogaryu/iggy/re3-miami/src/vehicles/Vehicle.cpp:2364`

## Commands Used

See `/Users/kogaryu/iggy/ai-docs/research-grep-commands.md`. The tactic tally
came from searching concrete constructs: `struct`, `class`, `enum`, `Save`,
`Load`, `Order`, `Action`, `State`, `Objective`, `Path`, and update verbs.

