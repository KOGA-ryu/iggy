# AI Research Grep Commands

These are the commands used to find the AI skeletons in the local reference
repos. The point is not just the answers; it is learning how to find the
answers again.

## Search Method

Use a three-pass search:

1. Find names: structs, classes, enums, and obvious verbs.
2. Narrow to owner files: headers for data shape, `.cpp`/`.c` files for flow.
3. Open exact line ranges with `nl -ba` or `sed` after grep gives anchors.

Useful generic commands:

```bash
rg --files /path/to/repo | rg "AI|Ai|Ped|Monster|Droid|Creature|Path|Save|Load"
rg -n "enum .*State|enum .*Order|enum .*Objective|struct .*AI|class .*AI" /path/to/repo/src
nl -ba /path/to/file.cpp | sed -n '120,220p'
```

## Why This Syntax

### `rg -n`

```bash
rg -n "pattern" /path/to/search
```

`rg` is ripgrep. It recursively searches files. `-n` prints line numbers, which
turns search results into source anchors you can cite later.

Use this when you know text patterns but not exact files.

### Quotes Around the Pattern

```bash
rg -n "struct MonsterData|enum class MonsterGoal|ProcessMonsters" /path
```

The quotes make the whole expression one argument. Without quotes, the shell may
split or interpret characters before `rg` sees them.

### `|` Inside the Pattern

```bash
"class AIModule|struct BattleAction|class BattleUnit"
```

Inside the quoted regex, `|` means "or." This lets one command search several
related clues at once: type names, enum names, update functions, and save/load
functions.

This is useful for architecture research because one search can reveal the
definition object, live actor object, command object, update flow, and
persistence boundary.

### `.*` Inside the Pattern

```bash
"enum .*State|enum .*Order|struct .*AI"
```

`.` means any character. `*` means repeat zero or more times. Together, `.*`
means "anything between these words."

This finds families of names when you do not know the exact identifier yet:

- `enum PedState`
- `enum CreatureStates`
- `enum class MonsterMode`
- `struct SomeAIThing`

### Escaped Parentheses

```bash
"think\(|m_move\(|Save\("
```

In regex, `(` has special meaning for grouping. `\(` means "literal open
parenthesis." Searching for `think\(` finds function calls or function
definitions named `think`, not comments that only mention the word.

Use this when you want verbs as functions instead of general prose.

### Prefix Patterns

```bash
"BA_|AI_PATROL|DORDER_|DACTION_|M1_|M2_|M3_"
```

Prefixes are useful in old C/C++ code because enum families often share naming
prefixes.

Examples:

- `BA_` finds OpenXcom battle actions.
- `DORDER_` finds Warzone droid orders.
- `DACTION_` finds Warzone droid actions.
- `M1_`, `M2_`, `M3_` find NetHack monster flag families.

### Multiple Search Roots

```bash
rg -n "pattern" \
  /repo/src/Battlescape \
  /repo/src/Savegame \
  /repo/src/Mod
```

Multiple paths at the end limit the search to likely ownership zones. This is
faster and produces less noise than searching the whole repo.

For AI research, a good split is:

- gameplay/runtime folders for update flow
- save/session folders for persisted state
- mod/data folders for static definitions
- path/map folders for query ownership

### Backslash at End of Line

```bash
rg -n "long pattern" \
  /first/path \
  /second/path
```

The trailing `\` tells the shell the command continues on the next line. It is
only for readability; the command is still one command.

### Pipeline Between Commands

```bash
rg --files /path/to/repo | rg "AI|Ped|Monster|Path"
```

The first command lists files. The `|` outside quotes is a shell pipe: it sends
the file list into the second `rg`, which filters filenames.

This is different from `|` inside quotes:

- inside quotes: regex "or"
- outside quotes: shell pipe between commands

### `nl -ba` and `sed -n`

```bash
nl -ba /path/to/file.cpp | sed -n '120,220p'
```

`nl -ba` prints a file with line numbers, including blank lines. `sed -n
'120,220p'` prints only lines 120 through 220.

Use this after `rg` finds an anchor and you want to read the surrounding code
without opening an entire huge file.

### Why Mix Nouns and Verbs

The search patterns intentionally mix:

- nouns: `Monster`, `DROID`, `CPed`, `BattleUnit`
- state words: `Objective`, `Order`, `Mode`, `State`, `Action`
- verbs: `Process`, `Update`, `think`, `Save`, `Load`
- map/query words: `Path`, `LineClear`, `Attractor`, `Room`, `Job`

Nouns find ownership. Verbs find flow. State words find vocabulary. Save/load
words find persistence boundaries.

## OpenXcom

```bash
rg -n "class AIModule|struct BattleAction|class BattleUnit|class SavedBattleGame|think\(|BA_|AI_PATROL|AI_AMBUSH|AI_COMBAT|AI_ESCAPE" \
  /Users/kogaryu/iggy/OpenXcom-master/src/Battlescape \
  /Users/kogaryu/iggy/OpenXcom-master/src/Savegame \
  /Users/kogaryu/iggy/OpenXcom-master/src/Mod
```

What this finds:

- `AIModule`: decision module attached to a battle unit.
- `BattleAction`: command/proposal object returned by AI.
- `BattleUnit`: live actor truth.
- `SavedBattleGame`: battle/session/map owner.
- `BA_*` and `AI_*`: action and mode vocabulary.

Why this syntax:

- class/struct names find ownership containers
- `think\(` finds the decision function rather than general comments
- `BA_` finds battle action enum values
- `AI_PATROL|AI_AMBUSH|AI_COMBAT|AI_ESCAPE` finds the AI mode vocabulary
- separate `Battlescape`, `Savegame`, and `Mod` roots split runtime flow, saved
  actor/session state, and static rules

Good follow-up:

```bash
nl -ba /Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp | sed -n '130,230p'
nl -ba /Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.cpp | sed -n '220,285p'
```

## Warzone 2100

```bash
rg -n "struct DROID|class DROID|DroidOrder|MOVE_CONTROL|DORDER_|DACTION_|updateDroid|orderUpdate|actionUpdate|moveUpdate|SECONDARY|DROID_ORDER|DROID_ACTION" \
  /Users/kogaryu/iggy/warzone2100-master/src
```

What this finds:

- `DROID`: live unit record.
- `DroidOrder`: durable order payload.
- `MOVE_CONTROL`: movement/path-follow state.
- `DORDER_*`, `DACTION_*`, `SECONDARY_*`: high-level intent, executor state,
  and standing policy.
- `orderUpdateDroid`, `actionUpdateDroid`, `moveUpdateDroid`: tick phases.

Why this syntax:

- `DroidOrder`, `DROID_ORDER`, and `DORDER_` search durable intent
- `DROID_ACTION` and `DACTION_` search current execution state
- `MOVE_CONTROL` searches path/motion scratch
- `updateDroid|orderUpdate|actionUpdate|moveUpdate` searches phase order
- `SECONDARY` finds standing policy flags layered over primary orders

Good follow-up:

```bash
nl -ba /Users/kogaryu/iggy/warzone2100-master/src/droid.cpp | sed -n '910,965p'
nl -ba /Users/kogaryu/iggy/warzone2100-master/src/orderdef.h | sed -n '35,125p'
nl -ba /Users/kogaryu/iggy/warzone2100-master/src/actiondef.h | sed -n '20,90p'
```

## NetHack

```bash
rg -n "struct permonst|struct monst|struct mextra|mfndpos|m_move\(|movemon\(|dochug|muse|dog_move|savelev|restmonchn|M1_|M2_|M3_" \
  /Users/kogaryu/iggy/NetHack-NetHack-5.0/include \
  /Users/kogaryu/iggy/NetHack-NetHack-5.0/src
```

What this finds:

- `permonst`: static monster definition.
- `monst`: live monster.
- `mextra`: optional monster extension state.
- `mfndpos`: movement candidate data.
- `m_move`, `dochug`, `mattacku`, `muse`, `dog_move`: turn/action hooks.
- `M1_*`, `M2_*`, `M3_*`: behavior/capability flags.

Grep lesson: NetHack has many `struct monst *` references, so broad searches
get noisy. Refine to headers first, then specific behavior files.

Why this syntax:

- `permonst|monst|mextra` finds static definition, live actor, and optional
  extension records
- `mfndpos` finds movement candidate data
- `m_move\(|movemon\(|dochug` searches monster turn/movement functions
- `muse|dog_move` finds special item-use and pet behavior hooks
- `savelev|restmonchn` finds save/restore boundaries
- `M1_|M2_|M3_` finds static monster capability/role flag families

Good follow-up:

```bash
nl -ba /Users/kogaryu/iggy/NetHack-NetHack-5.0/include/permonst.h | sed -n '50,95p'
nl -ba /Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h | sed -n '90,200p'
nl -ba /Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mfndpos.h | sed -n '25,70p'
```

## DevilutionX

```bash
rg -n "struct MonsterData|struct UniqueMonsterData|class Monster|enum class MonsterGoal|enum class MonsterMode|ProcessMonsters|AiProc|AiPlanPath|FindPath|LineClear|SaveMonster|LoadMonster" \
  /Users/kogaryu/iggy/DevilutionX-master/Source \
  /Users/kogaryu/iggy/DevilutionX-master/assets/txtdata/monsters
```

What this finds:

- `MonsterData` and `UniqueMonsterData`: static monster records.
- `Monster`: live monster actor.
- `MonsterGoal`, `MonsterMode`: durable intent and executor state.
- `ProcessMonsters`: monster update phase.
- `AiProc`: behavior dispatch table.
- `AiPlanPath`, `FindPath`, `LineClear`: movement and visibility helpers.
- `SaveMonster`, `LoadMonster`: persistence boundary.

Why this syntax:

- `MonsterData|UniqueMonsterData` finds static base and unique overlays
- `MonsterGoal|MonsterMode` finds intent versus executor state
- `ProcessMonsters` finds the monster tick entry point
- `AiProc` finds behavior dispatch
- `AiPlanPath|FindPath|LineClear` finds path and LOS query boundaries
- `SaveMonster|LoadMonster` finds persistence shape

Good follow-up:

```bash
nl -ba /Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp | sed -n '3080,3135p'
nl -ba /Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp | sed -n '4250,4340p'
nl -ba /Users/kogaryu/iggy/DevilutionX-master/Source/engine/path.cpp | sed -n '170,220p'
```

## KeeperFX

```bash
rg -n "struct Thing|struct CreatureControl|struct CreatureJobConfig|struct CreatureModelConfig|enum CreatureStates|enum CreatureStateTypes|struct Room|struct Dungeon|struct ComputerTask|struct Computer2|process_func_list|send_creature_to_job|init_creature_state|save_game_chunks|load_game_chunks" \
  /Users/kogaryu/iggy/keeperfx-master/src \
  /Users/kogaryu/iggy/keeperfx-master/config
```

What this finds:

- `Thing`: live world object.
- `CreatureControl`: creature brain/control extension.
- `CreatureModelConfig`: static creature definition.
- `CreatureJobConfig`: job/work metadata.
- `Room`, `Dungeon`: map/work-site/faction owners.
- `process_func_list`: state dispatch table.
- `ComputerTask`, `Computer2`: director/player AI structures.
- save/load chunk functions.

Grep lesson: `struct Thing *` appears everywhere. Once it identifies the owner
files, refine to specific files:

```bash
rg -n "struct Thing|struct CreatureControl|enum CreatureStates" \
  /Users/kogaryu/iggy/keeperfx-master/src/thing_data.h \
  /Users/kogaryu/iggy/keeperfx-master/src/creature_control.h \
  /Users/kogaryu/iggy/keeperfx-master/src/creature_states.h
```

Why this syntax:

- `Thing|CreatureControl` finds common live entity state and creature-specific
  brain/control state
- `CreatureJobConfig|CreatureModelConfig` finds static job and creature data
- `Room|Dungeon` finds work-site and faction/session ownership
- `ComputerTask|Computer2` finds director AI
- `process_func_list` finds state dispatch
- `send_creature_to_job|init_creature_state` finds job/state flow
- `save_game_chunks|load_game_chunks` finds chunk persistence

## re3 Miami

```bash
rg -n "enum eObjective|enum PedState|enum eMoveState|class CPed|class CPedType|class CPedStats|class CPedAttractor|class CPedAttractorManager|class CPathFind|class CAutoPilot|SetObjective|ProcessObjective|SetFollowPath|SetNewAttraction|LoadSavedGame|Save\(" \
  /Users/kogaryu/iggy/re3-miami/src/peds \
  /Users/kogaryu/iggy/re3-miami/src/control \
  /Users/kogaryu/iggy/re3-miami/src/save \
  /Users/kogaryu/iggy/re3-miami/src/vehicles
```

What this finds:

- `CPedType`, `CPedStats`: profile/type data.
- `CPed`: live actor.
- `eObjective`, `PedState`, `eMoveState`: intent, executor state, movement style.
- `CPedAttractor` and manager: world-owned interaction sites.
- `CPathFind`: path graph owner.
- `CAutoPilot`: vehicle movement controller.
- save/load entry points.

Why this syntax:

- `eObjective|PedState|eMoveState` finds objective, executor state, and movement
  intensity
- `CPed|CPedType|CPedStats` finds live actor and static profile data
- `CPedAttractor|CPedAttractorManager` finds interaction-site ownership
- `CPathFind|CAutoPilot` finds path graph and special movement controller
- `SetObjective|ProcessObjective|SetFollowPath|SetNewAttraction` finds the data
  flow from goal to state/path/interaction
- `LoadSavedGame|Save\(` finds persistence boundaries

Good follow-up:

```bash
nl -ba /Users/kogaryu/iggy/re3-miami/src/peds/Ped.h | sed -n '185,375p'
nl -ba /Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp | sed -n '760,840p'
nl -ba /Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.h | sed -n '20,95p'
```
