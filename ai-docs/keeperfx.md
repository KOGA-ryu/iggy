# KeeperFX AI Study

Reference repo: `/Users/kogaryu/iggy/keeperfx-master`

KeeperFX is useful because it is not just enemy AI. It models a dungeon as a
living work system: creatures have needs, jobs, rooms, moods, combat states,
navigation scratch, and a higher-level computer keeper that creates tasks. The
main lesson is how it separates actor state from work-site ownership and
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

Learning point: creature and job definitions are static data. Room capacity,
current work targets, and path scratch live elsewhere.

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

Learning point: KeeperFX keeps common live entity data in `Thing` and heavier
creature brain state in `CreatureControl`.

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

Learning point: per-creature control state owns objective/job/mood/memory/path
scratch, while room truth and static profile data live elsewhere.

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

Learning point: work sites belong to room/dungeon data. Creatures can hold
assignments and target room ids, but rooms own slots, capacity, availability,
and map position.

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

Learning point: the category layer matters as much as the long concrete state
list. State categories control transitions and cleanup expectations.

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

This is the strongest KeeperFX pattern. A job is a typed interaction contract
between actor profile, world site, validation rules, and behavior state.

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

Learning point: pathfinding and movement legality are level/query systems.
Creature control state holds current route/follower scratch, while collision
truth and path graph ownership stay in the world/navigation layer.

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

Learning point: director AI is separate from individual creature behavior. The
computer keeper owns map/faction tasks and manipulates creatures through task
and action interfaces.

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
a warning about raw world-struct dumps and post-load repair complexity.

## Reference Patterns

- Static creature profiles are separate from live actor state.
- Live behavior/control state is separate from common actor identity.
- Job assignment follows validate -> assign -> state transition.
- Rooms/work sites own capacity and availability.
- State categories guard transitions.
- Director-level AI handles map/faction goals.
- Save/load has explicit post-load repair of derived systems.

## Legacy Costs

- Giant global structs as save format.
- Monolithic `Thing` style payload union.
- Function-pointer tables as the primary extension mechanism.
- Hundreds of concrete creature states before the category model is proven.
- Hidden side effects in state changes without typed reports.
- Computer player logic directly issuing low-level mutations everywhere.

## Minimal Reference Lesson

KeeperFX's work AI skeleton is:

1. Static creature profiles say what jobs a creature can do.
2. Rooms own work sites, capacity, ownership, and availability.
3. Creature control state stores current need, job, room target, mood, route,
   and transition scratch.
4. Job assignment validates creature/profile/world-site conditions.
5. Accepted jobs move creatures into initial/continue states.
6. State functions run movement, work, cleanup, combat, or need behavior.
7. Computer keeper tasks operate as a higher-level director.
