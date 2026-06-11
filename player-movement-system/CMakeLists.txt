cmake_minimum_required(VERSION 3.20)

project(PlayerMovementSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(player_movement_system
  src/actions/ActionExecutor.cpp
  src/actions/ActionRules.cpp
  src/app/GameLoop.cpp
  src/combat/CombatEventRecorder.cpp
  src/combat/CombatRegistry.cpp
  src/combat/CombatResolver.cpp
  src/combat/CombatSystem.cpp
  src/commands/CommandDispatcher.cpp
  src/commands/CommandQueue.cpp
  src/commands/IntentCommandBuilder.cpp
  src/enemies/EnemyMovement.cpp
  src/events/EventRecorder.cpp
  src/focus/InputFocus.cpp
  src/input/InputMapper.cpp
  src/interaction/InteractionCommandBuilder.cpp
  src/interaction/InteractionIntentBuilder.cpp
  src/network/MovementCodec.cpp
  src/player/PlayerActionGate.cpp
  src/player/PlayerController.cpp
  src/player/PlayerMovement.cpp
  src/player/WalkPath.cpp
  src/replay/CommandLog.cpp
  src/replay/CommandReplayer.cpp
  src/simulation/SimulationFramePolicy.cpp
  src/simulation/SimulationTick.cpp
  src/simulation/SimulationWorld.cpp
  src/targeting/TargetResolver.cpp
  src/world/Collision.cpp
  src/world/DiagonalCornerPolicy.cpp
  src/world/PathFinder.cpp
  src/world/TileMap.cpp
)

target_include_directories(player_movement_system PUBLIC src)

add_executable(player_movement_demo src/main.cpp)
target_link_libraries(player_movement_demo PRIVATE player_movement_system)

enable_testing()

add_executable(movement_tests tests/movement_tests.cpp)
target_link_libraries(movement_tests PRIVATE player_movement_system)

add_test(NAME movement_tests COMMAND movement_tests)
