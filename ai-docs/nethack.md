# NetHack AI Study

Reference repo: `/Users/kogaryu/iggy/NetHack-NetHack-5.0`

NetHack is useful because its monster AI is old, dense, and very data-rich. The
best lesson is not its control flow. The best lesson is how much behavior can be
driven from static monster definitions, live actor state, terrain queries, and a
small set of special behavior hooks.

## Core Ownership Map

### Static Monster Definitions

Useful files:

- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/permonst.h:40`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/permonst.h:56`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monflag.h:10`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monflag.h:85`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monflag.h:123`

`permonst` is the monster species definition. It owns the durable type facts:
name, glyph class, level, speed, armor class, magic resistance, alignment,
attacks, weight, nutrition, sound, size, resistances, and behavior flags.

The flag files split static behavior hints into categories:

- `M1_*`: movement/body/capability flags such as flying, swimming, tunneling,
  wall-walking, hiding, hands, eyes, regeneration, teleporting, diet.
- `M2_*`: role and attitude flags such as hostile, peaceful, domestic, wandering,
  stalking, collecting, greed, magic use, item interest.
- `M3_*`: special desire/wait/covetous flags.

Learning point: monster species/profile data is separate from live actor state.
Static capabilities answer "what can this monster ever do?" before runtime
decision code runs.

### Live Monster State

Useful files:

- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:96`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:107`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:108`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:111`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:116`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:122`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:170`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:189`

`monst` is the live monster record. It points back to `permonst` and owns
position, believed hero position, track memory, HP, tameness, status effects,
visibility state, role flags, strategy flags, and current goal.

Important boundary: NetHack mixes too much into one live struct, but the
separation between static `permonst` and live `monst` is still strong.

Learning point: the live actor points back to static definition data and stores
current truth such as position, status, target belief, strategy, and memory.
NetHack mixes many concerns into one struct, but it still keeps species facts
outside the live monster.

### Optional Role Extensions

Useful files:

- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mextra.h:13`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mextra.h:77`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mextra.h:95`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mextra.h:123`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mextra.h:157`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mextra.h:205`

`mextra` stores optional role-specific extension data for guards, priests,
shopkeepers, minions, pets, and other special cases.

This is a useful pattern even though the implementation is pointer-heavy. Special
behaviors often need persistent memory that should not pollute every NPC.

Learning point: special behavior often needs persistent role memory. NetHack
keeps guard, priest, shopkeeper, minion, pet, and other role data in optional
extension records instead of putting every field on every monster.

### Map and Terrain Queries

Useful files:

- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/rm.h:55`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/rm.h:117`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mfndpos.h:10`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mfndpos.h:33`

Terrain is represented as level tiles with query macros for walls, rooms,
water, lava, doors, accessibility, and other tile traits.

`mfndpos` returns a compact movement-candidate result: up to adjacent positions
plus per-position flags describing what interaction would be required or allowed.

Learning point: movement decision code first gathers legal candidates and reason
flags, then action selection decides whether the result is movement, attack,
opening, unlocking, digging, waiting, or failure.

## Turn Flow

Useful files:

- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/mon.c:1124`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/mon.c:1211`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/mon.c:1329`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:690`

The broad flow is:

1. Calculate monster movement points.
2. Iterate live monsters.
3. Run a per-monster turn dispatcher.
4. Handle status, wake/flee/confusion/special cases.
5. Decide whether to use an item, cast, attack, move, or wait.
6. Execute the chosen action and update live state.

The per-monster dispatcher is powerful but monolithic. It is valuable as a phase
map, not as a structure to copy.

Learning point: NetHack's monster turn can be read as phases even though the
implementation is dense:

- refresh status and perception
- resolve objective
- generate action proposals
- score/select proposal
- produce command/effect requests
- apply accepted movement or action result

## Movement Candidate Generation

Useful files:

- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1717`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1765`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1771`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1857`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1920`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:1927`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:2000`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/monmove.c:2017`

`m_move` combines capability checks, special role handling, target selection,
item pickup, movement allow flags, candidate generation, and candidate execution.

The useful shape is the middle:

- derive allow flags from actor capability and current intent
- ask the map for possible positions
- preserve metadata about doors, traps, walls, hero occupancy, monsters, and
  other blockers
- choose whether the candidate implies movement, attack, interaction, or no-op

Learning point: movement candidates carry structured reasons. The AI is not
choosing from raw coordinates alone.

## Behavior Hooks

Useful files:

- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/mhitu.c:491`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/mhitm.c:293`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/muse.c:441`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/muse.c:1421`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/dog.c:691`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/dog.c:1020`

NetHack plugs special behavior into the monster turn through dedicated functions:
attacking the hero, attacking other monsters, defensive item use, offensive item
use, pet movement, pet hunger, covetous behavior, guards, priests, shopkeepers,
and other role-specific rules.

Learning point: special behavior plugs into the general monster runner through
dedicated functions for attacks, item use, pet behavior, hunger, guards, priests,
shopkeepers, and other role-specific rules.

## Pets

Useful files:

- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mextra.h:172`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/dog.c:691`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/dog.c:720`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/dog.c:1020`

Pets are not just friendly monsters. They have memory: parent, dropped item
timing, distance, apport/training, whistle timing, hunger timing, previous goal,
abuse, revivals, starvation penalty, and whether the player killed them.

Learning point: pets are modeled with explicit relationship and training memory,
not as ordinary hostile monsters with a single friendliness flag changed.

## Save/Restore Shape

Useful files:

- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/save.c:836`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/save.c:842`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/save.c:845`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/save.c:894`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/save.c:907`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/restore.c:306`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/restore.c:400`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/restore.c:1198`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/restore.c:1202`

NetHack persists live monsters and their inventories, plus optional extension
state. On restore it rebuilds pointers, extension links, id mappings, level
monster placement, worm segments, stuck/steed links, and hiding placement.

Learning point: NetHack persists live monsters and their inventories/extensions,
then repairs links, ids, placement, and special side structures during restore.

## Reference Pattern

NetHack's reusable conceptual pattern:

- static monster definitions describe capabilities and behavior affordances
- live monsters reference static definitions instead of duplicating them
- optional role memory handles special cases
- movement candidates carry reason flags
- the monster turn stages status, perception, movement, attack, item use, and
  role-specific behavior
- save files persist live truth, then restore repairs runtime links and indexes

## Legacy Costs

The reference also shows costs:

- giant mutable actor structs as the primary extension mechanism
- monolithic per-turn movement functions
- hidden global ownership
- behavior rules spread through hardcoded switches
- raw pointer-oriented save/restore design
- many special roles coupled to legacy global state

## Minimal Reference Lesson

NetHack's cleanest skeleton is:

1. Monster definitions say what a monster type can do.
2. Live monster state says what this monster is currently doing and remembers.
3. Level queries say what the map allows right now.
4. Behavior hooks choose movement, attack, item use, or special action.
5. Execution mutates live monster and level state.
6. Save files persist actor truth and role memory, not candidate scratch data.

That gives the useful part of NetHack's AI without inheriting the legacy control
flow.
