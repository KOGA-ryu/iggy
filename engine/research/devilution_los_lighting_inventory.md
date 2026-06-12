# DevilutionX LOS / Lighting Inventory

Source inspected:
- DevilutionX-master/Source/lighting.cpp
- DevilutionX-master/Source/lighting.h
- DevilutionX-master/Source/vision.cpp
- DevilutionX-master/Source/vision.hpp
- DevilutionX-master/Source/missiles.cpp
- DevilutionX-master/Source/missiles.h
- DevilutionX-master/Source/monster.cpp
- engine/src/modules/line_of_sight

Useful names:
- DoVision
- DoUnVision
- DoVisionFlags
- AddLight
- ChangeLightXY
- ChangeLightOffset
- ChangeLightRadius
- VisionList
- LightList
- dLight
- dPreLight
- DungeonFlag::Visible
- DungeonFlag::Lit
- LineClear
- IsLineNotSolid
- LineClearMissile
- LineClearMovingMissile

Ownership boundary:
- LOS belongs in modules/line_of_sight.
- Light presentation belongs in render or effects later.
- Tile transparency/solidity comes from level data.

Take:
- line-clear query as a pure tile predicate.
- vision radius query over level bounds.
- separate visibility from lighting.
- explicit missile line-clear naming.

Skip:
- global dungeon arrays.
- palette/infravision tables.
- renderer lighting implementation.
- monster-specific callers.

Defer:
- dynamic light list.
- automap exploration flags.
- missile-specific collision rules.

Next local target:
- engine/src/modules/line_of_sight/LineOfSight2D.*
- engine/tests/line_of_sight_tests.cpp
