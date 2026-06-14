# KeeperFX AI Study

Reference repo: `/Users/kogaryu/iggy/keeperfx-master`

KeeperFX is useful because it is not just enemy AI. It models a dungeon as a
living work system: creatures have needs, jobs, rooms, moods, combat states,
navigation scratch, and a higher-level computer keeper that creates tasks. The
main lesson for Iggy is how to separate actor state from work-site ownership and
director-level intent.

## Core Ownership Map

### Static Creature and Job Data

Useful files:

- `/Users/kogaryu/iggy/keeperfx-master/config/fxdata/creature.cfg`
- `/Users/kogaryu/iggy/keeperfx-master/config/creatrs/*.cfg`
- `/Users/kogaryu/iggy/keeperfx-master/config/fxdata/crstates.cfg`
- `/Users/kogaryu/iggy/keeperfx-master/src/config_creature.h:74`
- `/Users/kogaryu/iggy/keeperfx-master/src/config_creature.h:236`
- `/Users/kogaryu/iggy/keeperfx-master/src/config_creature.h:255`
- `/Users/kogaryu/iggy/keeperfx-master/src/config_creature.h:401`
- `/Users/kogaryu/iggy/keeperfx-master/src/config_creature.c:311`

Creature model config owns durable profile facts: model flags, supported jobs,
start states, health, strength, defense, speed, hunger/sleep/lair settings,
vision/hearing, combat preferences, training/research/manufacture values,
annoyance, anger jobs, and learned instances.

Job config owns the bridge from a job id to room role, event kind, initial state,
continue state, and assignment/check callbacks. Jobs are not just labels; they
encode "can this creature do this work here?" and "which state should it enter?"

Iggy lesson: keep `NpcDefinition2D` and `NpcJobProfile2D` as static data. Do not
put room capacity, current work target, or path scratch in the definition.

### Live Entity Data

Useful files:

- `/Users/kogaryu/iggy/keeperfx-master/src/thing_data.h:120`
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_data.h:124`
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_data.h:125`
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_data.h:128`
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_data.h:206`
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_data.h:258`
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_data.h:303`

`Thing` is the broad live entity. It owns common runtime identity, owner,
current/continue state, map position, model id, class payload, render/minimap
scratch, holding links, and interpolation-ish fields.

Creature-specific entity data in `Thing` is small compared with the actual brain
state. The heavy creature memory is moved into `CreatureControl`.

Iggy lesson: keep `NpcActorState2D` small: id, definition id, owner/faction,
position, facing, visible execution state, and lightweight status. Put behavior
memory in a separate control/state component.

### Live Creature Brain

Useful files:

- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:72`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:97`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:105`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:142`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:156`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:168`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:180`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:199`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:245`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:350`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:371`

`CreatureControl` owns live AI memory: combat flags, opponent lists, experience,
movement target, hunger, paydays, annoyance, mood, lair/work/target room ids,
turn counters, pickup/drag targets, group links, job-specific scratch, patrol
memory, backups for temporary states, navigation route state, assigned job, and
throttled check turns.

The important boundary is that this is not static creature data and not room
truth. It is per-creature runtime control state.

Iggy lesson: add a separate `NpcBehaviorState2D` only when needed. It should own
current objective, assigned job, mood/need counters, target memory, and path
follow scratch. It should not own map work-site truth or asset resources.

### Room and Dungeon Ownership

Useful files:

- `/Users/kogaryu/iggy/keeperfx-master/src/room_data.h:50`
- `/Users/kogaryu/iggy/keeperfx-master/src/room_data.h:79`
- `/Users/kogaryu/iggy/keeperfx-master/src/room_data.h:150`
- `/Users/kogaryu/iggy/keeperfx-master/src/room_data.h:171`
- `/Users/kogaryu/iggy/keeperfx-master/src/room_data.h:217`
- `/Users/kogaryu/iggy/keeperfx-master/src/room_data.h:239`
- `/Users/kogaryu/iggy/keeperfx-master/src/room_jobs.h:42`
- `/Users/kogaryu/iggy/keeperfx-master/src/room_jobs.h:51`
- `/Users/kogaryu/iggy/keeperfx-master/src/dungeon_data.h:143`
- `/Users/kogaryu/iggy/keeperfx-master/src/dungeon_data.h:154`
- `/Users/kogaryu/iggy/keeperfx-master/src/dungeon_data.h:179`
- `/Users/kogaryu/iggy/keeperfx-master/src/dungeon_data.h:205`
- `/Users/kogaryu/iggy/keeperfx-master/src/dungeon_data.h:297`

Rooms own room kind, owner, center, capacity, slab list, entrance/list links,
stored contents, creatures working in the room, efficiency, and room-specific
storage. Room helpers find room positions, capacity, and rooms by role.

Dungeon owns player-level economy/session indexes: creature lists, creature
state/job counters, call-to-arms state, room/door counts, research/manufacture
progress, digger stack, room list starts, script flags, and player tendencies.

Iggy lesson: work sites belong to the level/world layer, not the NPC. An NPC can
hold an assignment id, but `LevelWorkSiteIndex2D` or `InteractionWorkSiteStore2D`
should own slots, capacity, availability, and map position.

## Creature State Machine

Useful files:

- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.h:36`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.h:204`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.h:224`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.h:258`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.h:278`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.h:295`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:309`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:449`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:494`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:505`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:4839`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:4873`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:4924`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:4982`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:5045`
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_creature.c:2579`

KeeperFX has many concrete creature states, but also a useful category layer:
idle, work, own needs, sleep, feed, creature fight, move, salary, escape,
unconscious, anger job, door fight, object fight, call-to-arms, follow, and deep
work.

The update flow is table-driven:

- creature has `active_state` and `continue_state`
- active state resolves to configured process/move/cleanup functions
- state transitions clear temporary instance data
- external transitions are checked against current state category and override
  rules
- cleanup removes room membership, combat traces, dragged objects, spell effects,
  summons, and other side links

Iggy lesson: build a small `NpcBehaviorStateType2D` grouping before adding many
states. The categories matter more than the exact KeeperFX state list.

## Jobs and Room Assignment

Useful files:

- `/Users/kogaryu/iggy/keeperfx-master/src/creature_jobs.h:32`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_jobs.h:47`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_jobs.c:97`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_jobs.c:111`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_jobs.c:130`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_jobs.c:157`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_jobs.c:177`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_jobs.c:197`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_jobs.c:691`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_jobs.c:750`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_jobs.c:869`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_jobs.c:924`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:2971`

Job assignment is not a direct mutation. It is a validation pipeline:

- check whether the creature profile supports the job
- reject if actor state/spell/ownership disallows it
- check room role or coordinate-specific availability
- check capacity
- dispatch a job-specific assignment function
- set assigned job only for durable jobs
- move creature into an initial/continue state

This is the strongest KeeperFX lesson for Iggy. A job is a typed interaction
contract between actor profile, world site, validation rules, and behavior state.

Iggy mapping:

- `NpcJobDefinition2D`: static job id, required tags, target kind, state entered
- `NpcJobEligibilityQuery2D`: actor/profile/world-site validation
- `NpcJobAssignment2D`: accepted target, site id, start tile, mode
- `NpcJobRunner2D`: turns assignment into movement/action proposals
- `InteractionWorkSiteStore2D`: owns capacity and site availability

## Navigation and Movement

Useful files:

- `/Users/kogaryu/iggy/keeperfx-master/src/ariadne.h:80`
- `/Users/kogaryu/iggy/keeperfx-master/src/ariadne.h:96`
- `/Users/kogaryu/iggy/keeperfx-master/src/ariadne.h:103`
- `/Users/kogaryu/iggy/keeperfx-master/src/ariadne.h:146`
- `/Users/kogaryu/iggy/keeperfx-master/src/ariadne.h:182`
- `/Users/kogaryu/iggy/keeperfx-master/src/ariadne.h:216`
- `/Users/kogaryu/iggy/keeperfx-master/src/ariadne.h:254`
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_navigate.c`
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_navigate.h`
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_creature.c:2690`
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_creature.c:2763`
- `/Users/kogaryu/iggy/keeperfx-master/src/thing_creature.c:2871`

Navigation uses map-derived legality plus route scratch. Ariadne/Navigation
hold route/path fields, current waypoint, wall-hug/reroute flags, size rules,
and movement state. Movement applies velocity, wall collision, sliding, door
collision, creature collision avoidance, map exploration, and state-specific
move-from-slab hooks.

Iggy lesson: pathfinding and movement legality are level/query systems. The NPC
control state can hold current route/follower scratch, but collision truth and
path graph ownership should remain with the level/runtime world.

## Computer Keeper AI

Useful files:

- `/Users/kogaryu/iggy/keeperfx-master/src/player_computer.h:57`
- `/Users/kogaryu/iggy/keeperfx-master/src/player_computer.h:182`
- `/Users/kogaryu/iggy/keeperfx-master/src/player_computer.h:262`
- `/Users/kogaryu/iggy/keeperfx-master/src/player_computer.h:307`
- `/Users/kogaryu/iggy/keeperfx-master/src/player_computer.h:418`
- `/Users/kogaryu/iggy/keeperfx-master/src/player_computer.h:494`
- `/Users/kogaryu/iggy/keeperfx-master/src/player_computer.h:522`
- `/Users/kogaryu/iggy/keeperfx-master/src/player_computer.h:544`
- `/Users/kogaryu/iggy/keeperfx-master/src/player_comptask.c`
- `/Users/kogaryu/iggy/keeperfx-master/src/player_computer.c`

The computer player is a higher-level director. It owns tasks such as dig room,
place room, dig to gold, dig to attack, call to arms, pickup for attack, move
creatures to room/position, defend, slap diggers, sell traps/doors, and move
gold. It has task states for waiting, selecting, and performing.

This layer points at the dungeon and creates game actions. It does not replace
per-creature behavior. It assigns or manipulates creatures through task/action
interfaces.

Iggy lesson: use a future `FactionDirector2D` or `AiDirector2D` for map-owned
plans: spawn pressure, work orders, patrol goals, defense calls, and scripted
events. Do not make every NPC independently rediscover global goals.

## Save/Load Ownership

Useful files:

- `/Users/kogaryu/iggy/keeperfx-master/src/game_saves.h:34`
- `/Users/kogaryu/iggy/keeperfx-master/src/game_saves.h:102`
- `/Users/kogaryu/iggy/keeperfx-master/src/game_saves.c:87`
- `/Users/kogaryu/iggy/keeperfx-master/src/game_saves.c:181`
- `/Users/kogaryu/iggy/keeperfx-master/src/game_saves.c:309`
- `/Users/kogaryu/iggy/keeperfx-master/src/game_saves.c:353`
- `/Users/kogaryu/iggy/keeperfx-master/src/game_saves.c:411`
- `/Users/kogaryu/iggy/keeperfx-master/src/game_saves.c:416`

KeeperFX saves chunks for catalogue info, the large game struct, intralevel
data, and Lua serialized data. Before saving it exports light system state. On
load it reads chunks, switches/reloads campaign resources and stats, restores
game/intralevel/Lua state, updates tabs, reapplies creature sounds, and runs
post-load level reinitialization.

This is a legacy whole-world snapshot style with repair hooks. It is useful as
a warning, not as an Iggy format model.

Iggy lesson: copy the chunk boundary and explicit post-load repair concept, not
the raw world-struct dump. Persist authoritative session/world/actor/job state;
rebuild path graphs, render caches, collision caches, visible queries, and
debug panel models.

## Iggy Translation

Good candidate types:

- `NpcDefinition2D`: static creature profile and supported capability tags.
- `NpcJobDefinition2D`: static job contract, required world tags, state entered.
- `NpcActorState2D`: live identity, transform, health/status, definition id.
- `NpcBehaviorState2D`: live objective, assigned job, mood/need counters, target
  memory, route follower scratch.
- `InteractionWorkSiteStore2D`: level-owned work sites, capacity, availability,
  target tile, site tags.
- `NpcJobEligibilityQuery2D`: pure validator over actor/profile/site/world.
- `NpcJobAssignmentRunner2D`: converts accepted assignments into movement/action
  proposals.
- `FactionDirector2D`: map/faction-owned task planner for global work and
  pressure.
- `LevelNavigationCache2D`: derived route graph/query state.
- `RuntimePostLoadRepair2D`: rebuilds derived caches after loading snapshots.

Ownership shape:

- `scene/npc`: definitions, actor state, behavior state structs.
- `scene/ai`: job definitions, eligibility queries, objective runners, scoring.
- `scene/level`: work-site store, tile/collision/navigation query ownership.
- `runtime`: current session snapshot, accepted commands, post-load repair.
- `ui`: inspectors only; no AI truth.

## What To Copy Conceptually

- Static creature profiles separate from live actor state.
- Live behavior/control state separate from actor identity.
- Job assignment as validate -> assign -> state transition.
- Rooms/work sites owning capacity and availability.
- State categories that guard transitions.
- Director-level AI for map/faction goals.
- Explicit post-load repair of derived systems.

## Do Not Copy

- Giant global structs as save format.
- Monolithic `Thing` style payload union.
- Function-pointer tables as the primary extension mechanism for Iggy.
- Hundreds of concrete creature states before the category model is proven.
- Hidden side effects in state changes without typed reports.
- Computer player logic directly issuing low-level mutations everywhere.

## Minimal Iggy Lesson

Build work-site ownership before building complex NPC personalities. A small
engine version should let the map say "this tile/site has work with capacity",
let NPC profiles say "I can do that job", let a validator produce an assignment,
and let the existing movement/command pipeline execute it. That gives Iggy the
KeeperFX-style dungeon intelligence without importing KeeperFX's global state
shape.
