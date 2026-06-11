cmake_minimum_required(VERSION 3.20)

project(PlayerMovementSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(player_movement_system
  src/actions/ActionExecutor.cpp
  src/actions/ActionRules.cpp
  src/app/GameLoop.cpp
  src/app/RuntimeDebugArtifactBundle.cpp
  src/app/RuntimeExitCodePolicy.cpp
  src/app/RuntimeFrameTrace.cpp
  src/app/RuntimeFrameTraceFileStore.cpp
  src/app/RuntimeInputRouter.cpp
  src/app/RuntimeOutputFinalizer.cpp
  src/app/RuntimeRawInputDrainer.cpp
  src/app/RuntimeRunRecorder.cpp
  src/app/RuntimeSetupRunner.cpp
  src/app/RuntimeSourceDrainer.cpp
  src/app/RuntimeTraceService.cpp
  src/combat/CombatEventRecorder.cpp
  src/combat/CombatRegistry.cpp
  src/combat/CombatResolver.cpp
  src/combat/CombatSystem.cpp
  src/commands/CommandDispatcher.cpp
  src/commands/CommandQueue.cpp
  src/commands/IntentCommandBuilder.cpp
  src/commands/MovementCommandSource.cpp
  src/effects/EffectApplier.cpp
  src/effects/EffectRecorder.cpp
  src/effects/EffectRouter.cpp
  src/enemies/EnemyMovement.cpp
  src/events/EventRecorder.cpp
  src/focus/InputFocus.cpp
  src/input/InputMapper.cpp
  src/input/RawInputSource.cpp
  src/inventory/InventoryCommandCodec.cpp
  src/inventory/InventoryCommandDispatcher.cpp
  src/inventory/InventoryCommandLog.cpp
  src/inventory/InventoryCommandLogCodec.cpp
  src/inventory/InventoryCommandLogFileStore.cpp
  src/inventory/InventoryCommandReplayer.cpp
  src/inventory/InventoryCommandSource.cpp
  src/inventory/InventoryEventRecorder.cpp
  src/inventory/InventoryScriptRunner.cpp
  src/inventory/InventoryScriptSource.cpp
  src/inventory/EquipmentService.cpp
  src/inventory/EquipmentStatsService.cpp
  src/inventory/InventoryService.cpp
  src/interaction/InteractionCommandBuilder.cpp
  src/interaction/InteractionIntentBuilder.cpp
  src/network/MovementCodec.cpp
  src/player/PlayerActionGate.cpp
  src/player/PlayerController.cpp
  src/player/PlayerMovement.cpp
  src/player/WalkPath.cpp
  src/replay/CommandLog.cpp
  src/replay/CommandReplayer.cpp
  src/save/SaveGameService.cpp
  src/save/SaveSlotService.cpp
  src/save/SnapshotCodec.cpp
  src/save/SnapshotFileStore.cpp
  src/save/SnapshotReader.cpp
  src/save/SnapshotWriter.cpp
  src/session/SessionCommandCodec.cpp
  src/session/SessionCommandDispatcher.cpp
  src/session/SessionCommandLog.cpp
  src/session/SessionCommandLogCodec.cpp
  src/session/SessionCommandLogFileStore.cpp
  src/session/SessionCommandReplayer.cpp
  src/session/SessionCommandSource.cpp
  src/session/SessionEventRecorder.cpp
  src/session/SessionScriptRunner.cpp
  src/session/GameSession.cpp
  src/simulation/SimulationClock.cpp
  src/simulation/SimulationFrameEvents.cpp
  src/simulation/SimulationFramePolicy.cpp
  src/simulation/SimulationFrameRunner.cpp
  src/simulation/SimulationTimeStep.cpp
  src/simulation/SimulationTick.cpp
  src/simulation/SimulationWorld.cpp
  src/simulation/WorldEntityService.cpp
  src/targeting/TargetRegistry.cpp
  src/targeting/TargetResolver.cpp
  src/targeting/TargetSynchronizer.cpp
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
