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
  src/modules/npc_ai/NpcAgentBatchUpdater.cpp
  src/modules/npc_ai/NpcAgentController.cpp
  src/modules/npc_ai/NpcAgentRenderCommands.cpp
  src/modules/npc_ai/NpcBrainTick.cpp
  src/modules/npc_ai/NpcIntentSelector.cpp
  src/modules/npc_ai/NpcMovementPlanner.cpp
  src/modules/npc_ai/NpcNavigationController.cpp
  src/modules/npc_ai/NpcSpawnBuilder.cpp
  src/modules/npc_ai/NpcTickReporter.cpp
  src/runtime/RuntimeTick.cpp
  src/runtime/RuntimeTickRunner.cpp
  src/scene/camera/CameraClamp.cpp
  src/scene/camera/CameraFollow.cpp
  src/scene/camera/CameraLookahead.cpp
  src/scene/camera/CameraRig.cpp
  src/scene/camera/CameraShake.cpp
  src/scene/camera/CameraView.cpp
  src/scene/level/LevelBlueprint.cpp
  src/scene/level/LevelNpcUpdateStep.cpp
  src/scene/level/LevelRenderFrame2D.cpp
  src/scene/level/LevelRuntimeBuilder.cpp
  src/scene/level/LevelRuntimeUpdate.cpp
  src/scene/level/LevelTileDrawList.cpp
  src/scene/level/LevelTileMap.cpp
  src/scene/level/LevelTileRenderCommands.cpp
  src/scene/level/LevelVisibleTiles.cpp
  src/servers/navigation/NavigationGridPathfinder.cpp
  src/servers/navigation/NavigationGridValidator.cpp
  src/servers/navigation/NavigationPathFollower.cpp
  src/servers/physics2d/ShapeQuery2D.cpp
  src/servers/render/RenderCommand2D.cpp
  src/servers/render/RenderCommandList2DComposer.cpp
)

target_include_directories(iggy_engine PUBLIC src)

enable_testing()

function(iggy_add_test name source)
  add_executable(${name} ${source})
  target_link_libraries(${name} PRIVATE iggy_engine)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

iggy_add_test(core_math_tests tests/core_math_tests.cpp)
iggy_add_test(core_resource_tests tests/core_resource_tests.cpp)
iggy_add_test(camera_clamp_tests tests/camera_clamp_tests.cpp)
iggy_add_test(camera_follow_tests tests/camera_follow_tests.cpp)
iggy_add_test(camera_lookahead_tests tests/camera_lookahead_tests.cpp)
iggy_add_test(camera_rig_tests tests/camera_rig_tests.cpp)
iggy_add_test(camera_shake_tests tests/camera_shake_tests.cpp)
iggy_add_test(camera_view_tests tests/camera_view_tests.cpp)
iggy_add_test(level_blueprint_tests tests/level_blueprint_tests.cpp)
iggy_add_test(level_tile_draw_list_tests tests/level_tile_draw_list_tests.cpp)
iggy_add_test(level_tile_map_tests tests/level_tile_map_tests.cpp)
iggy_add_test(level_tile_render_commands_tests tests/level_tile_render_commands_tests.cpp)
iggy_add_test(level_render_frame_2d_tests tests/level_render_frame_2d_tests.cpp)
iggy_add_test(level_runtime_builder_tests tests/level_runtime_builder_tests.cpp)
iggy_add_test(level_runtime_update_tests tests/level_runtime_update_tests.cpp)
iggy_add_test(level_npc_update_step_tests tests/level_npc_update_step_tests.cpp)
iggy_add_test(level_visible_tiles_tests tests/level_visible_tiles_tests.cpp)
iggy_add_test(line_of_sight_tests tests/line_of_sight_tests.cpp)
iggy_add_test(npc_awareness_tests tests/npc_awareness_tests.cpp)
iggy_add_test(npc_agent_controller_tests tests/npc_agent_controller_tests.cpp)
iggy_add_test(npc_agent_batch_tests tests/npc_agent_batch_tests.cpp)
iggy_add_test(npc_agent_render_commands_tests tests/npc_agent_render_commands_tests.cpp)
iggy_add_test(npc_brain_tick_tests tests/npc_brain_tick_tests.cpp)
iggy_add_test(npc_intent_tests tests/npc_intent_tests.cpp)
iggy_add_test(npc_movement_planner_tests tests/npc_movement_planner_tests.cpp)
iggy_add_test(npc_navigation_controller_tests tests/npc_navigation_controller_tests.cpp)
iggy_add_test(npc_spawn_builder_tests tests/npc_spawn_builder_tests.cpp)
iggy_add_test(npc_tick_report_tests tests/npc_tick_report_tests.cpp)
iggy_add_test(navigation_request_tests tests/navigation_request_tests.cpp)
iggy_add_test(navigation_pathfinder_tests tests/navigation_pathfinder_tests.cpp)
iggy_add_test(navigation_path_follower_tests tests/navigation_path_follower_tests.cpp)
iggy_add_test(physics2d_tests tests/physics2d_tests.cpp)
iggy_add_test(render_command_2d_tests tests/render_command_2d_tests.cpp)
iggy_add_test(render_command_list_2d_composer_tests tests/render_command_list_2d_composer_tests.cpp)
iggy_add_test(runtime_tick_tests tests/runtime_tick_tests.cpp)
iggy_add_test(runtime_tick_runner_tests tests/runtime_tick_runner_tests.cpp)
