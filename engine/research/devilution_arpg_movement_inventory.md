# DevilutionX ARPG Movement Inventory

Source inspected:
- DevilutionX-master/Source/player.cpp
- DevilutionX-master/Source/player.h
- DevilutionX-master/Source/players
- DevilutionX-master/Source/monster.cpp
- DevilutionX-master/Source/engine/path.*
- player-movement-system/src/player
- player-movement-system/src/world

Useful names:
- position.tile
- position.future
- position.old
- position.temp
- walkpath
- PlayerWalkPathSizeForSaveGame
- Direction
- GetDirection
- WalkingDistance
- WalkInDirection
- MonsterWalk
- occupyTile
- FixPlrWalkTags
- pathCount

Ownership boundary:
- Actor position/state belongs with scene/entity or gameplay actor data.
- Tile path legality belongs with level/world constraints.
- Runtime movement stepping belongs outside blueprint data.

Take:
- tile/future/old position vocabulary.
- direction as first-class movement state.
- path as explicit short command/result data.
- occupancy update at step commit.

Skip:
- Diablo-specific animation modes.
- save-format path limits as runtime limits.
- global dPlayer/dMonster arrays.

Defer:
- diagonal corner policy.
- network command replay.
- monster movement coupling.

Next local target:
- engine/src/scene/entity/ActorPosition.*
- engine/src/scene/level/LevelTileMap.*
- engine/tests/actor_movement_tests.cpp
