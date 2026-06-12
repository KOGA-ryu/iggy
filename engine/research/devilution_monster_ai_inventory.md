# DevilutionX Monster AI Inventory

Source inspected:
- DevilutionX-master/Source/monster.cpp
- DevilutionX-master/Source/monster.h
- DevilutionX-master/Source/monsters
- DevilutionX-master/Source/players
- player-movement-system/src/enemies
- player-movement-system/src/combat

Useful names:
- Monster
- ActiveMonsters
- MonsterMode
- MonsterGoal
- MonsterAIID
- enemy
- enemyPosition
- UpdateEnemy
- AiDelay
- GetMonsterDirection
- StartRangedAttack
- StartRangedSpecialAttack
- M_StartStand
- MonsterAttackMonster
- IsRanged
- activeForTicks

Ownership boundary:
- AI state belongs in gameplay/enemy modules, not physics.
- Target selection can consume LOS and tile distance.
- Combat resolution should stay separate from AI choice.

Take:
- mode/goal split.
- enemy target position snapshot.
- delay/cooldown state.
- ranged attack requires line-clear query.

Skip:
- unique monster quest placement.
- hard-coded global monster arrays.
- Diablo-specific AI id table.
- animation side effects in AI decisions.

Defer:
- group leaders/minions.
- flee/scavenger variants.
- monster-vs-monster targeting.

Next local target:
- engine/src/scene/entity/EnemyState.*
- engine/src/modules/ai/EnemyAiStep.*
- engine/tests/enemy_ai_tests.cpp
