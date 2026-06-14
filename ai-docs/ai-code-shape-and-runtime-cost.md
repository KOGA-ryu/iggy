# AI Code Shape and Runtime Cost

This document explains how to read AI code for compute cost. It is not a
benchmark report. It is a code-reading cost model: what the code shape implies
about CPU work, memory access, rebuild cost, and scaling pressure.

## Core Cost Question

For each AI system, ask:

```text
How often does this run?
How many actors does it touch?
How much map does it scan?
Does it allocate?
Does it pathfind?
Does it do LOS/visibility?
Does it rebuild a shared cache?
Does it save/load or repair pointers?
```

The expensive part of AI is usually not the enum switch. It is repeated world
queries: pathfinding, line of sight, visibility, collision, occupancy, target
search, and cache rebuilds.

## Cheap Code Shapes

### Enum Switches

Common in all references.

Typical cost:

```text
O(1) per actor decision branch
```

A switch over objective, mode, action, or state is usually cheap. The cost comes
from what each branch calls.

Examples:

- OpenXcom AI mode branching.
  - `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:174`
- Warzone order/action dispatch.
  - `/Users/kogaryu/iggy/warzone2100-master/src/order.cpp:4235`
- re3 objective processing.
  - `/Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp:777`

### Function Pointer Tables

Common in DevilutionX and KeeperFX.

Typical cost:

```text
O(1) dispatch + branch function cost
```

The indirect call itself is not the main cost. The behavior function may be
expensive if it performs pathfinding, target scans, or map queries.

Examples:

- DevilutionX `AiProc`.
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:3091`
- KeeperFX `process_func_list`.
  - `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:309`

### Bitflags

Common in all references.

Typical cost:

```text
O(1) capability/policy checks
```

Flags are cheap and compact. They let code reject impossible behavior before
calling expensive systems.

Examples:

- NetHack monster capability flags.
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monflag.h:10`
- Warzone secondary order policy.
  - `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:88`

## Expensive Code Shapes

### Per-Actor Full Scans

Risk:

```text
O(number_of_actors * number_of_targets)
```

If every actor scans every other actor each tick, cost grows quickly. Mature
engines reduce this with visibility, spatial grids, factions, sleeping actors,
or target memory.

Search recipe:

```bash
rg -n "for .*monster|for .*droid|for .*ped|for .*creature|for .*unit" /path/to/repo/src
rg -n "visible|target|enemy|threat|nearby|grid|bucket|sector" /path/to/repo/src
```

Examples:

- Warzone rebuilds/query spatial grid and object visibility.
  - `/Users/kogaryu/iggy/warzone2100-master/src/mapgrid.cpp:36`
  - `/Users/kogaryu/iggy/warzone2100-master/src/visibility.h:37`
- re3 uses world/path/sector systems around peds and vehicles.
  - `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.h:203`

### Pathfinding

Risk:

```text
O(search_area) per path request
```

Pathfinding is one of the most expensive AI helpers. The references avoid
calling full path search for every actor every tick when possible. They use
direct-line checks, cached movement state, path-follow buffers, activity timers,
or limited candidate generation.

Search recipe:

```bash
rg -n "FindPath|PathFind|pathfind|route|node|gateway|mfndpos|SetFollowPath|AiPlanPath" /path/to/repo/src
```

Examples:

- DevilutionX checks line/path conditions before or around path planning.
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1861`
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/engine/path.cpp:184`
- re3 stores short path-follow buffers on peds after querying `CPathFind`.
  - `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:6208`
  - `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.h:203`
- OpenXcom calculates paths when executing walk actions.
  - `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.cpp:255`

### Line Of Sight and Visibility

Risk:

```text
O(line_length) per LOS check
O(map_or_actor_count) for visibility rebuilds
```

LOS is cheaper than pathfinding but can still dominate if every actor checks
many targets every tick.

Search recipe:

```bash
rg -n "LineClear|LOS|visible|visibility|sight|sees|canSee|light|vision" /path/to/repo/src
```

Examples:

- DevilutionX uses line-clear helpers for movement and missile decisions.
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1596`
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4384`
- Warzone updates visibility as a runtime phase.
  - `/Users/kogaryu/iggy/warzone2100-master/src/loop.cpp:545`
  - `/Users/kogaryu/iggy/warzone2100-master/src/visibility.h:37`
- OpenXcom stores visible units/tiles on battle units.
  - `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/BattleUnit.h:83`

### Cache Rebuilds

Risk:

```text
O(map_size) or O(actor_count) per rebuild
```

Shared caches are good when many actors query them. They are costly when rebuilt
too often.

Search recipe:

```bash
rg -n "rebuild|update.*grid|Init.*Level|Process.*Visibility|occup|grid|cache|clear.*list|reset.*map" /path/to/repo/src
```

Examples:

- Warzone rebuilds a spatial grid from live objects in the main loop.
  - `/Users/kogaryu/iggy/warzone2100-master/src/loop.cpp:548`
  - `/Users/kogaryu/iggy/warzone2100-master/src/mapgrid.cpp:36`
- DevilutionX uses dungeon arrays for player, monster, item, object, light, and
  corpse occupancy/query data.
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/levels/gendung.h:149`
- NetHack keeps map occupancy and monster lists in level state.
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/rm.h:476`

### Save/Load Repair

Risk:

```text
O(saved_actors + saved_items + map_size + pointer_fixups)
```

Save/load cost usually does not matter every frame, but it reveals hidden
ownership complexity. Pointer fixup and post-load repair show where runtime
state is derived from saved truth.

Search recipe:

```bash
rg -n "Save|Load|Restore|Fix|Repair|Rebuild|PostLoad|Init.*Level|chunk" /path/to/repo/src
```

Examples:

- NetHack saves/restores monster chains and repairs links.
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/save.c:836`
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/restore.c:1198`
- DevilutionX loads monsters and resyncs runtime arrays/resources.
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:663`
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:1502`
- KeeperFX uses chunk save/load plus post-load level reinitialization.
  - `/Users/kogaryu/iggy/keeperfx-master/src/game_saves.c:87`

## Per-Game Cost Shape

### OpenXcom

Cost profile:

- turn-based, so AI can spend more per decision
- candidate action generation may query pathfinding and visibility
- path calculation happens when a walk action is executed
- fewer actors per tick than RTS/open-world games

Main scaling pressure:

```text
units taking turns * candidate/path/visibility checks
```

### Warzone 2100

Cost profile:

- many live units
- per-droid order/action/move update
- shared visibility and spatial grid phases
- group/formations add coordination cost
- path/move state must be amortized across many droids

Main scaling pressure:

```text
droid_count * update_cost + grid/visibility rebuild cost + path requests
```

### NetHack

Cost profile:

- turn-based, grid-local, mostly adjacent movement
- monster candidate generation is local but dense with special cases
- global monster list iteration per turn
- save/restore pointer repair is complex but not per-frame

Main scaling pressure:

```text
monster_count * local_candidate/special_behavior_cost
```

### DevilutionX

Cost profile:

- active monsters processed each game tick
- offscreen/sleep/activity timers reduce work
- pathfinding and LOS are central costs
- dungeon arrays make occupancy queries cheap
- animation and mode updates are coupled to AI

Main scaling pressure:

```text
active_monster_count * (LOS + path planning + behavior dispatch)
```

### KeeperFX

Cost profile:

- many creatures and world objects
- state functions are cheap dispatch, but jobs/rooms/navigation can be costly
- work-site queries require room capacity and ownership checks
- director AI tasks add global planning cost
- navigation and collision are recurring runtime costs

Main scaling pressure:

```text
creature_count * state/job/navigation cost + room/dungeon/director scans
```

### re3 Miami

Cost profile:

- many ambient peds and vehicles
- objectives/states are cheap, but path graph, attractors, threat scanning, and
  vehicle autopilot are expensive
- world systems reduce cost through sectors, path nodes, attractor managers, and
  short path-follow buffers

Main scaling pressure:

```text
active_ped_count * objective/threat/path cost + vehicle_autopilot cost
```

## Reading Code For Cost

When you inspect a function, mark each line mentally as one of:

```text
branch: cheap
field read/write: cheap
small local candidate loop: usually cheap
actor-list scan: can scale badly
map scan: can scale badly
path request: expensive
LOS request: medium but frequent
allocation: suspicious inside tick
cache rebuild: fine if scheduled, risky if hidden
save/load repair: not frame-critical, but ownership-heavy
```

## Cost Smells

These are places to slow down:

- pathfinding inside every actor tick with no throttle
- LOS checks against many targets per actor
- nested actor loops
- cache rebuilds hidden inside individual actor decisions
- allocation inside hot update functions
- save/load writing derived caches as if they were truth
- state transition code that scans rooms, actors, and map all at once
- behavior functions that both search the world and mutate many systems

## Cost-Friendly Patterns In The References

Repeated patterns that reduce runtime pressure:

- static flags reject impossible actions before expensive queries
- live actor stores current objective/action/path state instead of recomputing
  everything every tick
- map/session owns shared grids and occupancy arrays
- path-follow state stores the result of a path query
- activity/sleep/offscreen logic avoids updating every actor fully
- local candidate generation is used before full pathfinding
- save/load repairs runtime caches instead of treating all caches as durable

## Commands For Cost-Oriented Research

Find hot loops:

```bash
rg -n "for \\(|while \\(|FOREACH|for_each" /path/to/repo/src
```

Find path/LOS cost:

```bash
rg -n "FindPath|PathFind|LineClear|LOS|visible|visibility|CanMove|blocked|solid" /path/to/repo/src
```

Find allocation in update code:

```bash
rg -n "new |malloc|calloc|realloc|std::vector<|push_back|resize|reserve" /path/to/repo/src
```

Find cache rebuilds:

```bash
rg -n "rebuild|Rebuild|clear\\(|reset|Init.*Level|update.*grid|Process.*Visibility" /path/to/repo/src
```

Find throttling/sleep/activity:

```bash
rg -n "sleep|inactive|active|activity|timer|cooldown|delay|throttle|skip" /path/to/repo/src
```

Find save/load repair:

```bash
rg -n "Save|Load|Restore|Fix|Repair|Rebuild|PostLoad|chunk" /path/to/repo/src
```

## Minimal Cost Summary Template

Use this after reading a system:

```text
Runs:
Actor count touched:
Map area touched:
Path requests:
LOS/visibility requests:
Allocations:
Cache rebuilds:
Saved state:
Rebuilt state:
Likely hot spots:
Cost controls:
```

