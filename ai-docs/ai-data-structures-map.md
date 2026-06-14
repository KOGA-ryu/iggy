# AI Data Structures Map

This document lists the important AI data containers and what kind of state
they appear to own.

## OpenXcom

- `Unit`: static unit definition.
  - `/Users/kogaryu/iggy/OpenXcom-master/src/Mod/Unit.h:53`
- `BattleUnit`: live actor, position, faction, status, stats, inventory, AI.
  - `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/BattleUnit.h:55`
- `AIModule`: attached decision state and candidate action scratch.
  - `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.h:40`
- `BattleAction`: short-lived action proposal.
  - `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.h:43`
- `SavedBattleGame`: battle map/session owner.
  - `/Users/kogaryu/iggy/OpenXcom-master/src/Savegame/SavedBattleGame.h:42`

## Warzone 2100

- `DROID`: live unit, current order, current action, movement state, group.
  - `/Users/kogaryu/iggy/warzone2100-master/src/droiddef.h:101`
- `DroidOrder`: durable order payload.
  - `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:41`
- `DROID_ORDER`: order enum vocabulary.
  - `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:44`
- `DROID_ACTION`: current action enum vocabulary.
  - `/Users/kogaryu/iggy/warzone2100-master/src/actiondef.h:27`
- `MOVE_CONTROL`: path/motion scratch owned by live movement.
  - `/Users/kogaryu/iggy/warzone2100-master/src/movedef.h`
- `DROID_GROUP`: group/formation ownership.
  - `/Users/kogaryu/iggy/warzone2100-master/src/group.h:40`

## NetHack

- `permonst`: static monster definition.
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/permonst.h:56`
- `mons`: global static monster table.
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/permonst.h:82`
- `monst`: live monster state.
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:96`
- `mextra`: optional extension state for special monster roles.
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mextra.h:205`
- `mfndposdata`: movement candidate/scratch data.
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mfndpos.h:33`
- `monsters[COLNO][ROWNO]`: map occupancy grid.
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/rm.h:476`
- `monlist`: level monster list.
  - `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/rm.h:479`

## DevilutionX

- `MonsterData`: static monster data.
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/tables/monstdat.h:98`
- `UniqueMonsterData`: static unique monster data.
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/tables/monstdat.h:322`
- `MonsterMode`: active executor/mode enum.
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:75`
- `MonsterGoal`: goal/intent enum.
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h:120`
- `Monster`: live monster actor.
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.h`
- `AiProc`: behavior dispatch table.
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:3091`
- `FindPath`: reusable path search helper.
  - `/Users/kogaryu/iggy/DevilutionX-master/Source/engine/path.h:39`

## KeeperFX

- `Thing`: live world object record.
  - `/Users/kogaryu/iggy/keeperfx-master/src/thing_data.h:120`
- `CreatureControl`: creature-specific control/AI extension.
  - `/Users/kogaryu/iggy/keeperfx-master/src/creature_control.h:72`
- `CreatureModelConfig`: static creature definition.
  - `/Users/kogaryu/iggy/keeperfx-master/src/config_creature.h:74`
- `CreatureJobConfig`: static job/work behavior data.
  - `/Users/kogaryu/iggy/keeperfx-master/src/config_creature.h:236`
- `CreatureStates`: state enum vocabulary.
  - `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.h:36`
- `Room`: work site / owned room state.
  - `/Users/kogaryu/iggy/keeperfx-master/src/room_data.h:50`
- `Dungeon`: player/faction/session state.
  - `/Users/kogaryu/iggy/keeperfx-master/src/dungeon_data.h:143`
- `ComputerTask`, `Computer2`: director AI task state.
  - `/Users/kogaryu/iggy/keeperfx-master/src/player_computer.h:57`
  - `/Users/kogaryu/iggy/keeperfx-master/src/player_computer.h:182`

## re3 Miami

- `eObjective`: durable ped objective vocabulary.
  - `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:190`
- `PedState`: active behavior state vocabulary.
  - `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:280`
- `eMoveState`: movement style/intensity vocabulary.
  - `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:353`
- `CPed`: live actor.
  - `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:367`
- `CPedType`: profile/threat data.
  - `/Users/kogaryu/iggy/re3-miami/src/peds/PedType.h:63`
- `CPedStats`: stat profile data.
  - `/Users/kogaryu/iggy/re3-miami/src/peds/PedType.h:155`
- `CPedAttractorManager`, `CPedAttractor`: world-owned interaction sites.
  - `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.h:28`
  - `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.h:78`
- `CPathFind`: path graph owner.
  - `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.h:203`
- `CAutoPilot`: vehicle movement controller.
  - `/Users/kogaryu/iggy/re3-miami/src/control/AutoPilot.h:62`

## Commands Used

The searches in `/Users/kogaryu/iggy/ai-docs/research-grep-commands.md` found
these data structures by combining type names, state enums, update verbs, and
save/load terms.

