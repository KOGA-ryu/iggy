# AI Turn Pipeline Walkthroughs

This document follows the update path: who gets called, what kind of decision
is produced, and who executes it.

## OpenXcom

Pipeline:

```text
BattlescapeGame::think
  -> BattleUnit::think
  -> AIModule::think
  -> BattleAction
  -> BattlescapeGame pushes walk/attack/use state
```

Key behavior:

- AI fills a `BattleAction`.
- `BA_RETHINK` asks for another decision.
- `BA_WALK` is converted into path/walk execution by the runtime.
- attack/use actions are converted into specific execution states.

Useful anchors:

- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.cpp:226`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.cpp:229`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.cpp:231`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.cpp:255`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/BattlescapeGame.cpp:270`
- `/Users/kogaryu/iggy/OpenXcom-master/src/Battlescape/AIModule.cpp:137`

## Warzone 2100

Pipeline:

```text
updateDroid
  -> orderUpdateDroid
  -> actionUpdateDroid
  -> moveUpdateDroid
```

Key behavior:

- order update interprets durable intent
- action update advances current executor behavior
- movement update advances path/motion state
- current action can temporarily differ from the durable order

Useful anchors:

- `/Users/kogaryu/iggy/warzone2100-master/src/droid.cpp:927`
- `/Users/kogaryu/iggy/warzone2100-master/src/droid.cpp:950`
- `/Users/kogaryu/iggy/warzone2100-master/src/order.h:43`
- `/Users/kogaryu/iggy/warzone2100-master/src/action.h:65`
- `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:2165`

## NetHack

Pipeline:

```text
monster turn dispatcher
  -> status/special handling
  -> movement candidate generation
  -> attack/item/spell/pet/special branch
  -> map/list state update
```

Key behavior:

- monster definitions drive capabilities through flags and stats
- live `monst` state stores current position, status, target memory, and extras
- candidate movement uses scratch structures before committing a move
- special cases plug into the general monster turn rather than living in one
  clean class hierarchy

Useful anchors:

- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/permonst.h:56`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/monst.h:96`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/mfndpos.h:33`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/mhitu.c:491`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/mhitm.c:106`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/muse.c:441`
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/dog.c:691`

## DevilutionX

Pipeline:

```text
ProcessMonsters
  -> per-monster eligibility/state checks
  -> target/path/visibility helpers
  -> AiPlanPath
  -> AiProc[ai_id]
  -> mode/animation/movement/combat update
```

Key behavior:

- static monster AI id selects behavior dispatch
- `MonsterGoal` and `MonsterMode` split intent from current executor mode
- path and LOS helpers are queried during the monster phase
- save/load persists monster state, while path/visibility work is runtime work

Useful anchors:

- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4257`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:4322`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:3091`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1861`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/monster.cpp:1596`
- `/Users/kogaryu/iggy/DevilutionX-master/Source/engine/path.cpp:184`

## KeeperFX

Pipeline:

```text
thing/creature update
  -> active creature state
  -> process_func_list[state]
  -> job/room/movement/task logic
  -> cleanup or state transition
```

Key behavior:

- live `Thing` data is extended by `CreatureControl`
- creature state functions run through a dispatch table
- room/job systems provide world-owned work targets
- state transition functions centralize whether a creature can change state
- computer keeper tasks operate as a separate director layer

Useful anchors:

- `/Users/kogaryu/iggy/keeperfx-master/src/thing_creature.c:2579`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:309`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:449`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:4839`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_states.c:4924`
- `/Users/kogaryu/iggy/keeperfx-master/src/creature_jobs.c:691`

## re3 Miami

Pipeline:

```text
CPed update
  -> ProcessObjective
  -> objective-specific branch
  -> SetFollowPath / SetMoveState / SetNewAttraction / action state
  -> movement, animation, vehicle, or interaction update
```

Key behavior:

- objectives are durable "why"
- ped states are active "what"
- move states are movement intensity/style
- attractors assign world interaction destinations
- vehicle autopilot is a separate movement controller

Useful anchors:

- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp:777`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp:131`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:6208`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:9361`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.h:28`
- `/Users/kogaryu/iggy/re3-miami/src/control/AutoPilot.h:62`

## Commands Used

See `/Users/kogaryu/iggy/ai-docs/research-grep-commands.md`. The strongest
patterns came from searching for update verbs, objective/order/action names,
and save/load boundaries together.

