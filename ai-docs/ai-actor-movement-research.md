# AI Actor Movement Research

This document studies how the reference games turn actor intent into movement.
It is a learning map of code shape, ownership, mutation, and compute cost. It is
not a recommendation doc and it does not copy source code.

## Core Movement Skeleton

Across the references, actor movement usually separates into six layers:

```text
objective / order / intent
  -> path or candidate generation
  -> movement state / path-follow state
  -> authoritative position mutation
  -> map occupancy / collision mutation
  -> post-move recompute and effects
```

Movement is rarely just assigning `x` and `y`. Mature games attach position
mutation to direction, path progress, occupancy, animation, collision, vision,
lighting, triggers, and save/session state.

## What To Look For

When reading actor movement code, classify every field or function as one of:

- `goal`: why the actor wants to move
- `route`: where it plans to go
- `step`: the next immediate move
- `executor`: code that performs the move
- `truth`: authoritative position and occupancy state
- `cache`: visibility, path, collision, or render state rebuilt from truth
- `post-move`: work triggered after position changes

The important question is:

```text
Who is allowed to mutate actor position, and what else must be updated when it does?
```

## OpenXcom

### Movement Shape

```text
battle action
  -> Pathfinding calculates route
  -> UnitWalkBState owns walking execution
  -> BattleUnit stores position, direction, walk phase
  -> TileEngine updates FOV, doors, gravity, and tile interactions
```

### Source Anchors

- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.cpp:262`:
  walk action asks pathfinding to calculate a path.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.cpp:266`:
  walking is pushed as a battle state.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/UnitWalkBState.cpp:45`:
  walking state object is created for a unit action.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/UnitWalkBState.cpp:61`:
  walking state initialization.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/UnitWalkBState.cpp:80`:
  walking state tick/update.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/UnitWalkBState.cpp:118`:
  continues walking along the current route.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/UnitWalkBState.cpp:396`:
  route direction is dequeued from pathfinding.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/UnitWalkBState.cpp:407`:
  next walking step starts.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/UnitWalkBState.cpp:484`:
  post-path work after route execution.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/UnitWalkBState.cpp:526`:
  FOV recalculation tied to movement.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/Pathfinding.cpp:202`:
  path search open list is populated.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/Pathfinding.cpp:207`:
  path search consumes nodes from the open list.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/Pathfinding.cpp:610`:
  path directions are consumed one step at a time.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/BattleUnit.cpp:491`:
  unit position setter.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/BattleUnit.cpp:529`:
  unit direction setter.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/BattleUnit.cpp:608`:
  actor begins a walking phase.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/BattleUnit.cpp:652`:
  actor continues a walking phase.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/TileEngine.cpp:228`:
  per-unit FOV calculation.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/TileEngine.cpp:2003`:
  door interaction is routed through tile engine behavior.
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/TileEngine.cpp:2706`:
  gravity can be applied after map/unit changes.

### Ownership Lesson

OpenXcom keeps the movement executor separate from the unit data. `BattleUnit`
holds actor truth. `Pathfinding` holds route/candidate work. `UnitWalkBState`
drives the live walking process. `TileEngine` owns terrain queries and post-move
world consequences.

### Compute Shape

- path search: `O(path area)`
- walking execution: `O(step)`
- FOV after movement: `O(visible tiles)` around the moved unit
- tile/door/gravity side effects: usually `O(local tiles)`, sometimes broader

## Warzone 2100

### Movement Shape

```text
order
  -> action
  -> move control route
  -> per-frame droid movement update
  -> position, direction, speed, blocking, and waypoint progress
```

### Source Anchors

- `/Users/kogaryu/iggy/warzone2100-master/src/droid.cpp:927`:
  droid update calls order update.
- `/Users/kogaryu/iggy/warzone2100-master/src/droid.cpp:939`:
  droid update calls action update.
- `/Users/kogaryu/iggy/warzone2100-master/src/droid.cpp:945`:
  droid update calls movement update.
- `/Users/kogaryu/iggy/warzone2100-master/src/order.cpp:415`:
  per-droid order update.
- `/Users/kogaryu/iggy/warzone2100-master/src/action.cpp:680`:
  current action update.
- `/Users/kogaryu/iggy/warzone2100-master/src/movedef.h:45`:
  movement control struct.
- `/Users/kogaryu/iggy/warzone2100-master/src/movedef.h:48`:
  path index records route progress.
- `/Users/kogaryu/iggy/warzone2100-master/src/movedef.h:61`:
  bump/blocking state is part of movement control.
- `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:235`:
  movement order setup.
- `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:342`:
  droid movement target API.
- `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:635`:
  path index is used during movement.
- `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:692`:
  path endpoint comparison.
- `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:870`:
  movement blocking/slide handling.
- `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:1191`:
  avoidance-related movement helper.
- `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:1386`:
  movement speed calculation.
- `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:1479`:
  direction update.
- `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:1588`:
  final waypoint check.
- `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:1611`:
  position update.
- `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:2165`:
  top-level droid movement tick.
- `/Users/kogaryu/iggy/warzone2100-master/src/droid.cpp:3888`:
  droid position setter.

### Ownership Lesson

Warzone splits durable order state from immediate action state and movement
control. A droid carries live movement fields, but movement execution is
centralized in movement code. Route progress is explicit with path indices.

### Compute Shape

- per-droid update: `O(actor)`
- action/order dispatch: usually `O(1)` before expensive calls
- path following: `O(step)` plus local collision checks
- path search and reroute: `O(path area)` when movement target changes or route fails
- blocking/avoidance: `O(local actors or local tiles)`

## NetHack

### Movement Shape

```text
monster turn
  -> decide if monster can act
  -> generate legal move positions
  -> choose move or action
  -> remove/place monster on map
  -> run post-move effects
```

### Source Anchors

- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:102`:
  live monster movement points.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:111`:
  live monster track/history field.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:170`:
  monster strategy field.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:213`:
  dead monsters remain on the list until end-of-turn cleanup.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mfndpos.h:33`:
  candidate position data struct.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/mon.c:1124`:
  movement amount is calculated for the current monster turn.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/mon.c:1211`:
  single monster movement/update entry.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/mon.c:1329`:
  monster list movement loop.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:690`:
  main monster action/movement procedure.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1455`:
  post-move handling.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1492`:
  remove/place style mutation during post-move.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1709`:
  movement function entry.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1927`:
  legal move position generation.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:2049`:
  post-move effect handling before map placement is finalized.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:2090`:
  aggressive movement behavior.

### Ownership Lesson

NetHack is turn based and map-centric. Candidate generation is temporary.
Monster structs hold live actor truth and strategy. The map holds occupancy.
Movement mutation is tied to remove/place functions and delayed cleanup rules.

### Compute Shape

- monster turn loop: `O(actor)`
- legal neighbor generation: `O(local tiles)`
- special behavior checks: branch-dependent
- map remove/place: `O(1)` for grid occupancy
- delayed cleanup: `O(dead or changed actors)`

## DevilutionX

### Movement Shape

```text
monster mode / goal
  -> AI dispatch
  -> path or direction choice
  -> direction legality check
  -> walking mode and future position
  -> dungeon occupancy update
```

### Source Anchors

- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:75`:
  monster mode enum includes movement modes.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:99`:
  helper for identifying movement modes.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:120`:
  monster goal enum.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:248`:
  actor position field.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:251`:
  monster goal field.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:256`:
  monster mode field.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:257`:
  path count field.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:444`:
  walking visual offset.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:480`:
  occupancy helper documents map occupancy mutation.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:546`:
  monster walk API.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:552`:
  direction legality API.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:196`:
  monster initialization sets position/mode/goal/path state.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:293`:
  monster placement enters map occupancy.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4144`:
  walking entry.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4146`:
  direction legality checked before movement.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4257`:
  active monster processing loop.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4322`:
  path planning before AI dispatch.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4637`:
  monster lookup by map position.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4682`:
  tile availability query.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4922`:
  walking state query.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:5038`:
  dungeon occupancy stores current or moving monster id.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/engine/path.cpp:184`:
  pathfinding entry.
- `/Users/kogaryu/iggy/DevilutionX-master/Source/engine/path.cpp:246`:
  pathfinder neighbor expansion.

### Ownership Lesson

DevilutionX makes dungeon occupancy explicit. Actor position has current,
future, and old components. Movement modes explain whether a monster is
standing, walking, attacking, or otherwise busy. Occupancy can encode movement
state, not only presence.

### Compute Shape

- active monster loop: `O(actor)`
- path planning: `O(path area)` when needed
- direction legality: `O(1)` or `O(local tile)`
- occupancy mutation: `O(1)`
- target/path/LOS branches: potentially more expensive than movement state itself

## KeeperFX

### Movement Shape

```text
creature state / job
  -> navigation setup
  -> Ariadne route and waypoint state
  -> creature movement update
  -> velocity/acceleration applied
  -> thing map position updated
```

### Source Anchors

- `/Users/kogaryu/iggy/keeperfx-master/src/ariadne.h:7`:
  route/pathfinding system header.
- `/Users/kogaryu/iggy/keeperfx-master/src/ariadne.h:146`:
  route-following state struct.
- `/Users/kogaryu/iggy/keeperfx-master/src/ariadne.h:151`:
  current waypoint state.
- `/Users/kogaryu/iggy/keeperfx-master/src/ariadne.h:169`:
  waypoint list storage.
- `/Users/kogaryu/iggy/keeperfx-master/src/ariadne.h:258`:
  route initialization API.
- `/Users/kogaryu/iggy/keeperfx-master/src/ariadne.h:266`:
  route invalidation API.
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:142`:
  creature control struct.
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:159`:
  movement target position.
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:201`:
  movement flags.
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:333`:
  movement acceleration.
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:340`:
  movement speed.
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:362`:
  navigation state inside creature control.
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:364`:
  route-following state inside creature control.
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_navigate.h:59`:
  setup move to position API.
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_navigate.h:72`:
  navigation feasibility query.
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_navigate.h:81`:
  creature move-to API.
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_navigate.h:82`:
  map movement mutation API.
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_navigate.c:153`:
  move setup stores target and flags.
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_navigate.c:344`:
  map position mutation removes and re-adds thing in map structures.
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_navigate.c:423`:
  route feasibility check.
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_creature.c:5890`:
  controlled creature movement update.
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_creature.c:5965`:
  creature movement update entry.
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_creature.c:6427`:
  movement acceleration feeds velocity.
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_creature.c:6459`:
  thing map movement is committed.

### Ownership Lesson

KeeperFX separates creature decision/control data from route-follow state.
`CreatureControl` holds live movement targets and route state. Map placement is
handled through thing-map movement functions, not by free position assignment.

### Compute Shape

- creature movement update: `O(actor)`
- route setup/search: `O(path area)`
- waypoint following: `O(step)`
- map placement mutation: `O(1)` or `O(local map list)`
- room/job navigation checks: branch-dependent and can trigger route work

## re3 Miami

### Movement Shape

```text
objective
  -> ped state
  -> move state
  -> seek/follow path
  -> animation movement delta and speed
  -> world position/collision response
```

### Source Anchors

- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:280`:
  ped state enum.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:353`:
  movement state enum.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:523`:
  animation movement delta.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:531`:
  move state field.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:544`:
  path direction field.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:545`:
  next path node field.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:567`:
  seek position.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:672`:
  movement animation setter.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:717`:
  objective setter.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:730`:
  seek setup.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:741`:
  seek execution.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:743`:
  follow path setup.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:806`:
  objective processing.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:808`:
  path following update.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:837`:
  position correction out of collision.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:857`:
  follow path cleanup.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:928`:
  movement state setter.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:961`:
  position update.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:979`:
  state change can clear path-follow state.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:6208`:
  follow path setup.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:6260`:
  static follow path setup.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:9160`:
  movement force application.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:9172`:
  movement speed is applied to actor motion.
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:9544`:
  ped save records position-related state.
- `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.h:203`:
  path graph owner class.
- `/Users/kogaryu/iggy/re3-miami/src/control/AutoPilot.h:94`:
  vehicle autopilot path node state.

### Ownership Lesson

re3 separates `objective`, `ped state`, and `move state`. That is the important
learning point. A ped can have a high-level reason, a current behavior state,
and a movement intensity/locomotion mode at the same time. Path graph ownership
lives outside the ped.

### Compute Shape

- ped update: `O(actor)`
- objective/state dispatch: usually `O(1)`
- path node following: `O(step)`
- collision/position correction: `O(local geometry or local actors)`
- path graph queries: `O(path graph area)` when requested

## Cross-Game Patterns

### 1. Movement Has At Least Two Truths

Most games separate:

- actor truth: position, direction, mode, speed, route index
- map truth: occupancy, blocking, thing lists, path graph, tile solidity

The actor can say "I am moving." The map decides whether the new location is
valid and records what now occupies that space.

### 2. Path Is Usually Temporary Runtime State

Route data is often not the same as actor truth:

- OpenXcom: path directions are consumed by walking state.
- Warzone: path index tracks current route progress.
- NetHack: candidate positions are generated per monster move.
- DevilutionX: path count and path planning support monster mode/goal.
- KeeperFX: Ariadne route data stores waypoint-follow state.
- re3: next path node and path direction are live follow state.

### 3. Movement State Is Not The Same As Objective

Games often have several layers:

```text
goal/objective/order = why
state/mode/action = what behavior is active
move mode/path phase = how locomotion is being executed
```

This is visible in Warzone orders/actions/move control, re3
objective/state/move-state, DevilutionX goal/mode/path state, and KeeperFX
creature state/navigation/Ariadne.

### 4. Position Setters Are Rarely Enough

Search for position setters, then search for who calls them. In these games,
the real movement path usually goes through higher-level functions that update
occupancy and side effects.

Examples:

- OpenXcom has `BattleUnit::setPosition`, but walking flows through
  `UnitWalkBState`.
- Warzone has a droid position setter, but movement flows through
  `moveUpdateDroid`.
- DevilutionX has actor position fields, but dungeon occupancy is updated by
  monster placement/occupancy helpers.
- KeeperFX has thing positions, but movement flows through thing-map movement.
- re3 has position updates, but peds also use state, path, collision, and
  animation movement.

### 5. Post-Move Work Is A Major Cost Center

Movement can trigger:

- visibility/FOV rebuilds
- lighting checks
- occupancy writes
- collision resolution
- door/trap/tile effects
- line-of-sight or target reacquisition
- path invalidation
- animation state changes
- event or audio emission
- save/session dirty state

The step may be cheap. The consequences can be expensive.

## Actor Movement Compute Costs

| Cost | Shape | Where Seen |
|---|---:|---|
| Objective/order dispatch | `O(actor)` | Warzone, re3, KeeperFX, DevilutionX |
| Candidate neighbor generation | `O(local tiles)` | NetHack, DevilutionX, OpenXcom |
| Full path search | `O(path area)` | OpenXcom, Warzone, DevilutionX, KeeperFX, re3 |
| Path following | `O(step)` | OpenXcom, Warzone, KeeperFX, re3 |
| Direction/facing update | `O(1)` | OpenXcom, Warzone, DevilutionX, re3 |
| Occupancy update | `O(1)` or `O(local list)` | NetHack, DevilutionX, KeeperFX |
| Collision correction | `O(local geometry)` | Warzone, KeeperFX, re3 |
| FOV/visibility after move | `O(visible tiles)` | OpenXcom, DevilutionX-like ARPG checks |
| Cleanup/removal | `O(changed actors)` | NetHack, Warzone, DevilutionX |

## Code Reading Method

For movement, do not start with the AI decision function. Start with the
position mutation and work backward.

### Step 1: Find Position Truth

Search terms:

```bash
rg -n "position|Position|pos\\.|x\\b|y\\b|tile|Tile|coord|Coord" <repo-path>
```

Why this works:

- actor movement must eventually mutate coordinates or tile ids
- broad search is noisy, but it reveals the owning structs/classes

### Step 2: Find Movement Executors

Search terms:

```bash
rg -n "move|Move|walk|Walk|seek|Seek|follow|Follow|path|Path" <repo-path>
```

Why this works:

- movement executor functions often contain verbs
- combine with actor names to reduce noise, such as `Droid`, `Ped`, `Monster`,
  `Creature`, `Unit`

### Step 3: Find Route State

Search terms:

```bash
rg -n "pathIndex|pathCount|waypoint|route|nextPath|m_pNextPathNode|dequeuePath|Ariadne" <repo-path>
```

Why this works:

- route-follow state usually needs an index, pointer, count, or next-node field
- this reveals how movement progresses after pathfinding finishes

### Step 4: Find Occupancy Mutation

Search terms:

```bash
rg -n "occupy|place|remove|mapwho|dMonster|blocking|solid|available|canMove|DirOK" <repo-path>
```

Why this works:

- mature games do not trust actor coordinates alone
- maps keep blocking, occupancy, and spatial lookup structures

### Step 5: Find Post-Move Work

Search terms:

```bash
rg -n "post|Post|FOV|visibility|visible|light|gravity|collision|trigger|door|recalc|invalidate" <repo-path>
```

Why this works:

- movement side effects are often named as post steps or recalculations
- this exposes the hidden cost after a move is accepted

## Grep Commands Used For This Study

### OpenXcom

```bash
rg -n "class Pathfinding|calculate\\(|dequeuePath|UnitWalkBState|startWalking|keepWalking|abortPath|setPosition|setDirection|openDoor|TileEngine|canMove|validateUpDown" \
  /Users/kogaryu/iggy/OpenXcom-master/src/Battlescape \
  /Users/kogaryu/iggy/OpenXcom-master/src/Savegame
```

### Warzone 2100

```bash
rg -n "struct MOVE_CONTROL|MOVE_CONTROL|moveUpdateDroid\\(|moveUpdateDroidPos|moveUpdateDroidDirection|moveCalc|moveDroidTo|moveCheck|path|Position|orderUpdateDroid|actionUpdateDroid" \
  /Users/kogaryu/iggy/warzone2100-master/src/movedef.h \
  /Users/kogaryu/iggy/warzone2100-master/src/move.cpp \
  /Users/kogaryu/iggy/warzone2100-master/src/droid.cpp \
  /Users/kogaryu/iggy/warzone2100-master/src/order.cpp \
  /Users/kogaryu/iggy/warzone2100-master/src/action.cpp
```

### NetHack

```bash
rg -n "movemon\\(|dochug\\(|m_move\\(|mfndpos\\(|remove_monster|place_monster|mtrack|movement|MMOVE_|postmov|m_postmove_effect|m_move_aggress|mnexto|enexto" \
  /Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h \
  /Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mfndpos.h \
  /Users/kogaryu/iggy/NetHack-NetHack-5.0/src/mon.c \
  /Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c
```

### DevilutionX

```bash
rg -n "Start.*Walk|M_Start|StartWalk|AiPlanPath|FindPath|LineClear|Walk|MonsterMode|MonsterGoal|Move|monster.*position|dMonster|position|pathCount|ProcessMonsters|M_.*Walk|MAI|Dir" \
  /Users/kogaryu/iggy/DevilutionX-master/Source/monster.h \
  /Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp \
  /Users/kogaryu/iggy/DevilutionX-master/Source/engine/path.h \
  /Users/kogaryu/iggy/DevilutionX-master/Source/engine/path.cpp
```

### KeeperFX

```bash
rg -n "navigate|Navigation|move|movement|thing_in_map|thing.*position|move_creature|CreatureControl|Ariadne|path|route|waypoint|State|process_func_list|set_thing_position|move_thing|thing_in_wall|creature.*move" \
  /Users/kogaryu/iggy/keeperfx-master/src/thing_navigate.c \
  /Users/kogaryu/iggy/keeperfx-master/src/thing_navigate.h \
  /Users/kogaryu/iggy/keeperfx-master/src/thing_creature.c \
  /Users/kogaryu/iggy/keeperfx-master/src/creature_control.h \
  /Users/kogaryu/iggy/keeperfx-master/src/ariadne.h \
  /Users/kogaryu/iggy/keeperfx-master/src/creature_states.c
```

### re3 Miami

```bash
rg -n "SetFollowPath|FollowPath|PathFind|m_nMoveState|PedState|SetMoveState|Seek|Move|SetObjective|ProcessObjective|m_pNextPathNode|m_nPathDir|m_vecMoveSpeed|m_vecAnimMoveDelta|Position|CPed::Process|CPed::Save" \
  /Users/kogaryu/iggy/re3-miami/src/peds/Ped.h \
  /Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp \
  /Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp \
  /Users/kogaryu/iggy/re3-miami/src/control/PathFind.h \
  /Users/kogaryu/iggy/re3-miami/src/control/PathFind.cpp \
  /Users/kogaryu/iggy/re3-miami/src/control/AutoPilot.h
```

## Short Study Checklist

When reading a new actor movement implementation, answer these in order:

1. What struct/class owns actor position?
2. What map/grid/list owns occupancy?
3. What function is allowed to commit a move?
4. What route/path state exists between pathfinding and movement?
5. What state says the actor is currently walking, blocked, seeking, or idle?
6. What post-move work runs after position changes?
7. What movement data is saved?
8. What movement data is recomputed after load?
9. What work is done once per actor?
10. What work is done only when the map, target, or route changes?
