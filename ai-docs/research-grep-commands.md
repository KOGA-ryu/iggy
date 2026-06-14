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

Good follow-up:

```bash
nl -ba /Users/kogaryu/iggy/re3-miami/src/peds/Ped.h | sed -n '185,375p'
nl -ba /Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp | sed -n '760,840p'
nl -ba /Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.h | sed -n '20,95p'
```

