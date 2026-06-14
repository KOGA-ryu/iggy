# AI Programming Methods Tally

References summarized:

- `/Users/kogaryu/iggy/ai-docs/openxcom.md`
- `/Users/kogaryu/iggy/ai-docs/warzone2100.md`
- `/Users/kogaryu/iggy/ai-docs/nethack.md`
- `/Users/kogaryu/iggy/ai-docs/devilutionx.md`
- `/Users/kogaryu/iggy/ai-docs/keeperfx.md`
- `/Users/kogaryu/iggy/ai-docs/re3-miami.md`

Legend: `Y` = clear pattern, `P` = partial/variant, `-` = not a main pattern in
the study.

## Method Tally

| Programming method / strategy | OpenXcom | Warzone | NetHack | DevilutionX | KeeperFX | re3 Miami | Count |
|---|---:|---:|---:|---:|---:|---:|---:|
| Static definition/profile data separate from live actor state | Y | Y | Y | Y | Y | Y | 6 |
| Live actor owns mutable truth, not static rules | Y | Y | Y | Y | Y | Y | 6 |
| Map/level/session owns query authority | Y | Y | Y | Y | Y | Y | 6 |
| Save authoritative state, rebuild derived caches/indexes after load | Y | Y | Y | Y | Y | Y | 6 |
| Phase-based tick/turn runner | Y | Y | Y | Y | Y | P | 6 |
| Durable objective/order/goal separate from execution state/mode | P | Y | P | Y | Y | Y | 5 |
| Movement/path-follow scratch separate from behavior definition | Y | Y | Y | Y | Y | Y | 6 |
| Short-lived action/proposal/command object | Y | P | P | P | P | P | 6 |
| Candidate generation before action selection | Y | Y | Y | Y | Y | P | 6 |
| Data-driven tables/config/flags drive behavior | Y | Y | Y | Y | Y | Y | 6 |
| Behavior dispatch from type/profile id | P | P | Y | Y | Y | P | 6 |
| Special behavior hooks/modules plugged into general runner | P | P | Y | Y | Y | P | 6 |
| Perception/threat memory separate from static threat profile | P | P | Y | Y | Y | Y | 6 |
| Previous objective/state restore for temporary actions | P | P | P | P | Y | Y | 6 |
| Work-site/interaction capacity owned by world object | - | P | P | - | Y | Y | 4 |
| Director/faction/global AI separate from individual actors | - | Y | - | P | Y | P | 4 |
| Optional role-specific persistent memory | - | P | Y | Y | Y | P | 5 |
| Standing policy flags layered over current order/objective | - | Y | Y | P | Y | P | 5 |
| Level-owned path graph/navigation cache | Y | Y | Y | Y | Y | Y | 6 |
| Runtime resource/cache separate from static definition and actor state | P | P | - | Y | P | P | 5 |

## Concrete Implementation Tactics Tally

This is the lower-level programming-method view: what tools they actually use
to build those AI systems.

| Implementation tactic | OpenXcom | Warzone | NetHack | DevilutionX | KeeperFX | re3 Miami | Count |
|---|---:|---:|---:|---:|---:|---:|---:|
| Raw pointers / object references between actors, maps, targets | Y | Y | Y | Y | Y | Y | 6 |
| Pointer fixup or reference repair after load | P | P | Y | Y | Y | Y | 6 |
| Raw ids / indexes / handles for actors, rooms, levels, path nodes | Y | Y | Y | Y | Y | Y | 6 |
| Fixed-size arrays for pools, maps, path nodes, actors, flags | P | Y | Y | Y | Y | Y | 6 |
| Dynamic arrays / vectors / growable lists | Y | Y | P | P | P | Y | 6 |
| Linked lists or intrusive list links | P | Y | Y | Y | Y | P | 6 |
| Bit flags / bitfields for capabilities, state, policy, map flags | Y | Y | Y | Y | Y | Y | 6 |
| Enums for states, orders, objectives, actions, modes | Y | Y | Y | Y | Y | Y | 6 |
| Large structs/classes as live actor records | Y | Y | Y | Y | Y | Y | 6 |
| Smaller optional extension structs for special roles | - | P | Y | P | Y | P | 5 |
| Function pointer tables / callback dispatch | - | P | P | Y | Y | P | 5 |
| Virtual methods / class polymorphism | Y | Y | - | P | - | Y | 4 |
| Big switch dispatch over states/objectives/actions | P | Y | Y | Y | Y | Y | 6 |
| Table/config-driven behavior lookup | Y | Y | Y | Y | Y | Y | 6 |
| Short-lived command/action/proposal structs | Y | Y | P | P | P | P | 6 |
| Fixed pools/registries for live objects | P | Y | Y | Y | Y | Y | 6 |
| Global or singleton world services | P | Y | Y | Y | Y | Y | 6 |
| Save/load by binary blocks or structured chunks | Y | Y | Y | Y | Y | Y | 6 |
| Recomputed transient arrays/candidates each tick/turn | Y | Y | Y | Y | Y | P | 6 |
| Manual ownership cleanup on state/object deletion | P | P | Y | Y | Y | Y | 6 |

### What Those Tactics Mean

**Pointers**

All six use pointers or object references heavily: actor to target, actor to
map/session, actor to vehicle, actor to room, actor to path node, actor to
profile. Older C/C++ games also need cleanup/fixup after load or deletion.

**Arrays**

Arrays are everywhere: tile maps, occupancy maps, path nodes, actor pools,
candidate lists, route buffers, save chunks, per-player tables, flags. Fixed
arrays are common because these games care about deterministic iteration,
cache locality, and simple save/load.

**Vectors / Growable Lists**

Modern C++ references use vectors for active lists, queues, route points, and
manager-owned collections. re3's attractor manager is the clearest example:
vectors own active attractors by type, while each attractor owns actor queues.

**Linked Lists**

Older engines use linked lists or intrusive links for map buckets, rooms,
dungeon lists, live object pools, creature room membership, and active object
iteration.

**Bitflags**

Flags are one of the most universal techniques:

- movement capabilities
- monster traits
- threat masks
- map collision/visibility flags
- secondary policy
- path node mutable state
- save/load dirty or availability bits

**Enums**

Every game uses enums for objectives, states, orders, actions, modes, movement
styles, jobs, room types, and mission types.

**Switches and Dispatch Tables**

Big switches are common. Function pointer tables are common in KeeperFX and
DevilutionX-style systems. Virtual dispatch appears more in C++ app/game layers.

**Pools and Registries**

Live objects usually live in pools/registries. Actors often refer to each other
through ids, indexes, or pointers into pools. This supports fast iteration and
save/load, but creates cleanup/fixup complexity.

**Save Buffers / Chunks**

All games use block/chunk save flows in some form. The old ones serialize broad
runtime structures; the better lesson is that they still rebuild dynamic
pointers, path flags, collision, streaming, and occupancy after load.

## Strongest Repeated Strategies

### 1. Definition / Actor / Runtime Cache Split

Used by all six.

Mature engines avoid making one NPC object own everything. The recurring split:

- definition/profile: what the actor type can ever do
- actor state: what this actor is right now
- runtime/cache state: derived graph, render, resource, path, occupancy, or
  loaded-per-level data

### 2. Objective Is Not Execution State

Strong in Warzone, DevilutionX, KeeperFX, and re3; partial in OpenXcom and
NetHack.

Common layers:

- objective/order/goal: durable intent
- state/mode/action: current executor step
- move mode/path state: movement intensity and path-follow scratch

### 3. AI Proposes, Runtime Executes

Clearest in OpenXcom, present as a variant everywhere.

Good engines keep decision and execution apart:

- AI gathers context
- AI emits action/proposal/order/state request
- runtime validates and executes through movement/combat/interaction systems

### 4. Map-Owned Intelligence

Used by all six.

AI depends on level/session services for:

- walkability
- collision
- path nodes
- occupancy
- line of sight
- visibility
- interaction site capacity
- spawn or route data

### 5. Candidate Generation First

Used by all six.

The repeated pattern:

- generate legal move/action candidates
- attach reasons/metadata
- score or select
- only then execute

### 6. Data-Driven Behavior Flags

Used by all six.

Definitions carry flags and parameters:

- movement capability
- aggression/fear/intelligence
- job eligibility
- threat masks
- special behavior ids
- role tags

### 7. Special Hooks Should Not Replace the Runner

Used by all six.

Special behavior exists, but mature games still route it through the normal
turn/tick loop:

- pets
- bosses
- jobs
- attractors
- vehicle missions
- spells/items
- room work

### 8. Persist Truth, Rebuild Caches

Used by all six.

Save files keep actor/session/world truth and small mutable flags. They rebuild
or repair:

- path graph links
- occupancy
- visibility
- collision
- render/resource caches
- pointer/id links
- UI/debug models

## Tradeoffs and Costs Observed

| Pattern / cost | Seen in | Observed consequence |
|---|---|---|
| Giant mutable actor struct | NetHack, DevilutionX, KeeperFX, re3 | Convenient central access, but many systems touch the same actor record. |
| Huge switch as primary architecture | NetHack, DevilutionX, re3 | Easy to follow locally, but grows into broad centralized control flow. |
| Hidden direct mutation from AI | most older engines | Decision code often changes actor, map, animation, and side links directly. |
| Global singleton world access | most older engines | AI can query anything easily, but dependencies are implicit. |
| Raw save of broad runtime structs | KeeperFX, older GTA style | Fast to implement for a fixed engine shape, but requires repair hooks and version care. |
| Hardcoded game-specific behavior names | all | Behavior vocabulary closely matches each game's mechanics and world fiction. |
