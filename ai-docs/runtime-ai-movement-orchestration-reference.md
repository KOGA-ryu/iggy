# Runtime AI and Movement Orchestration Reference

Educational notes on how reference games connect AI intent, active behavior,
path/movement execution, occupancy/collision, map semantics, display projection,
and runtime compute. This is a source-reading map only.

## Grep Trail

Commands used during this pass:

```sh
ls -d /Users/kogaryu/iggy/NetHack-NetHack-5.0 /Users/kogaryu/iggy/DevilutionX-master /Users/kogaryu/iggy/openxcom-master /Users/kogaryu/iggy/warzone2100-master /Users/kogaryu/iggy/re3-miami
rg --files /Users/kogaryu/iggy/ai-docs | sed -n '1,220p'
nl -ba /Users/kogaryu/iggy/ai-docs/README.md | sed -n '1,220p'
rg -n "mfndpos|monmove|m_move|dochug|m_at|place_monster|remove_monster|newsym|vision|display|glyph|cansee|mstrategy|STRAT|dist2|flee|scared|mtmp" /Users/kogaryu/iggy/NetHack-NetHack-5.0/src /Users/kogaryu/iggy/NetHack-NetHack-5.0/include | sed -n '1,260p'
rg -n "enum class MonsterGoal|enum class MonsterMode|ProcessMonsters|AiProc|AiPlanPath|AiRetreat|AiAvoid|DirOK|dMonster|MFLAG|Monster::|StartWalk|MonsterTryWalk|UpdateMonsters|LineClear|GetDirection" /Users/kogaryu/iggy/DevilutionX-master/Source | sed -n '1,300p'
rg -n "BattleAIState|AIModule|think\\(|AIAction|AI_ESCAPE|escape|Pathfinding|calculate|setUnitPosition|Tile::_unit|setUnit|occup|patrol|Node|visible|FOV" /Users/kogaryu/iggy/openxcom-master/src | sed -n '1,300p'
rg -n "order|DORDER|DroidOrder|action|DACTION|move|path|astar|Path|route|formation|cluster|vis|visibility|grid|block|occup|reservation|psMove|updateDroid|aiUpdate|processDroid|actionUpdate|moveUpdate" /Users/kogaryu/iggy/warzone2100-master/src | sed -n '1,340p'
rg -n "enum eObjective|OBJECTIVE_|SetObjective|ProcessObjective|PedState|m_nPedState|MoveState|m_nMoveState|SetMoveState|SetFlee|FLEE|CPathFind|AutoPilot|ProcessControl|ProcessEntityCollision|AddToMovingList|RemoveFromMovingList|sector|attractor|CPedAttractor|threat|avoid" /Users/kogaryu/iggy/re3-miami/src | sed -n '1,340p'
rg -n "void AIModule::think|AIModule::|BattleAIState|AIAction|Pathfinding::calculate|Pathfinding::dequeue|UnitWalkBState|setUnitPosition|Tile::setUnit|Tile::_unit|calculateFOV|visible|getPatrolNode|patrol|escape" /Users/kogaryu/iggy/openxcom-master/src/Battlescape /Users/kogaryu/iggy/openxcom-master/src/Savegame | sed -n '1,360p'
rg --files /Users/kogaryu/iggy/warzone2100-master/src | rg '(orderdef|droiddef|actiondef|action\\.cpp|move\\.cpp|mapgrid|ai\\.cpp|path|astar)'
rg -n "PATH|sMove|asPath|numPath|pathIndex|moveUpdate|moveCalc|fpath|astar|DROID_MOVE|MOVESTATE|MoveStatus|psMove|struct MOVE|path" /Users/kogaryu/iggy/warzone2100-master/src/droiddef.h /Users/kogaryu/iggy/warzone2100-master/src/move.cpp /Users/kogaryu/iggy/warzone2100-master/src/fpath.cpp /Users/kogaryu/iggy/warzone2100-master/src/astar.cpp /Users/kogaryu/iggy/warzone2100-master/src/mapgrid.cpp | sed -n '1,340p'
rg -n "enum eObjective|OBJECTIVE_|m_objective|m_nPedState|m_nMoveState|ProcessObjective|SetObjective|SetFlee|SetMoveState|Seek|FLEE|AddToMovingList|RemoveFromMovingList|sector" /Users/kogaryu/iggy/re3-miami/src/peds /Users/kogaryu/iggy/re3-miami/src/core /Users/kogaryu/iggy/re3-miami/src/control | sed -n '1,340p'
```

Direct file reads used `nl -ba ... | sed -n` around the line anchors cited
below.

## Vocabulary Lens

- Intent/order/objective: durable request such as patrol, attack, flee, move,
  guard, follow, observe.
- Active behavior state: immediate state machine state such as walking, seeking,
  fleeing, attacking, waiting, dying.
- Movement controller/path state: route result, current path index, current
  step, walk phase, or path thread result.
- Occupancy/collision structure: tile occupant pointer/id, dungeon occupancy
  grid, world sector lists, or physics collision broad-phase.
- Map semantic data: patrol nodes, danger flags, visibility/LOS, path graph
  nodes, attractors, doors, traps, light, region/sector membership.
- Projection/display: glyphs, tile cache, map sprites, debug labels, renderer
  lists. These are updated from authoritative state, not treated as the state.

## NetHack

Source anchors:

- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:96-202`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:704-988`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/mon.c:1329-1355`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/mon.c:2143-2335`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/rm.h:500-535`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/display.c:912-980`

State ownership:

- `struct monst` carries live monster state: static species pointer, current
  position, remembered target, track memory, inventory, HP, movement points,
  flee flags, sleep/confusion/can-move flags, peaceful/tame bits, strategy bits,
  goal coordinate, and last move turn.
- The level owns per-cell monster occupancy through `svl.level.monsters[x][y]`.
  `m_at`, `MON_AT`, and `remove_monster` are macros over that level array.
- Static monster definition data is reached through `monst::data`, not copied
  into each monster.

Decision to execution:

- The monster loop is `movemon()`, which walks the monster list and calls the
  monster turn logic while preserving iterator safety across death, migration,
  and level transfer.
- `monmove.c` gates many early states before movement: waiting, frozen, asleep,
  strategy arrival, defensive item use, miscellaneous item use, and special
  actions.
- Movement returns status-like outcomes (`nothing`, `done`, `moved`, `died`,
  no moves), and the caller branches into post-move traps, grabs, and attacks.

Path and movement style:

- Common monster movement is local and candidate-based, not long-path owned per
  actor.
- `mfndpos()` scans neighboring cells, filters terrain, doors, diagonal rules,
  water/lava, scary cells, hero occupancy, monster occupancy, sanctuary rules,
  and object hazards.
- Special actors such as pets and guards add extra logic, but still plug into
  the same level occupancy/display substrate.

Occupancy and collision:

- Tile occupancy is direct and authoritative for monsters: `m_at(x, y)` reads
  the current monster pointer in the level cell.
- Moving code removes and places monsters by mutating the level occupancy array.
- Some movement functions can displace or move another monster, but this is
  explicit special handling rather than a general reservation system.

Map semantics:

- Movement checks map terrain, doors, rooms, temples, sanctuary, water/lava,
  traps, objects, poison gas regions, hero visibility/displacement, and monster
  relationship flags.
- The map is not just geometry; it is a dense semantic query surface.

Display and debug separation:

- `newsym(x, y)` recalculates the visible glyph for a cell from level state,
  visibility, monster occupancy, regions, objects, and memory.
- `movemon()` can request full vision recalculation when a moving light source
  changes visibility.

Compute notes:

- Cost shape is mostly `monsters this turn * local neighbor scan`.
- Candidate generation is bounded to adjacent cells, but each candidate can
  touch many semantic checks.
- Display work is dirty-cell oriented through `newsym`, with full vision
  recalculation reserved for cases such as moved light sources.
- Failure/replan cases are status returns: no legal move, no action, moved,
  died, special attack, or deferred migration.

## DevilutionX

Source anchors:

- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:40-55`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:75-129`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1732-1814`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1816-1891`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4257-4337`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4356-4382`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:5035-5039`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/diablo.cpp:1521-1542`

State ownership:

- Monster definitions and flags describe capabilities such as searching,
  opening doors, targeting monsters, or special behavior.
- `MonsterGoal` is durable goal-like state: normal, retreat, healing, move,
  attack, inquiring, talking.
- `MonsterMode` is immediate animation/action state: stand, walk directions,
  melee, ranged attack, death, charge, heal, talk, and similar modes.
- The dungeon owns occupancy grids such as `dMonster` and `dPlayer`.

Decision to execution:

- `GameLogic()` runs players, then monsters, objects, missiles, items, and light.
- `ProcessMonsters()` loops active monsters, updates enemy info, active ticks,
  goal/path state, behavior procs, stance, group unity, and animation.
- Behavior functions choose or continue actions; mode/animation state then
  advances the visible/action execution.

Path and movement style:

- `AiPlanWalk()` builds a local fixed path buffer, calls pathfinding, and uses
  the first direction through `RandomWalk()`.
- `AiPlanPath()` throttles path planning with active ticks and `pathCount`, and
  short-circuits if line of sight is clear or the monster is not in a planning
  state.
- Direction checks use `DirOK()` and related terrain/occupancy helpers.

Occupancy and collision:

- `IsTileAvailable()` checks player and monster grids plus walkability.
- `Monster::occupyTile()` writes the monster id to `dMonster`; negative ids are
  used for moving/future occupancy.
- This is a live occupancy grid, not a separate abstract reservation manager.

Map semantics:

- Monster movement reads dungeon flags, door state, missile/lightning danger,
  line clear checks, transparency/visibility-style data, leader leash distance,
  and group/minion relations.

Display and debug separation:

- Movement modes and animation state are separate from the dungeon occupancy
  grid. Rendering observes monster state and dungeon state.

Compute notes:

- Cost shape is `active monsters * behavior/path/line checks`.
- Pathfinding is avoided or delayed when line clear or state gates make it
  unnecessary.
- Occupancy checks are O(1) grid reads. Leader/minion leash checks add bounded
  local area scans.
- Failure/replan cases include blocked directions, failed path, retreat/goal
  changes, death, inactive ticks, and stale enemy position.

## OpenXcom

Source anchors:

- `/Users/kogaryu/iggy/openxcom-master/src/Battlescape/AIModule.cpp:49-153`
- `/Users/kogaryu/iggy/openxcom-master/src/Battlescape/AIModule.cpp:247-360`
- `/Users/kogaryu/iggy/openxcom-master/src/Battlescape/AIModule.cpp:782-981`
- `/Users/kogaryu/iggy/openxcom-master/src/Battlescape/Pathfinding.cpp:80-179`
- `/Users/kogaryu/iggy/openxcom-master/src/Battlescape/Pathfinding.cpp:600-625`
- `/Users/kogaryu/iggy/openxcom-master/src/Battlescape/UnitWalkBState.cpp:61-75`
- `/Users/kogaryu/iggy/openxcom-master/src/Battlescape/UnitWalkBState.cpp:80-230`
- `/Users/kogaryu/iggy/openxcom-master/src/Battlescape/UnitWalkBState.cpp:249-390`
- `/Users/kogaryu/iggy/openxcom-master/src/Savegame/SavedBattleGame.cpp:1290-1360`
- `/Users/kogaryu/iggy/openxcom-master/src/Savegame/SavedBattleGame.cpp:1584-1638`
- `/Users/kogaryu/iggy/openxcom-master/src/Savegame/Tile.h:60-83`
- `/Users/kogaryu/iggy/openxcom-master/src/Savegame/Tile.h:220-235`

State ownership:

- `AIModule` owns a small amount of persistent AI memory: mode, previous/from/to
  nodes, and hit memory.
- The AI turn produces a `BattleAction` such as walk, shoot, throw, launch, or
  rethink.
- `SavedBattleGame` owns battlefield data, tiles, units, and pathfinding access.
- `Tile` owns tile occupant pointer, inventory, light, fire, smoke, visibility,
  danger flag, preview marker, and debug marker state.

Decision to execution:

- `AIModule::think()` computes known/visible/spotting enemies, reachable tiles,
  candidate actions, and mode evaluation.
- AI modes select one action candidate: escape, patrol, combat, or ambush.
- Walking is executed by `UnitWalkBState`, which consumes the current pathfinder
  direction, spends TU/energy, opens doors, updates unit walk phase, updates tile
  occupancy, and stops for spotting/reaction fire.

Path and movement style:

- `Pathfinding::calculate()` validates bounds, destination blockers, big unit
  blockers, stairs/falling cases, tries direct same-level path, then computes a
  path.
- The pathfinder owns the current path until `dequeuePath()` consumes directions
  or `abortPath()` clears it.
- `UnitWalkBState` asks for the start direction, computes destination and TU
  cost, then dequeues or aborts based on the movement result.

Occupancy and collision:

- `SavedBattleGame::setUnitPosition()` supports `testOnly` placement, checks
  tile occupancy, floor/wall constraints, large units, and terrain blocking.
- Actual movement clears the old tile occupant and sets the new tile occupant
  when the unit crosses to another tile.
- Final movement still checks the next tile for units before walking into it.

Map semantics:

- Patrol node selection reads node flags, rank/desirability, danger, allocation,
  unit type, flying/small restrictions, fire, and `setUnitPosition(..., true)`
  feasibility.
- Escape selection samples a shuffled local tile search, scores exposure,
  distance from threat, fire, danger, reachability, and path validity.
- FOV and visibility are recalculated after movement and can stop movement if a
  unit spots enemies or triggers reaction fire.

Display and debug separation:

- Tile preview markers, marker colors, and TU markers are debug/projection data
  layered onto tiles.
- Unit sprite cache is refreshed by map cache calls after direction, stance, or
  position changes.

Compute notes:

- Cost shape includes AI candidate scoring, reachable tile computation, path
  calculation, FOV recalculation, reaction checks, and sprite cache updates.
- Escape can try up to 150 candidates; high scores can terminate early.
- For accepted escape candidates, pathfinding is called to prove the candidate
  can be walked.
- Failure/replan cases include no action, invalid path, not enough TU/energy,
  occupied next tile, closed door wait, spotted enemy, reaction fire, falling,
  and no valid escape tile.

## Warzone 2100

Source anchors:

- `/Users/kogaryu/iggy/warzone2100-master/src/actiondef.h:27-77`
- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:41-135`
- `/Users/kogaryu/iggy/warzone2100-master/src/droiddef.h:181-205`
- `/Users/kogaryu/iggy/warzone2100-master/src/ai.cpp:600-660`
- `/Users/kogaryu/iggy/warzone2100-master/src/ai.cpp:1136-1270`
- `/Users/kogaryu/iggy/warzone2100-master/src/mapgrid.cpp:206-245`
- `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:235-335`
- `/Users/kogaryu/iggy/warzone2100-master/src/fpath.cpp:51-105`
- `/Users/kogaryu/iggy/warzone2100-master/src/fpath.cpp:420-545`
- `/Users/kogaryu/iggy/warzone2100-master/src/astar.cpp:21-42`
- `/Users/kogaryu/iggy/warzone2100-master/src/astar.cpp:160-220`

State ownership:

- `DroidOrderType` is the durable order layer: move, attack, build, observe,
  repair, scout, patrol, rearm, recover, guard, hold, circle, and others.
- `DROID_ACTION` is the immediate action layer: move, build, attack, observe,
  wait/repair/rearm, move-to-attack, rotate-to-attack, return-to-position, etc.
- `DROID` stores action fields, action targets, secondary orders, movement
  control `sMove`, previous position, and blocker bits.
- The path subsystem produces `MOVE_CONTROL` results for a droid.

Decision to execution:

- `aiUpdateDroid()` checks current order/action, queued orders, commander
  assignment, secondary attack/hold settings, target state, and target refresh
  cadence.
- AI can set a durable order and call `actionDroid()` to set an immediate
  action.
- Movement requests go through `moveDroidToBase()` and pathfinding, then `sMove`
  navigates the returned route.

Path and movement style:

- `moveDroidToBase()` calls `fpathDroidRoute()` unless the unit can use a direct
  route. Success sets `MOVENAVIGATE` and path index 0. Wait sets
  `MOVEWAITROUTE`. Failure sets inactive movement and a sulk action.
- `fpathRoute()` has an async-style path job system keyed by droid id. It polls
  existing results, discards stale destination results, or queues a new job.
- `astar.cpp` caches path contexts for repeated destinations within a tick and
  stores explored tiles plus a blocking map.

Occupancy and collision:

- Blocking is represented in path blocking maps by propulsion, owner, move type,
  and game time.
- Objects are found through map/grid broad-phase lists for targeting and repair
  queries.
- Formation handling joins or creates formations after a path succeeds; it is a
  coordination layer separate from the core path result.

Map semantics:

- AI uses object visibility, sensor range, alliance checks, weapon range,
  commander assignment, secondary order flags, and map grid iteration.
- Pathfinding uses blocking maps and optional destination ignore areas.

Display and debug separation:

- AI and movement state live in droid/order/action/path structures. Rendering
  and debug path displays observe those structures.

Compute notes:

- Cost shape includes target scans over grid lists, weapon/visibility checks,
  path jobs, and path context reuse.
- Static `GridList` reuse avoids repeated allocation in target search.
- Pathfinding is thread-backed with up to two path threads and a per-droid
  future/result table.
- A* caches up to 30 maps/contexts for repeated destination queries.
- Failure/replan cases include invalid start/end, already at target, stale path
  result, path wait, failed path, dead target, queued orders, commander override,
  secondary order restrictions, and target refresh throttling.

## re3 Miami

Source anchors:

- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:190-252`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:500-575`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:590-675`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:1701-1729`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:5510-5725`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:5728-5810`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:5867-5935`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.h:28-122`
- `/Users/kogaryu/iggy/re3-miami/src/core/World.h:8-48`
- `/Users/kogaryu/iggy/re3-miami/src/core/World.h:52-110`
- `/Users/kogaryu/iggy/re3-miami/src/core/Lists.h:73-112`
- `/Users/kogaryu/iggy/re3-miami/src/entities/Physical.cpp:114-135`
- `/Users/kogaryu/iggy/re3-miami/src/core/World.cpp:278-312`
- `/Users/kogaryu/iggy/re3-miami/src/core/World.cpp:883-925`

State ownership:

- `eObjective` is the high-level ped objective layer: wait, flee, guard, kill,
  go to char, follow formation, enter/leave car, follow route, use attractor,
  wander, sprint to area, and others.
- `CPed` stores objective, previous objective, objective entities, threat data,
  ped state, last ped state, move state, wait state, path nodes, current/next
  path node, seek position/entity, flee data, and timers.
- `CPedAttractorManager` owns lists of world interaction attractors such as
  ATMs, seats, bus stops, pizza, shelters, and ice cream.
- `CWorld` owns sector lists and moving entity lists for spatial queries.

Decision to execution:

- `CPed::ProcessControl()` is the per-frame control hub for peds; it prunes
  references, updates visibility alpha, builds nearby ped lists, and processes
  physics/water/control state.
- Objective setters store high-level target semantics. Seek/flee functions turn
  those into active ped states and move states.
- `SetMoveState()` changes the movement intensity, while `SetMoveAnim()` maps
  move state to animation.

Path and movement style:

- `SetSeek()` sets `PED_SEEK_POS` or `PED_SEEK_ENTITY`, a seek target/position,
  and done distance.
- `Seek()` chooses a move speed from objective and distance, checks obstruction
  at the target area, follows path nodes when present, rotates toward the next
  target, updates movement animation, and returns when destination criteria are
  satisfied.
- `SetFlee()` stores source position/entity, sets flee ped state, run move
  state, node seeking, and timer. `Flee()` uses path nodes when available or
  updates rotation away from the flee source.

Occupancy and collision:

- The open-world model uses sectors and physical collision, not tile occupancy.
- `CWorld` partitions the world into 80 by 80 sectors, with separate lists for
  buildings, vehicles, peds, objects, and overlap lists.
- `CEntryInfoNode` records the list and sector membership for physical objects.
- Collision and LOS functions iterate only relevant sector lists.

Map semantics:

- Semantic sources include path nodes, route points, threat/event fields,
  attractors, vehicle state, script-assigned objectives, zones/sectors, line of
  sight, and collision queries.
- Ped objectives often hold references to entities rather than only positions.

Display and debug separation:

- Ped debug code can print state/objective/seek information, but authoritative
  behavior lives in ped state, objectives, path nodes, world sectors, and
  physical movement.

Compute notes:

- Cost shape includes per-frame ped control, nearby ped list building, sector
  collision queries, path node following, LOS/physics queries, and animation
  blending.
- Sector lists reduce broad-phase query cost from whole-world scans to local
  sector scans.
- Flee logic uses timers, node choice, and collision reaction rather than
  discrete tile reservation.
- Failure/replan cases include invalid seek target, target obstruction,
  objective entity gone, flee timer expiry, collision while fleeing, impossible
  path node, and script/objective completion.

## Cross-Repo Compute Notes

- Tile games often pay for local candidate scans and O(1) tile occupancy checks.
- Tactical games pay for pathfinding, FOV/visibility, reaction checks, and
  candidate scoring.
- RTS games pay for many unit updates, target scans, path jobs, cached path
  maps, and broad-phase object grids.
- Open-world games pay for continuous per-frame controller updates, physics,
  sector broad-phase, LOS, path node following, and animation blending.
- Explicit dirty/projection updates appear as `newsym`, map sprite cache refresh,
  visibility recalculation flags, and sector membership maintenance.
- Full recompute is usually avoided by one of three methods: local bounded
  scans, dirty updates, or cached/shared path/search structures.

## Cross-Repo Failure and Replan Patterns

- Already at destination: stop or return failure/no move.
- Invalid goal: clear action, rethink, or fail route.
- No legal local move: stay, attack, use item, or no-op.
- Occupied next tile: abort path, stop, displace only if special rules allow it,
  or retry later.
- Stale path result: discard and queue a new path.
- Partial path: accept nearest reachable point or stop with failure.
- New threat/visibility event: abort movement or switch mode.
- Actor gone/dead: clear objective/action target.
- Timer expired: restore previous state, wait, or return to idle/wander.
- Collision while fleeing: alter heading, path node choice, or wait state.
