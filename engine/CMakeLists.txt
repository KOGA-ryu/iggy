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
  src/scene/level/LevelBlueprint.cpp
  src/scene/level/LevelTileMap.cpp
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

add_executable(physics2d_tests tests/physics2d_tests.cpp)
target_link_libraries(physics2d_tests PRIVATE iggy_engine)

add_test(NAME core_math_tests COMMAND core_math_tests)
add_test(NAME core_resource_tests COMMAND core_resource_tests)
add_test(NAME level_blueprint_tests COMMAND level_blueprint_tests)
add_test(NAME level_tile_map_tests COMMAND level_tile_map_tests)
add_test(NAME physics2d_tests COMMAND physics2d_tests)
