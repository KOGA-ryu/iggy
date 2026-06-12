cmake_minimum_required(VERSION 3.20)

project(IggyEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(iggy_engine
  src/core/math/Aabb2.cpp
  src/core/math/Ray2.cpp
  src/core/math/Rect2.cpp
  src/core/math/Vec2.cpp
  src/core/resource/ResourceId.cpp
  src/modules/blueprint/LevelBlueprintValidator.cpp
  src/modules/line_of_sight/LineOfSight.cpp
  src/modules/npc_ai/AwarenessSensor.cpp
  src/modules/npc_ai/NpcIntentSelector.cpp
  src/modules/npc_ai/NpcMovementPlanner.cpp
  src/scene/level/LevelBlueprint.cpp
  src/scene/level/LevelTileMap.cpp
  src/servers/navigation/NavigationGridPathfinder.cpp
  src/servers/navigation/NavigationGridValidator.cpp
  src/servers/physics2d/ShapeQuery2D.cpp
)

target_include_directories(iggy_engine PUBLIC src)

enable_testing()

add_executable(core_math_tests tests/core_math_tests.cpp)
target_link_libraries(core_math_tests PRIVATE iggy_engine)

add_executable(core_resource_tests tests/core_resource_tests.cpp)
target_link_libraries(core_resource_tests PRIVATE iggy_engine)

add_executable(level_blueprint_tests tests/level_blueprint_tests.cpp)
target_link_libraries(level_blueprint_tests PRIVATE iggy_engine)

add_executable(level_tile_map_tests tests/level_tile_map_tests.cpp)
target_link_libraries(level_tile_map_tests PRIVATE iggy_engine)

add_executable(line_of_sight_tests tests/line_of_sight_tests.cpp)
target_link_libraries(line_of_sight_tests PRIVATE iggy_engine)

add_executable(npc_awareness_tests tests/npc_awareness_tests.cpp)
target_link_libraries(npc_awareness_tests PRIVATE iggy_engine)

add_executable(npc_intent_tests tests/npc_intent_tests.cpp)
target_link_libraries(npc_intent_tests PRIVATE iggy_engine)

add_executable(npc_movement_planner_tests tests/npc_movement_planner_tests.cpp)
target_link_libraries(npc_movement_planner_tests PRIVATE iggy_engine)

add_executable(navigation_request_tests tests/navigation_request_tests.cpp)
target_link_libraries(navigation_request_tests PRIVATE iggy_engine)

add_executable(navigation_pathfinder_tests tests/navigation_pathfinder_tests.cpp)
target_link_libraries(navigation_pathfinder_tests PRIVATE iggy_engine)

add_executable(physics2d_tests tests/physics2d_tests.cpp)
target_link_libraries(physics2d_tests PRIVATE iggy_engine)

add_test(NAME core_math_tests COMMAND core_math_tests)
add_test(NAME core_resource_tests COMMAND core_resource_tests)
add_test(NAME level_blueprint_tests COMMAND level_blueprint_tests)
add_test(NAME level_tile_map_tests COMMAND level_tile_map_tests)
add_test(NAME line_of_sight_tests COMMAND line_of_sight_tests)
add_test(NAME npc_awareness_tests COMMAND npc_awareness_tests)
add_test(NAME npc_intent_tests COMMAND npc_intent_tests)
add_test(NAME npc_movement_planner_tests COMMAND npc_movement_planner_tests)
add_test(NAME navigation_request_tests COMMAND navigation_request_tests)
add_test(NAME navigation_pathfinder_tests COMMAND navigation_pathfinder_tests)
add_test(NAME physics2d_tests COMMAND physics2d_tests)
