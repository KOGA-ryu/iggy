# Interaction and Worksite AI

This study focuses on AI that is not just "chase target." It covers jobs,
rooms, attractors, orders, shops, doors, items, and other world-owned
interaction sites.

## KeeperFX: Rooms and Jobs

KeeperFX has the strongest worksite model.

Observed shape:

- rooms own work destinations, capacity, ownership, and room type state
- creature jobs connect creature capabilities to work behavior
- creatures enter states that try to find, reserve, reach, use, or leave jobs
- dungeon/player state owns broader economy and faction context
- computer keeper tasks select construction, attack, defense, and utility work

Useful anchors:

- `/Users/kogaryu/iggy/keeperfx-master/src/room_data.h:50`
- `/Users/kogaryu/iggy/keeperfx-master/src/room_data.h:150`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_jobs.c:691`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_jobs.c:750`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:309`
- `/Users/kogaryu/iggy/keeperfx-master/src/player_computer.h:57`

## re3 Miami: Attractors

re3 uses attractors for world-owned interaction destinations.

Observed shape:

- attractor manager owns collections of attractors
- attractors describe a world interaction site
- peds are assigned to attractors and can queue or occupy a point
- objectives and states drive the ped toward the attractor behavior
- pathfinding remains separate from the attractor itself

Useful anchors:

- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.h:28`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.h:78`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:9361`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp:777`
- `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.h:203`

## Warzone 2100: Orders as Interaction Requests

Warzone treats many interactions as orders or action states.

Observed shape:

- orders carry target object, target stats, target position, and return data
- action states represent current build/repair/rearm/attack/recover work
- group and secondary policy can change how interaction orders behave
- structures and features are world objects that orders refer to

Useful anchors:

- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:41`
- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:72`
- `/Users/kogaryu/iggy/warzone2100-master/src/actiondef.h:27`
- `/Users/kogaryu/iggy/warzone2100-master/src/action.h:65`
- `/Users/kogaryu/iggy/warzone2100-master/src/droid.cpp:927`

## NetHack: World Interaction Through Special Branches

NetHack has many interaction behaviors, but they are less centralized.

Observed shape:

- doors, shops, items, pets, attacks, spells, and monster specials branch from
  the general monster turn flow
- monster flags and live state determine which special branches can happen
- level arrays and object lists own the world truth
- optional extension structs store role-specific state

Useful anchors:

- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/permonst.h:56`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:96`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mextra.h:205`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/muse.c:441`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/dog.c:691`

## OpenXcom: Route Nodes and Battle Objectives

OpenXcom's interactions are tactical rather than workplace-based.

Observed shape:

- battle session owns route nodes and tiles
- AI module can remember a route node and propose movement or attack actions
- action execution belongs to the battlescape runtime
- session state owns objectives and turn side

Useful anchors:

- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/SavedBattleGame.h:42`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.h:40`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:137`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.h:43`

## DevilutionX: ARPG Interaction as Combat/Quest/Item Logic

DevilutionX is less of a worksite reference and more of a compact ARPG runtime.

Observed shape:

- monsters mostly run combat, chase, retreat, special AI, and path/LOS logic
- doors, items, quests, and dungeon state exist around the monster loop
- monster interaction is tightly tied to dungeon occupancy, animation, and
  combat modes

Useful anchors:

- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4257`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:3091`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1596`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1861`

## Commands Used

Main commands are in `/Users/kogaryu/iggy/ai-docs/research-grep-commands.md`.
The important trick for interaction systems was searching world nouns together
with behavior verbs:

```bash
rg -n "Room|Job|Attractor|Order|Action|shop|door|item|use|repair|build|reserve|queue" /path/to/repo/src
```

