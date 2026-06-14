# OpenXcom AI Study

Reference repo: `/Users/kogaryu/iggy/OpenXcom-master`

OpenXcom is a useful reference for turn-based NPC AI because its battlescape AI
is not one giant "brain." It is a small decision module attached to a live unit,
fed by map/session services, and it outputs a `BattleAction` that the battlescape
runtime executes.

## Core Ownership Map

### Static Definition Data

Static unit rules live in `/Users/kogaryu/iggy/OpenXcom-master/src/Mod/Unit.h`.

Important anchors:

- `/Users/kogaryu/iggy/OpenXcom-master/src/Mod/Unit.h:53`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Mod/Unit.h:60`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Mod/Unit.h:104`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Mod/Unit.h:106`

`Unit` is the static alien/civilian/HWP definition. It owns things like:

- type, race, rank
- base stats
- armor id
- height/floating data
- intelligence and aggression
- special ability
- built-in weapons
- aggro/move/death sounds

Learning point: the AI does not hard-code most creature personality. It reads
lightweight parameters from static unit data, especially intelligence and
aggression.

### Live Actor State

Live battle units are represented by `BattleUnit`.

Important anchors:

- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/BattleUnit.h:55`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/BattleUnit.h:64`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/BattleUnit.h:73`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/BattleUnit.h:76`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/BattleUnit.h:83`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/BattleUnit.h:85`

`BattleUnit` owns the current actor truth:

- faction and original faction
- id, position, tile pointer, last position
- facing, destination, status, walk/fall phase
- visible units and visible tiles
- time units, energy, health, morale, stun
- inventory and special weapons
- current AI module pointer
- copied/static unit identity and stats

Learning point: the actor owns current mutable state. The AI module is attached
to the actor, but the actor remains the authoritative unit.

### Battle Session State

The battle map/session is `SavedBattleGame`.

Important anchors:

- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/SavedBattleGame.h:42`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/SavedBattleGame.h:50`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/SavedBattleGame.h:53`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/SavedBattleGame.h:55`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/SavedBattleGame.h:56`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/SavedBattleGame.h:58`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/SavedBattleGame.h:59`

`SavedBattleGame` owns:

- map size
- tiles
- route nodes
- live units
- live items
- pathfinding service
- tile engine service
- turn state and side
- objective/session state

Learning point: map intelligence is session-owned. AI queries the battle session
for tiles, units, pathfinding, visibility, and route nodes.

## AI Module Shape

The AI module is in:

- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.h`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp`

Important anchors:

- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.h:36`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.h:40`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.h:43`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.h:48`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.h:52`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.h:53`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.h:63`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.h:67`

`AIModule` stores:

- pointer to `SavedBattleGame`
- pointer to owning `BattleUnit`
- current aggro target
- counts of known/visible/spotting enemies
- candidate actions for escape, ambush, attack, patrol, psi
- current AI mode
- route node memory
- reachable tile caches
- small "was hit by" memory

AI modes:

- `AI_PATROL`
- `AI_AMBUSH`
- `AI_COMBAT`
- `AI_ESCAPE`

Learning point: OpenXcom does not model behavior as hundreds of types. It uses a
small set of modes, then computes candidate actions for each mode from current
battle state.

## Turn Flow

The battlescape runtime asks a unit to think and then executes the resulting
`BattleAction`.

Important anchors:

- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.cpp:226`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.cpp:229`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.cpp:231`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.cpp:255`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.cpp:270`

Flow:

1. Runtime creates a blank `BattleAction`.
2. Runtime sets actor and action number.
3. `BattleUnit::think()` delegates to AI.
4. If AI returns `BA_RETHINK`, runtime asks the unit to think again.
5. If action is `BA_WALK`, runtime calculates a path and pushes a walk state.
6. If action is an attack/use action, runtime pushes the matching execution state.

Learning point: AI proposes; runtime executes. The decision module does not move
pixels or animate units directly.

## BattleAction as Command Object

`BattleAction` is defined in `BattlescapeGame.h`.

Important anchors:

- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.h:41`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.h:43`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.h:45`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.h:48`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.h:49`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.h:58`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.h:60`

`BattleAction` carries:

- action type
- actor
- weapon
- target tile
- optional waypoints
- TU cost
- targeting/result flags
- final facing
- final action marker
- action number

Learning point: a command object can be plain and short-lived. It is not the
same as persisted actor state.

## AI Think Cycle

`AIModule::think()` starts by rebuilding context from the current battle.

Important anchors:

- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:137`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:147`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:148`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:149`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:153`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:190`

It computes:

- known enemy count
- visible enemy count
- enemies who can see this unit
- melee/rifle/blaster capability
- reachable positions with current TUs
- reachable positions after reserving attack TUs
- candidate escape, ambush, attack, patrol, and psi actions

Learning point: most AI "state" is rebuilt every think cycle from authoritative
map/unit data. This keeps save data small and avoids stale decisions.

## Mode Selection

The AI does not always reevaluate its mode. It reevaluates when pressure changes.

Important anchors:

- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:247`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:249`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:265`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:278`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:305`

Mode pressure examples:

- patrol reevaluates on danger or occasional randomness
- ambush reevaluates if no rifle, no ambush TU plan, or visible enemies
- combat reevaluates if attack action fails
- escape reevaluates when no longer spotted or no known enemies
- low health or being over-spotted forces reevaluation

Learning point: keep behavior mode stable unless conditions invalidate it. This
avoids twitchy AI.

## Patrol Nodes

Route nodes are saved battle/map data, not actor-owned data.

Important anchors:

- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/Node.h:30`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/Node.h:35`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/Node.h:38`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/Node.h:42`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/Node.h:82`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:415`

Nodes own:

- id
- position
- segment
- links to other nodes
- rank/type/priority
- allocation/reservation flags

AI stores only current/from/to node pointers and frees/allocates destinations.

Learning point: map-authored route graph is a powerful middle layer between
"wander randomly" and full path planning.

## Pathfinding

Pathfinding is a battle-session utility, not owned by the AI module.

Important anchors:

- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/Pathfinding.h:32`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/Pathfinding.h:38`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/Pathfinding.h:76`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/Pathfinding.h:100`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/Pathfinding.cpp:80`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/Pathfinding.cpp:87`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/Pathfinding.cpp:100`

Pathfinding owns:

- reusable pathfinding nodes
- current calculated path
- total TU cost
- current unit/movement type for calculation

AI uses it for:

- reachable tile sets
- "can I move there this turn?"
- selecting fire points, escape points, patrol points

Runtime uses it for:

- executing a chosen `BA_WALK` action

Learning point: AI should ask pathfinding questions; pathfinding should not know
the behavior policy.

## Tile and Map Data

Tiles are the physical map facts.

Important anchors:

- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/Tile.h:61`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/Tile.h:71`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/Tile.h:72`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/Tile.h:73`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/Tile.h:113`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/Tile.h:133`

Tiles own:

- map object parts
- light/fire/smoke/explosive data
- tile position
- unit occupying the tile
- items on the tile
- terrain movement cost
- door/open state

Learning point: interaction facts like doors, terrain cost, units, and items live
on the map/tile layer. AI can query them, but should not own them.

## Save/Load Shape

OpenXcom persists a small amount of AI state.

Important anchors:

- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:81`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:95`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:117`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:125`

Saved AI data:

- from node id
- to node id
- current AI mode
- unit ids that hit this unit

Not saved as durable AI truth:

- candidate attack action
- candidate escape action
- candidate ambush action
- candidate patrol action
- enemy counts
- reachable tile lists
- weapon capability booleans
- path search results
- current visible-enemy calculations

Learning point: persist memory and intent, not every derived calculation.

## Reference Pattern

OpenXcom's reusable conceptual pattern:

- coarse AI mode: patrol, ambush, combat, escape
- tiny persisted AI memory: route node, target, units that hit this unit
- short-lived action proposal: `BattleAction`
- map-owned route nodes and tile/pathfinding services
- reachable-tile and visible-enemy calculations rebuilt during AI thinking
- runtime-owned action execution after AI proposes

## Legacy Costs

The reference also shows costs:

- pointer-heavy object ownership
- AI module owning raw action pointers
- all behavior in one large class
- save/load details mixed with architecture details
- many game-specific action types
- saving derived reachable lists or candidate actions

## Minimal Reference Lesson

OpenXcom's smallest AI skeleton is:

1. Actor definition data provides movement, sight, aggression, and intelligence.
2. Live actor state provides position, facing, health, and current AI mode.
3. Battle map/session owns route nodes and queryable tiles.
4. AI tick builds context from the map and unit registry.
5. AI produces one action proposal.
6. Runtime executes the proposal through battle states.
7. Save actor state plus tiny AI memory; rebuild candidates after load.

OpenXcom's main lesson is not its exact tactics. The lesson is the boundary:
actor state is authoritative, map/session services answer questions, AI proposes,
and the runtime executes.
