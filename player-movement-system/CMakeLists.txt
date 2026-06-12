cmake_minimum_required(VERSION 3.20)

project(PlayerMovementSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(player_movement_system
  src/actions/ActionExecutor.cpp
  src/actions/ActionRules.cpp
  src/app/GameLoop.cpp
  src/app/RuntimeArtifactOutputService.cpp
  src/app/RuntimeArtifactOutputPlan.cpp
  src/app/RuntimeArtifactOutputRequestRunner.cpp
  src/app/RuntimeDebugArtifactBundle.cpp
  src/app/RuntimeDebugArtifactBundleResultBuilder.cpp
  src/app/RuntimeDebugArtifactLayout.cpp
  src/app/RuntimeDebugArtifactRootPreparer.cpp
  src/app/RuntimeDebugArtifactWriter.cpp
  src/app/RuntimeDebugManifest.cpp
  src/app/RuntimeDebugManifestContextBuilder.cpp
  src/app/RuntimeDebugBundleOutputStep.cpp
  src/app/RuntimeDebugManifestIndexText.cpp
  src/app/RuntimeDebugManifestPathsText.cpp
  src/app/RuntimeDebugManifestSections.cpp
  src/app/RuntimeDebugManifestSetupText.cpp
  src/app/RuntimeDebugManifestWriteStep.cpp
  src/app/RuntimeDebugTraceWriteStep.cpp
  src/app/RuntimeBlockedPointerInputStep.cpp
  src/app/RuntimeCombatText.cpp
  src/app/RuntimeEffectText.cpp
  src/app/RuntimeExitCodeMapper.cpp
  src/app/RuntimeExitCodePolicy.cpp
  src/app/RuntimeFinalModeRecorder.cpp
  src/app/RuntimeFrameCompletionReportRecorder.cpp
  src/app/RuntimeFrameEventDeltaCollector.cpp
  src/app/RuntimeFrameEventReportRecorder.cpp
  src/app/RuntimeFrameLoopRunner.cpp
  src/app/RuntimeFramePolicyReportRecorder.cpp
  src/app/RuntimeFramePolicyResolver.cpp
  src/app/RuntimeFramePolicyText.cpp
  src/app/RuntimeFrameSimulationPhaseRunner.cpp
  src/app/RuntimeFrameSourcePhaseRunner.cpp
  src/app/RuntimeFrameRunner.cpp
  src/app/RuntimeFrameTrace.cpp
  src/app/RuntimeFrameTraceFileStore.cpp
  src/app/RuntimeFrameTraceHeaderText.cpp
  src/app/RuntimeFrameTraceSections.cpp
  src/app/RuntimeInputContextBuilder.cpp
  src/app/RuntimeInputDrainResultBuilder.cpp
  src/app/RuntimeInputDrainReportRecorder.cpp
  src/app/RuntimeInputFocusResolver.cpp
  src/app/RuntimeInputRouteResultBuilder.cpp
  src/app/RuntimeInputRouter.cpp
  src/app/RuntimeInputSourceRouter.cpp
  src/app/RuntimeInventoryFrameSourceStep.cpp
  src/app/RuntimeInventoryCommandIntake.cpp
  src/app/RuntimeMovementInputRouter.cpp
  src/app/RuntimeInventoryCommandReportRecorder.cpp
  src/app/RuntimeInventoryScriptReportRecorder.cpp
  src/app/RuntimeInventoryScriptBatchRunner.cpp
  src/app/RuntimeInventoryScriptIntake.cpp
  src/app/RuntimeInventoryText.cpp
  src/app/RuntimeInventoryScriptText.cpp
  src/app/RuntimeMovementCommandReportRecorder.cpp
  src/app/RuntimeMovementCommandIntake.cpp
  src/app/RuntimeMovementCommandQueueStep.cpp
  src/app/RuntimeMovementEventText.cpp
  src/app/RuntimeMovementFrameSourceStep.cpp
  src/app/RuntimeMovementInputContextBuilder.cpp
  src/app/RuntimeMovementInputBlockSummary.cpp
  src/app/RuntimeMovementIntentInputStep.cpp
  src/app/RuntimeMovementScriptReportRecorder.cpp
  src/app/RuntimeMovementScriptBatchRunner.cpp
  src/app/RuntimeMovementScriptIntake.cpp
  src/app/RuntimeMovementScriptText.cpp
  src/app/RuntimeOutputFailurePolicy.cpp
  src/app/RuntimeOutputFinalizer.cpp
  src/app/RuntimeOutputResultBuilder.cpp
  src/app/RuntimePlayerActionText.cpp
  src/app/RuntimeRawInputDrainer.cpp
  src/app/RuntimeRunExecutor.cpp
  src/app/RuntimeRunFinalizer.cpp
  src/app/RuntimeRunFailurePolicy.cpp
  src/app/RuntimeRunRecorder.cpp
  src/app/RuntimeRunTraceFrameHeaderText.cpp
  src/app/RuntimeRunTraceOutputStep.cpp
  src/app/RuntimeRunSummaryText.cpp
  src/app/RuntimeSessionCommandReportRecorder.cpp
  src/app/RuntimeSessionCommandIntake.cpp
  src/app/RuntimeSessionFrameSourceStep.cpp
  src/app/RuntimeSetupFrameGate.cpp
  src/app/RuntimeSetupInventoryCommandReportRecorder.cpp
  src/app/RuntimeSetupRunResultApplier.cpp
  src/app/RuntimeSetupRunner.cpp
  src/app/RuntimeSetupFailurePolicy.cpp
  src/app/RuntimeSimulationFrameUpdater.cpp
  src/app/RuntimeSessionInputRouter.cpp
  src/app/RuntimeSessionModeTogglePolicy.cpp
  src/app/RuntimeSessionText.cpp
  src/app/RuntimeSourceContext.cpp
  src/app/RuntimeSourceDrainer.cpp
  src/app/RuntimeSourceDrainerSettingsBuilder.cpp
  src/app/RuntimeStartupScriptIntake.cpp
  src/app/RuntimeStopMovementInputStep.cpp
  src/app/RuntimeTargetInteractionInputStep.cpp
  src/app/RuntimeTargetInputRouter.cpp
  src/app/RuntimeTraceService.cpp
  src/combat/CombatEventRecorder.cpp
  src/combat/CombatRegistry.cpp
  src/combat/CombatResolver.cpp
  src/combat/CombatSystem.cpp
  src/commands/CommandDispatcher.cpp
  src/commands/CommandQueue.cpp
  src/commands/IntentCommandBuilder.cpp
  src/commands/MovementCommandEventEmitter.cpp
  src/commands/MovementCommandSource.cpp
  src/commands/MovementCommandValidator.cpp
  src/effects/EffectApplier.cpp
  src/effects/EffectRecorder.cpp
  src/effects/EffectRouter.cpp
  src/enemies/EnemyAttackEventEmitter.cpp
  src/enemies/EnemyAttackEntryPolicy.cpp
  src/enemies/EnemyAttackPhaseRunner.cpp
  src/enemies/EnemyAttackRange.cpp
  src/enemies/EnemyAttackRestartPolicy.cpp
  src/enemies/EnemyAttackRunner.cpp
  src/enemies/EnemyMovement.cpp
  src/enemies/EnemyMovementReporter.cpp
  src/enemies/EnemyPursuitBudget.cpp
  src/enemies/EnemyPursuitEventEmitter.cpp
  src/enemies/EnemyPursuitStepGate.cpp
  src/enemies/EnemyPursuitStepPlanner.cpp
  src/enemies/EnemyPursuitStepper.cpp
  src/events/EventRecorder.cpp
  src/focus/InputFocus.cpp
  src/files/ByteFileStore.cpp
  src/files/TextFileStore.cpp
  src/input/InputEventMatcher.cpp
  src/input/InputMapper.cpp
  src/input/RawInputSource.cpp
  src/inventory/InventoryCommandCodec.cpp
  src/inventory/InventoryCommandByteStream.cpp
  src/inventory/InventoryCommandDispatcher.cpp
  src/inventory/InventoryCommandEventEmitter.cpp
  src/inventory/InventoryCommandLog.cpp
  src/inventory/InventoryCommandLogChecksum.cpp
  src/inventory/InventoryCommandLogCodec.cpp
  src/inventory/InventoryCommandLogFileStore.cpp
  src/inventory/InventoryCommandLogFrameCodec.cpp
  src/inventory/InventoryCommandPacketByteCodec.cpp
  src/inventory/InventoryCommandPacketListCodec.cpp
  src/inventory/InventoryCommandPacketValidator.cpp
  src/inventory/InventoryCommandReplayer.cpp
  src/inventory/InventoryCommandSource.cpp
  src/inventory/InventoryEventRecorder.cpp
  src/inventory/InventoryScriptRunner.cpp
  src/inventory/InventoryScriptSource.cpp
  src/inventory/EquipmentService.cpp
  src/inventory/EquipmentStatsService.cpp
  src/inventory/InventoryService.cpp
  src/interaction/DestinationActionBuilder.cpp
  src/interaction/InteractionCommandBuilder.cpp
  src/interaction/InteractionIntentBuilder.cpp
  src/network/MovementCodec.cpp
  src/player/ActorStepCommitter.cpp
  src/player/PlayerAnimationLockGate.cpp
  src/player/PlayerActionRunner.cpp
  src/player/PlayerActionGate.cpp
  src/player/PlayerController.cpp
  src/player/PlayerMovement.cpp
  src/player/PlayerPathPlanner.cpp
  src/player/PlayerPathStepper.cpp
  src/player/WalkPath.cpp
  src/replay/CommandLog.cpp
  src/replay/CommandLogChecksum.cpp
  src/replay/CommandLogCodec.cpp
  src/replay/CommandLogFileStore.cpp
  src/replay/CommandLogFrameCodec.cpp
  src/replay/CommandPacketListCodec.cpp
  src/replay/CommandReplayReport.cpp
  src/replay/CommandReplayer.cpp
  src/replay/MovementScriptRunner.cpp
  src/replay/MovementScriptSource.cpp
  src/save/SnapshotByteStream.cpp
  src/save/SnapshotChecksum.cpp
  src/save/SnapshotFrameCodec.cpp
  src/save/SnapshotEntityCodec.cpp
  src/save/SnapshotEnemyCodec.cpp
  src/save/SnapshotPlayerCodec.cpp
  src/save/SnapshotSchemaCodec.cpp
  src/save/SaveGameService.cpp
  src/save/SaveSlotService.cpp
  src/save/SnapshotCodec.cpp
  src/save/SnapshotFileStore.cpp
  src/save/SnapshotReader.cpp
  src/save/SnapshotWriter.cpp
  src/session/SessionCommandByteStream.cpp
  src/session/SessionCommandCodec.cpp
  src/session/SessionCommandDispatcher.cpp
  src/session/SessionCommandApplier.cpp
  src/session/SessionCommandLog.cpp
  src/session/SessionCommandLogChecksum.cpp
  src/session/SessionCommandLogCodec.cpp
  src/session/SessionCommandLogFileStore.cpp
  src/session/SessionCommandLogFrameCodec.cpp
  src/session/SessionCommandPacketByteCodec.cpp
  src/session/SessionCommandPacketListCodec.cpp
  src/session/SessionCommandPacketValidator.cpp
  src/session/SessionCommandReplayer.cpp
  src/session/SessionCommandSource.cpp
  src/session/SessionEventEmitter.cpp
  src/session/SessionEventRecorder.cpp
  src/session/SessionFrameUpdater.cpp
  src/session/NewGameWorldBuilder.cpp
  src/session/SessionModeChanger.cpp
  src/session/SessionModePolicy.cpp
  src/session/SessionScriptRunner.cpp
  src/session/SessionWorldSlotLoader.cpp
  src/session/SessionWorldSlotSaver.cpp
  src/session/GameSession.cpp
  src/simulation/SimulationActorUpdater.cpp
  src/simulation/SimulationClock.cpp
  src/simulation/SimulationCommandDrainer.cpp
  src/simulation/SimulationCommandQueueDrainStep.cpp
  src/simulation/SimulationEnemyMovementRunner.cpp
  src/simulation/SimulationEnemyTargetSelector.cpp
  src/simulation/SimulationEnemyUpdater.cpp
  src/simulation/SimulationEffectFinalizer.cpp
  src/simulation/SimulationEffectPipeline.cpp
  src/simulation/SimulationFrameEventCapture.cpp
  src/simulation/SimulationFrameFinalizer.cpp
  src/simulation/SimulationFrameEvents.cpp
  src/simulation/SimulationFramePolicyDescriber.cpp
  src/simulation/SimulationFramePolicy.cpp
  src/simulation/SimulationFrameRunner.cpp
  src/simulation/SimulationFrameTickRunner.cpp
  src/simulation/SimulationInventoryFinalizer.cpp
  src/simulation/SimulationPlayerMovementRunner.cpp
  src/simulation/SimulationPlayerUpdater.cpp
  src/simulation/SimulationTargetFinalizer.cpp
  src/simulation/SimulationTimeStep.cpp
  src/simulation/SimulationTimeStepBuilder.cpp
  src/simulation/SimulationTick.cpp
  src/simulation/SimulationTickPipeline.cpp
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

add_executable(artifact_output_tests tests/artifact_output_tests.cpp)
target_link_libraries(artifact_output_tests PRIVATE player_movement_system)

add_executable(artifact_text_tests tests/artifact_text_tests.cpp)
target_link_libraries(artifact_text_tests PRIVATE player_movement_system)

add_executable(input_routing_tests tests/input_routing_tests.cpp)
target_link_libraries(input_routing_tests PRIVATE player_movement_system)

add_executable(input_drain_tests tests/input_drain_tests.cpp)
target_link_libraries(input_drain_tests PRIVATE player_movement_system)

add_executable(command_drain_tests tests/command_drain_tests.cpp)
target_link_libraries(command_drain_tests PRIVATE player_movement_system)

add_executable(source_drain_tests tests/source_drain_tests.cpp)
target_link_libraries(source_drain_tests PRIVATE player_movement_system)

add_executable(frame_source_tests tests/frame_source_tests.cpp)
target_link_libraries(frame_source_tests PRIVATE player_movement_system)

add_executable(frame_simulation_tests tests/frame_simulation_tests.cpp)
target_link_libraries(frame_simulation_tests PRIVATE player_movement_system)

add_executable(frame_lifecycle_tests tests/frame_lifecycle_tests.cpp)
target_link_libraries(frame_lifecycle_tests PRIVATE player_movement_system)

add_executable(run_result_policy_tests tests/run_result_policy_tests.cpp)
target_link_libraries(run_result_policy_tests PRIVATE player_movement_system)

add_executable(game_loop_output_tests tests/game_loop_output_tests.cpp)
target_link_libraries(game_loop_output_tests PRIVATE player_movement_system)

add_executable(game_loop_command_source_tests tests/game_loop_command_source_tests.cpp)
target_link_libraries(game_loop_command_source_tests PRIVATE player_movement_system)

add_executable(game_loop_script_source_tests tests/game_loop_script_source_tests.cpp)
target_link_libraries(game_loop_script_source_tests PRIVATE player_movement_system)

add_executable(game_loop_raw_input_tests tests/game_loop_raw_input_tests.cpp)
target_link_libraries(game_loop_raw_input_tests PRIVATE player_movement_system)

add_executable(game_loop_frame_report_tests tests/game_loop_frame_report_tests.cpp)
target_link_libraries(game_loop_frame_report_tests PRIVATE player_movement_system)

add_executable(runtime_frame_trace_tests tests/runtime_frame_trace_tests.cpp)
target_link_libraries(runtime_frame_trace_tests PRIVATE player_movement_system)

add_executable(runtime_debug_manifest_tests tests/runtime_debug_manifest_tests.cpp)
target_link_libraries(runtime_debug_manifest_tests PRIVATE player_movement_system)

add_executable(runtime_trace_persistence_tests tests/runtime_trace_persistence_tests.cpp)
target_link_libraries(runtime_trace_persistence_tests PRIVATE player_movement_system)

add_executable(runtime_debug_artifact_tests tests/runtime_debug_artifact_tests.cpp)
target_link_libraries(runtime_debug_artifact_tests PRIVATE player_movement_system)

add_test(NAME movement_tests COMMAND movement_tests)
add_test(NAME artifact_output_tests COMMAND artifact_output_tests)
add_test(NAME artifact_text_tests COMMAND artifact_text_tests)
add_test(NAME input_routing_tests COMMAND input_routing_tests)
add_test(NAME input_drain_tests COMMAND input_drain_tests)
add_test(NAME command_drain_tests COMMAND command_drain_tests)
add_test(NAME source_drain_tests COMMAND source_drain_tests)
add_test(NAME frame_source_tests COMMAND frame_source_tests)
add_test(NAME frame_simulation_tests COMMAND frame_simulation_tests)
add_test(NAME frame_lifecycle_tests COMMAND frame_lifecycle_tests)
add_test(NAME run_result_policy_tests COMMAND run_result_policy_tests)
add_test(NAME game_loop_output_tests COMMAND game_loop_output_tests)
add_test(NAME game_loop_command_source_tests COMMAND game_loop_command_source_tests)
add_test(NAME game_loop_script_source_tests COMMAND game_loop_script_source_tests)
add_test(NAME game_loop_raw_input_tests COMMAND game_loop_raw_input_tests)
add_test(NAME game_loop_frame_report_tests COMMAND game_loop_frame_report_tests)
add_test(NAME runtime_frame_trace_tests COMMAND runtime_frame_trace_tests)
add_test(NAME runtime_debug_manifest_tests COMMAND runtime_debug_manifest_tests)
add_test(NAME runtime_trace_persistence_tests COMMAND runtime_trace_persistence_tests)
add_test(NAME runtime_debug_artifact_tests COMMAND runtime_debug_artifact_tests)
