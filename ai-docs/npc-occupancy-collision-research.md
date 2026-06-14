# NPC Occupancy and Actor Collision Research

Purpose: map how reference games answer "is this tile/place occupied?" before Iggy adds NPC position mutation.

## Grep Commands Used

```sh
rg -n "MON_AT|m_at|remove_monster|place_monster|mfndpos" NetHack-NetHack-5.0/include NetHack-NetHack-5.0/src
rg -n "dMonster|dPlayer|occupyTile|IsTileAvailable|DirOK|WalkInDirection" DevilutionX-master/Source
rg -n "getUnit|setUnit|_unit|reserved|allocated|escapeTiles|setUnitPosition" OpenXcom-master/src
rg -n "CEntryInfoList|RemoveAndAdd|ProcessEntityCollision|bUsesCollision|Avoid" re3-miami/src
rg -n "MOVE_CONTROL|bumpTime|MOVEWAITROUTE|fpathDroidRoute|collision_avoidance" warzone2100-master/src
rg -n "Occup|occupied|reservation|position|NpcActor" engine/src/modules/npc_ai engine/src/scene/npc engine/src/scene/ai
```

## Reference Findings

### NetHack

- Authoritative level state owns both map tiles and monster lookup: `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/rm.h:473-483`.
- `MON_AT` / `m_at` query `level.monsters[x][y]`: `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/rm.h:515-516`.
- Monster list and tile occupancy are cross-checked for consistency: `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/mon.c:263-300`.
- Candidate/final movement checks occupied tiles before stepping: `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1958-1960`.
- Normal move removes from old cell and places in new cell: `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:2051-2055`.
- Shape/special cases can make remove/place unsafe, so large actors need special handling: `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1489-1505`.

Lesson: grid occupancy is a fast live index beside actor state, not a pathfinding afterthought.

### DevilutionX

- Monsters write to `dMonster`; moving monsters can reserve with a negative index: `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:479-484`.
- Placement rejects existing monsters, players, unsafe/set-piece tiles, and blocked tiles: `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:283-290`.
- Walk start claims the future tile while movement animation is in progress: `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:786-820`.
- Walk finish clears old occupancy, mutates actor tile, places final occupancy, then updates light/stand state: `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1087-1097`.
- Availability checks reject both `dPlayer` and `dMonster`: `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1791-1813`.
- Player movement uses the same signed live occupancy idea: `/Users/kogaryu/iggy/DevilutionX-master/Source/player.cpp:2014-2019`.

Lesson: a small reservation-like marker during movement prevents another actor from taking the target cell.

### OpenXcom

- Tile owns a `BattleUnit*` occupancy pointer: `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/Tile.h:68-76`.
- Units own position separately: `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/BattleUnit.cpp:491-495`.
- Tile occupancy can be rebuilt from unit positions after load/repair: `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/SavedBattleGame.cpp:977-999`.
- `setUnitPosition` validates all body tiles for unit occupancy, objects, floor/walls, and path blocking before applying: `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/SavedBattleGame.cpp:1584-1637`.
- Patrol nodes have allocation/reservation flags: `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/Node.h:38-58`, `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/Node.cpp:168-184`.
- Local escape resolution tracks chosen escape tiles to avoid duplicate targets in one pass: `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/UnitFallBState.cpp:216-305`.

Lesson: actor position can be durable truth while tile occupancy is rebuildable query state.

### re3 Miami

- Physical entities keep sector-list membership through `CEntryInfoList`: `/Users/kogaryu/iggy/re3-miami/src/entities/Physical.h:32-37`.
- Entity collision participation is a flag, not tile occupancy: `/Users/kogaryu/iggy/re3-miami/src/entities/Entity.h:45-50`.
- Add/remove/update move entities through sector lists: `/Users/kogaryu/iggy/re3-miami/src/entities/Physical.cpp:93-153`, `/Users/kogaryu/iggy/re3-miami/src/entities/Physical.cpp:158-222`.
- Collision and shift scan current sector lists, then update membership after movement: `/Users/kogaryu/iggy/re3-miami/src/entities/Physical.cpp:2031-2105`.
- Peds track collision/flee contacts separately from objective state: `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:580-590`, `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:660-690`.

Lesson: non-grid movement wants spatial buckets and collision resolution, not tile reservation.

### Warzone 2100

- Droid movement state owns path vector, destination, bump history, and formation pointer: `/Users/kogaryu/iggy/warzone2100-master/src/movedef.h:45-65`.
- Route planning writes movement status and may wait for async path calculation: `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:235-330`.
- Bump history can trigger reroute to the same destination: `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:805-860`.
- Steering blends destination and obstacle-avoid vectors: `/Users/kogaryu/iggy/warzone2100-master/src/steering/collision_avoidance_behavior.cpp:120-160`.

Lesson: RTS actor collision is often continuous steering plus reroute, not a strict tile-claim system.

## Iggy Current Anchors

- `NpcActorState2D` owns NPC id/profile/faction/position/present only: `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorState2D.hpp:11-24`.
- `LevelTileMap` owns static tiles, spawns, player start; no live occupancy: `/Users/kogaryu/iggy/engine/src/scene/level/LevelTileMap.hpp:12-33`.
- `LevelGridQuery` only answers static containment/walkability/index: `/Users/kogaryu/iggy/engine/src/scene/level/LevelGridQuery.hpp:16-35`.
- `NavigationGridPathfinder` checks map bounds/walkability only: `/Users/kogaryu/iggy/engine/src/servers/navigation/NavigationGridPathfinder.cpp:49-107`.
- `CollisionWorld2D` is a vector of collision objects; actors are not automatically part of it: `/Users/kogaryu/iggy/engine/src/servers/physics2d/CollisionWorld2D.hpp:11-26`.
- Player movement executor mutates a copied player state from `CharacterMove2D` against a collision world: `/Users/kogaryu/iggy/engine/src/scene/player/PlayerMovementExecutor2D.cpp:38-99`.
- Current escape target selection samples static walkability only: `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorEscapeTarget2D.hpp:53-59`.

## Builder-Ready Recommendation

### Safe Now: Derived NPC Occupancy

Build `NpcActorOccupancy2D` in `engine/src/scene/npc/`.

Ownership:
- input truth: `NpcActorState2DRegistry`
- derived query: tile-to-NPC occupancy
- not owned by `LevelTileMap`
- not owned by runtime/session
- not serialized
- rebuilt from actor positions whenever needed

Minimum data:
- `NpcActorOccupancyEntry2D { ResourceId npcId; TileCoord tile; std::size_t actorIndex; }`
- `NpcActorOccupiedTile2D { TileCoord tile; std::vector<ResourceId> npcIds; }`
- `NpcActorOccupancy2D { std::vector<NpcActorOccupancyEntry2D> entries; std::vector<NpcActorOccupancyIssue2D> issues; }`

Minimum query:
- `occupantsAt(TileCoord)`
- `firstOccupantAt(TileCoord)`
- `isOccupied(TileCoord)`
- `isBlockedFor(ResourceId movingNpcId, TileCoord)` where self does not block self, other present NPCs do

Minimum builder inputs:
- `NpcActorState2DRegistry`
- optional config: include absent actors false by default

Tests should prove:
- absent actors do not occupy tiles by default
- actor position maps to expected tile
- two actors on same tile are reported as duplicate occupancy but still queryable
- `isBlockedFor(self, ownTile)` is false
- `isBlockedFor(actorA, actorBTile)` is true
- no map mutation and no actor mutation happen

### Safe Next: Occupancy-Aware Read-Only Filters

- Escape candidate filter can optionally reject occupied tiles.
- Route target/path request can keep producing map-only targets.
- Pathfinding should stay map-only until an explicit overlay/blocker policy is reviewed.

### Wait For Ownership Review

- `NpcActorMovementReservation2D`: per-frame destination claims before execution.
- Occupancy-aware `NavigationGridPathfinder` overlay.
- Actor collision bodies inside `CollisionWorld2D`.
- Multi-tile footprints.
- swaps, push/displacement, shared-tile rules, allies passing through each other.
- runtime/session/save integration.

### Executor Policy Later

Before `NpcActorMovementExecutor2D` mutates position, it should consume:
- current actor registry
- static map/collision facts
- derived `NpcActorOccupancy2D`
- route/path/step proposal

Post-move result should report:
- old tile
- requested tile/position
- accepted tile/position
- blocked reason
- blocking NPC id when known
- dirty old/new tiles for later visibility/render/AI cache work

Do not make the executor mutate a map-owned occupancy grid first. Mutate the actor registry/result copy, then rebuild occupancy or emit dirty facts.

## Compute Costs

- Rebuilding occupancy from actors: `O(actor_count)`, cheap for tests and early runtime.
- Tile query with a flat vector: `O(actor_count)`, acceptable only for tiny tests.
- Tile query with indexed cells/hash map: `O(1)` average after `O(actor_count)` build.
- Pathfinding with occupancy overlay: BFS/A* neighbor checks add one occupancy query per neighbor.
- Reservations: `O(move_count)` inserts into destination map; conflict detection is cheap.
- Pairwise actor collision: `O(actor_count^2)`, avoid for grid actors.
- Post-move dirty marking: `O(1)` per moved actor for old/new tile facts.

## Short Answer

For Iggy, build `NpcActorOccupancy2D` and `NpcActorOccupancyQuery2D` as a derived `scene/npc` projection from `NpcActorState2DRegistry` before any NPC actor executor mutates positions.

Do not put live actor occupancy into `LevelTileMap`, `RuntimeSessionState`, or `CollisionWorld2D` yet. Add `NpcActorMovementReservation2D` only after route/path/step proposals can produce competing destinations.
