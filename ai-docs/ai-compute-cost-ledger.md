# AI Compute Cost Ledger

This is a learning list of compute costs seen in the reference games. It is not
benchmark data. It is an inferred cost map from code shape, ownership, and update
flow.

Use it to ask:

```text
What work does this game spend CPU on?
What work did the authors make cheap with data structures?
What work did they avoid doing every actor/every tick?
```

## Cost Scale

| Label | Meaning |
|---|---|
| `O(1)` | fixed cost, usually field reads, flags, enum switches |
| `O(actor)` | once per actor/unit/monster/ped |
| `O(actor * target)` | each actor scans possible targets |
| `O(local tiles)` | nearby tile/candidate checks |
| `O(path area)` | path search over graph/grid area |
| `O(map)` | whole map/grid/cache pass |
| `O(queue)` | drains pending orders/events/effects |
| `O(save size)` | save/load or post-load repair size |

## Compute Cost List

### 1. State Dispatch

Cost shape:

```text
O(1) per actor
```

What it does:

- chooses behavior function by enum, mode, state, action, objective, or AI id
- usually cheap by itself
- cost depends on what the selected branch calls next

Reference focus:

- OpenXcom: AI mode and battle action dispatch.
- Warzone: order/action update dispatch.
- DevilutionX: `AiProc` behavior dispatch.
- KeeperFX: creature state function table.
- re3: objective/state/move-state dispatch.

Watch for:

- dispatch branch that hides pathfinding, target scans, or map mutation
- huge switch that mutates several systems directly

### 2. Capability And Policy Checks

Cost shape:

```text
O(1) per check
```

What it does:

- flags/tags decide whether an actor can fly, swim, attack, repair, flee, use
  magic, take a job, enter a door, or target something
- usually prevents expensive work

Reference focus:

- NetHack: `M1_`, `M2_`, `M3_` monster flags.
- Warzone: secondary orders and droid type/component checks.
- KeeperFX: creature/job flags and room eligibility.
- re3: ped type threat/avoid masks.

Watch for:

- expensive checks happening before cheap reject flags

### 3. Per-Actor Tick

Cost shape:

```text
O(actor)
```

What it does:

- runs one update pass per active actor
- updates intent, action, movement, timers, status, animation, or reports

Reference focus:

- Warzone: `orderUpdateDroid -> actionUpdateDroid -> moveUpdateDroid`.
- DevilutionX: active monster processing.
- KeeperFX: creature state processing.
- NetHack: monster turn iteration.
- re3: active ped/vehicle updates.

Watch for:

- every actor performing expensive map/target/path work every tick
- no sleeping/offscreen/throttle path

### 4. Target Search

Cost shape:

```text
O(actor * target)
```

What it does:

- finds enemies, repair targets, work targets, visible actors, threats, or
  interactables
- can explode when every actor scans every possible target

Reference focus:

- Warzone: combat micro, repair target selection, droid/structure scans.
- re3: threat and ambient ped reactions.
- DevilutionX: target memory and visibility updates.
- NetHack: adjacent/nearby monster and hero checks.

Cost reducers:

- spatial grids
- faction/type filters
- visibility filters
- target memory
- sector/chunk buckets
- dormant/offscreen actors

### 5. Movement Candidate Generation

Cost shape:

```text
O(local tiles)
```

What it does:

- checks adjacent or nearby positions
- records reasons: blocked, occupied, door, trap, water, attack, open, dig

Reference focus:

- NetHack: `mfndpos` and local monster movement.
- OpenXcom: reachable/candidate tiles around unit decisions.
- KeeperFX: local movement/collision checks.

Why it matters:

- cheaper than full pathfinding
- good first filter before expensive search

Watch for:

- candidate generation directly mutating position before selection is final

### 6. Pathfinding

Cost shape:

```text
O(path area)
```

What it does:

- searches grid/graph for a route
- writes path buffer, route state, or movement plan

Reference focus:

- Warzone: unit path/move state across many droids.
- DevilutionX: `FindPath` and `AiPlanPath`.
- re3: `CPathFind` graph and ped/vehicle pathing.
- OpenXcom: path execution for walk actions.
- KeeperFX: Ariadne/navigation.

Cost reducers:

- path-follow buffers
- path requested only when destination changes
- direct-line check before path search
- local candidate checks first
- gateway/path graph abstraction
- activity timers

Watch for:

- pathfinding inside every actor tick
- pathfinding repeated after small non-semantic changes

### 7. Line Of Sight And Visibility

Cost shape:

```text
O(line length) per ray
O(actor * target * line length) if unfiltered
O(map) for full visibility rebuilds
```

What it does:

- checks whether actors can see/shoot each other
- marks visible tiles
- updates light/vision state

Reference focus:

- DevilutionX: line clear for movement and missiles.
- Warzone: visibility phase and sensor/object visibility.
- OpenXcom: visible units/tiles.
- NetHack: vision recalculation after movement/light changes.
- re3: threat/path/sector visibility style checks.

Cost reducers:

- map-owned visible tile sets
- spatial/sector prefilter
- target memory
- visibility phase instead of ad hoc checks everywhere
- dirty flags for moved lights/actors

Watch for:

- every actor raycasting every target every frame
- LoS repeated for both decision and execution without cache

### 8. Occupancy And Collision Lookup

Cost shape:

```text
O(1) for grid lookup
O(local tiles) for neighborhood collision
```

What it does:

- answers whether a tile has actor/object/item/door/rubble/fire
- prevents movement overlap
- feeds pathing, targeting, pickup, and interaction

Reference focus:

- DevilutionX: dungeon arrays for monsters, players, objects, items, corpses.
- NetHack: level monster grid/list.
- Warzone: map grid and movement blocking.
- KeeperFX: slab/room/navigation/collision state.
- OpenXcom: tile and unit/item ownership.

Why it matters:

- this is where games buy cheap AI queries with memory
- occupancy arrays are a classic "spend memory to save CPU" pattern

Watch for:

- actor-owned occupancy truth
- position mutation that forgets to update occupancy

### 9. Spatial Grid / Sector Rebuild

Cost shape:

```text
O(actor) to rebuild object buckets
O(map) if rebuilding full tile/grid state
```

What it does:

- updates broadphase lookup structures
- makes nearby actor/object queries cheaper later

Reference focus:

- Warzone: spatial grid rebuild from live objects.
- re3: path/world sector style ownership.
- DevilutionX: level arrays loaded/resynced.

Cost tradeoff:

- rebuild once, query many times
- worth it when many actors ask similar questions

Watch for:

- rebuilding the whole cache after tiny local changes
- hidden rebuilds inside actor decisions

### 10. Interaction / Work-Site Search

Cost shape:

```text
O(work sites) or O(region work sites)
```

What it does:

- finds room jobs, attractors, build sites, repair sites, pickups, doors, shops,
  patrol points, or capture points

Reference focus:

- KeeperFX: rooms/jobs/work capacity.
- re3: attractors and queues.
- Warzone: build/repair/rearm/guard orders.
- NetHack: shops, doors, pets, item use.
- OpenXcom: route nodes and battle objectives.

Cost reducers:

- map-owned semantic points
- region/faction/site indexes
- site capacity records
- queues/reservations
- tags and profile filters

Watch for:

- each actor scanning all sites every tick
- no ownership for reservation/capacity

### 11. Queue Drain

Cost shape:

```text
O(queue entries drained)
```

What it does:

- processes pending orders, effects, audio, events, deaths, removals, path
  directions, or state stack entries

Reference focus:

- Warzone: droid order list and pending orders.
- OpenXcom: path direction dequeue and battle state stack.
- NetHack: end-of-turn dead monster cleanup.
- re3: attractor queues.

Cost reducers:

- bounded queue size
- batch drain phase
- stale target validation before execution
- clear owner for push/pop/drain

Watch for:

- queues with dead pointers
- multiple drain sites with different rules
- unbounded growth

### 12. State Transition Cleanup

Cost shape:

```text
O(side effects owned by old state)
```

What it does:

- clears room membership, combat links, drag targets, spell effects, summons,
  queues, reservations, path state, or target state when behavior changes

Reference focus:

- KeeperFX: explicit creature state cleanup.
- Warzone: order/action changes and target cleanup.
- NetHack: delayed monster removal and list cleanup.
- re3: previous objective restore and attractor assignment.

Why it matters:

- cleanup is compute work, but also correctness work
- missing cleanup creates stale links and future compute waste

Watch for:

- state writes without cleanup
- target death not removing queued references

### 13. Runtime Cache Invalidation

Cost shape:

```text
O(1) mark dirty
O(region) or O(map) rebuild later
```

What it does:

- marks navigation, visibility, occupancy, LoS, render, or interaction indexes
  dirty after mutation

Reference focus:

- Warzone: visibility/grid phases.
- DevilutionX: level arrays and resync after load.
- NetHack: vision recalculation after movement/light source changes.
- KeeperFX: post-load level reinitialization.

Cost reducers:

- dirty regions
- phase-based rebuilds
- separating source truth from derived cache

Watch for:

- rebuilding immediately after every tiny mutation
- forgetting to invalidate after map mutation

### 14. Save/Load And Post-Load Repair

Cost shape:

```text
O(save size + actor count + item count + repair links + map rebuilds)
```

What it does:

- writes authoritative state
- reloads data
- repairs pointers/ids/lists
- rebuilds caches, runtime resources, visibility, occupancy, animation pointers

Reference focus:

- NetHack: monster chain restore and pointer repair.
- DevilutionX: monster save/load and runtime resync.
- KeeperFX: chunk save/load and post-load repair.
- re3: block save/load and dynamic path flags.
- OpenXcom: battle/session save structures.
- Warzone: order/action/movement persistence.

Watch for:

- saving rebuildable caches as truth
- no post-load repair owner
- pointer fixup scattered across gameplay logic

### 15. Allocation And Container Growth

Cost shape:

```text
O(allocation) plus possible copy/realloc cost
```

What it does:

- creates vectors, pushes dynamic entries, resizes order lists, allocates save or
  candidate structures

Reference focus:

- Warzone: order list resize and vector/list use.
- OpenXcom: saved battle/unit/item vectors.
- NetHack: old-style allocations/lists.
- re3: attractor vectors.

Watch for:

- allocation in hot per-actor tick
- vector resize while draining input/orders
- temporary allocations for every AI decision

## Per-Game Compute Attention Map

| Cost area | OpenXcom | Warzone | NetHack | DevilutionX | KeeperFX | re3 Miami |
|---|---:|---:|---:|---:|---:|---:|
| State dispatch | Medium | High | High | High | High | High |
| Capability flags | Medium | High | High | High | High | High |
| Per-actor tick | Medium | High | Medium | High | High | High |
| Target search | Medium | High | Medium | High | Medium | High |
| Movement candidates | High | Medium | High | Medium | Medium | Medium |
| Pathfinding | High | High | Low/Medium | High | High | High |
| LoS/visibility | High | High | Medium | High | Medium | Medium/High |
| Occupancy/collision | High | High | High | High | High | High |
| Spatial/cache rebuild | Low/Medium | High | Medium | High | High | High |
| Work-site search | Low | Medium | Medium | Low | High | High |
| Queue drain | Medium | High | Medium | Medium | High | High |
| State cleanup | Medium | High | High | High | High | High |
| Save/load repair | Medium | High | High | High | High | High |
| Allocation pressure | Medium | Medium/High | Medium | Medium | Medium | Medium |

## What Each Game Appears To Care About

### OpenXcom

Compute attention:

- tactical pathing
- visible units/tiles
- battle action execution
- turn-based AI decisions
- battle save/session state

Why:

- fewer actors, so AI can spend more per turn
- map/tile/path correctness matters more than frame-scale throughput

### Warzone 2100

Compute attention:

- many droid updates
- durable orders and queued orders
- action/move phases
- spatial grid and visibility
- path/move control over many units

Why:

- RTS scale means repeated per-unit work dominates
- shared map/query structures reduce many-unit cost

### NetHack

Compute attention:

- monster turn iteration
- local movement candidates
- dense special behavior
- level occupancy/list correctness
- save/restore repair

Why:

- turn-based grid lets local checks carry much of the AI
- special cases are more important than frame throughput

### DevilutionX

Compute attention:

- active monster tick
- LOS and missile line checks
- path planning
- dungeon occupancy arrays
- mode/animation coupling
- save/load resync

Why:

- ARPG monsters need fast tile queries and combat legality checks
- active/offscreen distinction helps limit full work

### KeeperFX

Compute attention:

- creature state dispatch
- room/job/work-site validation
- navigation and collision
- room/dungeon ownership
- director task planning
- state cleanup

Why:

- dungeon simulation spends compute on work assignment and world economy, not
  only combat
- room capacity and job ownership are core costs

### re3 Miami

Compute attention:

- active ped/vehicle updates
- objectives/states/move modes
- path graph and path-follow buffers
- attractors and queues
- threat reactions
- vehicle autopilot

Why:

- open-world simulation spends compute on many ambient actors
- world-owned attractors/path graph reduce repeated search

## Commands To Build A Cost Ledger

Find update phases:

```bash
rg -n "Process|Update|Tick|think\\(|movemon\\(|moveUpdate|actionUpdate|orderUpdate" /path/to/repo/src
```

Find path/LoS/visibility:

```bash
rg -n "FindPath|PathFind|LineClear|LOS|visible|visibility|vision|sight|CanMove|blocked|solid" /path/to/repo/src
```

Find queues/lists:

```bash
rg -n "queue|Queue|pending|listSize|push_back|pop|erase|drain|next order|order list" /path/to/repo/src
```

Find mutation:

```bash
rg -n "set[A-Z]|remove_|place_|update|clear|reset|openDoor|SetObjective|SetFollowPath|moveUpdate.*Pos" /path/to/repo/src
```

Find cache rebuilds:

```bash
rg -n "rebuild|Rebuild|dirty|invalidate|Init.*Level|Process.*Visibility|update.*grid|occup" /path/to/repo/src
```

Find allocations:

```bash
rg -n "new |malloc|calloc|realloc|alloc\\(|push_back|resize|reserve|std::vector" /path/to/repo/src
```

## Cost Ledger Template

Use this after reading any subsystem:

```text
Subsystem:
Runs:
Main loop owner:
Actor count touched:
Map area touched:
Queues drained:
Path requests:
LoS/visibility requests:
Target scans:
Cache reads:
Cache rebuilds:
Allocations:
Authoritative mutations:
Derived mutations:
Cleanup required:
Saved:
Rebuilt:
Likely hot spots:
Cost reducers:
```

## Main Learning Point

Games reveal their priorities through what they make cheap:

- RTS makes group/unit orders, movement, visibility, and spatial lookup cheap.
- Roguelikes make local grid decisions and special-case turn logic tolerable.
- ARPGs make occupancy, LOS, and active monster updates cheap.
- Dungeon sims make rooms, jobs, navigation, and state cleanup central.
- Open-world games make many ambient actors, attractors, sectors, and path
  graphs manageable.

The compute focus is the design focus.

