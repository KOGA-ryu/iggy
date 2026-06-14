# DevilutionX AI Study

Reference repo: `/Users/kogaryu/iggy/DevilutionX-master`

DevilutionX is useful because it shows a classic ARPG runtime where monster AI,
animation mode, combat state, map occupancy, lighting, vision, save/load, and
network determinism are tightly connected. The lesson for Iggy is not to copy the
global structure. The lesson is how ARPG monsters bridge between static monster
tables, live actor state, tile occupancy, path queries, LOS checks, and save
state.

## Core Ownership Map

### Static Monster Data

Useful files:

- `/Users/kogaryu/iggy/DevilutionX-master/assets/txtdata/monsters/monstdat.tsv`
- `/Users/kogaryu/iggy/DevilutionX-master/assets/txtdata/monsters/unique_monstdat.tsv:1`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/tables/monstdat.h:22`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/tables/monstdat.h:98`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/tables/monstdat.h:322`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/tables/monstdat.h:350`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/tables/monstdat.cpp:326`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/tables/monstdat.cpp:427`

Monster definitions are table-driven. `MonsterData` owns durable monster type
facts: name, art/sound ids, dungeon availability, animation frame/rate data,
level range, HP range, AI id, ability flags, intelligence, damage, armor,
class, resistances, selection region, treasure, and experience.

`UniqueMonsterData` overlays special monster data: base monster type, unique
name/translation art, level, max HP, AI id, intelligence, damage, stat drain,
resistance, pack behavior, custom hit/armor, and talk message.

Iggy lesson: use data tables for profile facts and special-case overlays. An
`NpcDefinition2D` can carry default AI id, movement/combat affordances, and
animation tags; unique actors can override a small subset without becoming a new
system.

### Level Monster Type Cache

Useful files:

- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:180`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:186`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:210`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:3290`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:3433`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:3519`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:3548`

`CMonster` is a per-level runtime/cache object for a monster type. It owns loaded
animation data, animation structs, sounds, corpse id, and placement flags for
the current level.

This is not the same as static monster data and not the same as live monster
state. It is a runtime resource cache selected for the current dungeon level.

Iggy lesson: separate static `NpcDefinition2D` from loaded runtime resources.
Per-level actor resource caches should sit near asset/runtime state, not inside
every NPC actor.

### Live Monster State

Useful files:

- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:75`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:120`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:212`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:229`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:248`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:250`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:253`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:256`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:263`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:270`

`Monster` is the live actor. It owns HP, flags, drop seed, AI seed, current
position, goal, enemy position, level monster type index, mode, path count,
direction, enemy id, AI id, intelligence, activity timer, unique type, corpse
data, damage, armor, leader/pack state, light id, and animation state.

Two fields are especially instructive:

- `MonsterGoal`: durable behavior intention such as normal, retreat, healing,
  move, attack, inquiring, talking.
- `MonsterMode`: current executable/visual state such as stand, movement,
  melee/ranged attack, hit recovery, death, delay, petrified, heal, talk.

Iggy lesson: distinguish objective from execution mode. `NpcObjective2D` should
not be the same thing as `NpcMoveMode2D` or animation state.

### Dungeon Grid and Query State

Useful files:

- `/Users/kogaryu/iggy/DevilutionX-master/Source/levels/gendung.h:64`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/levels/gendung.h:111`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/levels/gendung.h:149`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/levels/gendung.h:155`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/levels/gendung.h:159`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/levels/gendung.h:161`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/levels/gendung.h:169`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/levels/gendung.h:204`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/levels/gendung.h:244`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/levels/gendung.h:296`

The dungeon owns many tile arrays: generated tile ids, rendered pieces,
transparency values, realtime light, precomputed static light, flags, player
occupancy, monster occupancy, object occupancy, item occupancy, corpse ids, and
tile property data.

The AI reads tile visibility, walkability, safety, occupancy, solid flags,
doors, objects, and line checks through helpers. This means the map is not just
render data. It is the query authority for movement and combat legality.

Iggy lesson: a level-owned query world is appropriate for ARPG AI. Keep actor
state in actor registries, but keep tile occupancy/collision/visibility lookup
as derived level/runtime query state.

## AI Update Flow

Useful files:

- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:3090`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4257`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4264`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4265`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4278`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4295`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4298`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4313`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4321`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4331`

`ProcessMonsters` is the main monster tick. The flow is:

1. Clear pending monster deletion list.
2. Iterate active monster ids.
3. Apply leader/follower behavior.
4. In multiplayer, seed deterministic AI randomness from the monster.
5. Regenerate HP if allowed.
6. If a monster becomes visible, update enemy targeting.
7. Copy enemy future position into monster memory.
8. Activate visible monsters and decay offscreen activity.
9. If search/path mode applies, try path planning.
10. Otherwise dispatch the monster's AI function through `AiProc`.
11. Update mode/stance until the mode no longer chains.
12. Process animation unless locked/special.

Iggy lesson: AI tick should be explicit about phase order. Good phases for Iggy:
refresh visibility/perception, update target memory, run objective/path planner,
run behavior policy, emit action proposal, execute accepted mode, advance
animation/reporting.

## Behavior Dispatch

Useful files:

- `/Users/kogaryu/iggy/DevilutionX-master/Source/tables/monstdat.h:22`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1977`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:2014`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:2070`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:2100`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:2125`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:3090`

Monster type data chooses a `MonsterAIID`. `AiProc` maps that id to a function
such as zombie, skeleton, ranged, ranged avoidance, scavenger, fallen, bat,
gargoyle, butcher, golem, boss, and unique quest behaviors.

The behavior functions inspect mode, activity, distance, LOS, intelligence,
previous mode variables, and map state. They start attacks, ranged attacks,
special attacks, random walks, delays, retreats, or door checks.

Iggy lesson: a behavior store can map profile ids to policy modules. Use the
mapping idea, but return typed proposals instead of letting each policy mutate
everything directly.

## Movement, Pathing, and LOS

Useful files:

- `/Users/kogaryu/iggy/DevilutionX-master/Source/engine/path.h:19`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/engine/path.h:28`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/engine/path.h:39`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/engine/path.cpp:32`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/engine/path.cpp:184`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/engine/path.cpp:246`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1596`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1732`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1776`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1805`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1816`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1861`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4384`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4389`

Pathfinding is a reusable function that accepts callbacks:

- whether a step between adjacent points is legal
- whether a target position can be occupied
- start and destination
- output path buffer and max length

Monster pathing wraps this with monster-specific accessibility and safety
checks. Before pathfinding, `AiPlanPath` tries a line-clear check and only plans
when direct movement is blocked or search behavior has built up enough. Ranged
AI separately checks missile line clear before attacking.

Iggy lesson: keep pathfinding generic and inject level/actor legality. AI should
not know every collision rule; it should ask a movement candidate/path query that
knows the actor and the map.

## Save/Load Ownership

Useful files:

- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:663`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:783`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:1119`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:1128`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:1131`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:1502`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:2467`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:2536`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:2553`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:2567`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:2575`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:2593`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:2600`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:2610`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:2644`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:2665`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:2679`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:2762`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:2822`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:2865`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:2870`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:2883`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/loadsave.cpp:2885`

Saved as real state:

- game header/version flavor
- current level, set level, view position, UI flags
- player state and inventory
- quest and portal state
- monster kill counts
- active monster ids and live monster records
- missiles, objects, active lights, vision/light records
- dropped items and unique item flags
- selected tile flags
- player grid, monster grid, object grid, corpse grid
- static pre-light and automap exploration
- premium items and stash-related data

Rebuilt or resynced after load:

- level runtime structures via level load before applying saved actors
- animation pointers/data for players, monsters, and objects
- dropped item lookup grid from saved item records
- invalid/removed monsters and enemy references
- dLight from pre-light plus active lights
- player vision and vision list processing
- light list processing
- missile flags and missile animation data
- premium item compatibility changes

Iggy lesson: save authoritative actor/session facts and enough level facts to
restore the session. Rebuild indexes and presentation/runtime caches after load.

## ARPG Patterns Worth Copying Conceptually

- Monster definitions are data tables, not ad hoc code.
- Unique monsters are overlays on base monster types.
- Live monsters have both `goal` and `mode`.
- AI uses distance, LOS, visibility, tile safety, and activity timers.
- Offscreen monsters can sleep or decay activity.
- Ranged and melee behavior share map query services.
- Pathfinding is generic with callbacks for movement legality.
- Dungeon occupancy arrays make entity queries cheap.
- Save/load validates and resyncs runtime data after reading old state.

## Do Not Copy

- Global dungeon arrays as the only ownership mechanism.
- Direct mutation from every AI function.
- Generic `var1`, `var2`, `var3`, `goalVar1`, `goalVar2`, `goalVar3` fields as
  the public extension model.
- Animation mode and behavior mode being too tightly coupled.
- Large monolithic save/load functions.
- Behavior dispatch as raw function pointers without typed proposal boundaries.
- Network-determinism constraints unless Iggy explicitly needs multiplayer sync.

## Iggy Translation

Good Iggy type shape:

- `NpcDefinition2D`: static type data, tags, base stats, AI profile id.
- `NpcUniqueOverlay2D`: optional actor/profile overrides.
- `NpcRuntimeResourceCache2D`: loaded sprites/sounds/animation data for active
  NPC definitions.
- `NpcActorState2D`: live HP/status/position/target/objective/mode state.
- `NpcObjective2D`: durable goal such as patrol, pursue, retreat, talk, heal.
- `NpcActionMode2D`: current executable mode such as stand, move, attack, hit,
  delay, death.
- `NpcAiActivityState2D`: visible/asleep/alert/decaying activity timer.
- `LevelOccupancyGrid2D`: derived grid for players, NPCs, items, objects.
- `NpcMovementLegalityQuery2D`: level-owned query using tile, collision,
  occupancy, safety, door, and actor capability.
- `NpcAiPolicyStore2D`: maps profile ids to policy functions/modules.
- `NpcActionProposal2D`: typed result from policy before runtime execution.

Ownership recommendation:

- `scene/npc`: definitions, overlays, actor state.
- `scene/level`: tile data, tile properties, semantic regions.
- `runtime`: active actor registries, tick order, command/proposal execution.
- `runtime/cache` or `scene/level/cache`: occupancy grids, visibility/light/path
  scratch data.
- `assets/runtime`: loaded animation/sound resources for active definitions.
- save snapshot: session facts, level id/seed, actors, inventory/items, quest
  state, persistent exploration, persistent objects.

## Minimal Iggy Lesson

For an ARPG-like engine, NPC AI gets cleaner if each tick follows this shape:

1. Actor state says who the NPC is and what it remembers.
2. Definition/profile says what the NPC can do.
3. Level query state says what is legal right now.
4. AI policy proposes an action.
5. Runtime accepts, rejects, or translates the action into movement/combat/events.
6. Animation/render state follows the accepted action, not the other way around.
7. Save/load persists actor/session truth and rebuilds query/render caches.

DevilutionX proves that ARPG AI needs tight access to map occupancy, LOS, and
session state. Iggy should keep that access explicit and typed instead of global.
