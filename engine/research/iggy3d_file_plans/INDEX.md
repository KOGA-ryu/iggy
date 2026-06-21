# iggy3d Complete File Plan Index

Updated: 2026-06-20

This directory is the file-by-file construction contract for the standalone
`iggy3d` repo at `/Users/kogaryu/iggy3d`.

One planned repo file maps to one planning document. The complete build surface
is tracked in `COMPLETE_BUILD_SURFACE.md`; fill/build order is tracked in
`PRIORITY.md`.

Hard rule: these plans are for the fresh standalone repo. They must not depend
on old `/Users/kogaryu/iggy` build targets, headers, scenes, runtime files, or
native app code.

## Project Docs

- `COMPLETE_BUILD_SURFACE.md`
- `docs_architecture_md.md`
- `docs_ownership_md.md`
- `docs_roadmap_md.md`
- `docs_acceptance_demo_md.md`

## Build Files

- `CMakeLists_txt.md`
- `cmake_iggy3d_options_cmake.md`
- `cmake_iggy3d_warnings_cmake.md`
- `cmake_iggy3d_tests_cmake.md`

## Core

- `src_core_result_Result_hpp.md`
- `src_core_diagnostics_Diagnostic_hpp.md`
- `src_core_ids_EntityId_hpp.md`
- `src_core_hash_StableHash_hpp.md`
- `src_core_hash_StableHash_cpp.md`
- `src_core_math_Vec3_hpp.md`
- `src_core_math_Vec3_cpp.md`
- `src_core_math_Ray3_hpp.md`
- `src_core_math_Ray3_cpp.md`
- `src_core_math_Aabb3_hpp.md`
- `src_core_math_Aabb3_cpp.md`
- `src_core_math_Plane_hpp.md`
- `src_core_math_Plane_cpp.md`
- `src_core_math_Mat4_hpp.md`
- `src_core_math_Mat4_cpp.md`
- `src_core_math_Transform3_hpp.md`
- `src_core_math_Transform3_cpp.md`

## Config And App Support

- `src_config_RuntimeConfig_hpp.md`
- `src_app_AppConfig_hpp.md`
- `src_app_AppConfig_cpp.md`
- `src_app_CliParser_hpp.md`
- `src_app_CliParser_cpp.md`

## Content And Fixtures

- `src_content_PackageManifest_hpp.md`
- `src_content_PackageLoader_hpp.md`
- `src_content_PackageLoader_cpp.md`
- `src_content_PackageValidator_hpp.md`
- `src_content_PackageValidator_cpp.md`
- `src_content_FixtureScenarioLoader_hpp.md`
- `src_content_FixtureScenarioLoader_cpp.md`
- `fixtures_demos_first_room_package_iggy3d_toml.md`
- `fixtures_demos_first_room_scenario_iggy3d_toml.md`
- `fixtures_demos_first_room_expected_summary_txt.md`

## Runtime: Player And World

- `src_runtime_player_PlayerSlot_hpp.md`
- `src_runtime_player_PlayerRoster_hpp.md`
- `src_runtime_player_PlayerRoster_cpp.md`
- `src_runtime_world_EntityState_hpp.md`
- `src_runtime_world_WorldState_hpp.md`
- `src_runtime_world_WorldState_cpp.md`

## Runtime: Time And Camera

- `src_runtime_clock_ClockState_hpp.md`
- `src_runtime_clock_Clock_hpp.md`
- `src_runtime_clock_Clock_cpp.md`
- `src_runtime_camera_CameraState_hpp.md`
- `src_runtime_camera_CameraModePolicy_hpp.md`
- `src_runtime_camera_CameraModePolicy_cpp.md`

## Runtime: Commands And Session

- `src_runtime_command_Command_hpp.md`
- `src_runtime_command_CommandAdmission_hpp.md`
- `src_runtime_command_CommandAdmission_cpp.md`
- `src_runtime_session_SessionState_hpp.md`
- `src_runtime_session_Session_hpp.md`
- `src_runtime_session_Session_cpp.md`
- `src_runtime_session_SessionTick_hpp.md`
- `src_runtime_session_SessionTick_cpp.md`
- `src_runtime_session_SessionRunner_hpp.md`
- `src_runtime_session_SessionRunner_cpp.md`

## Runtime: Gameplay Systems

- `src_runtime_movement_MovementCommand_hpp.md`
- `src_runtime_movement_MovementSystem_hpp.md`
- `src_runtime_movement_MovementSystem_cpp.md`
- `src_runtime_targeting_TargetQuery_hpp.md`
- `src_runtime_targeting_TargetQuery_cpp.md`
- `src_runtime_targeting_ReachQuery_hpp.md`
- `src_runtime_targeting_ReachQuery_cpp.md`
- `src_runtime_interaction_InteractionDefinition_hpp.md`
- `src_runtime_interaction_InteractionSystem_hpp.md`
- `src_runtime_interaction_InteractionSystem_cpp.md`
- `src_runtime_inventory_InventoryState_hpp.md`
- `src_runtime_inventory_InventorySystem_hpp.md`
- `src_runtime_inventory_InventorySystem_cpp.md`
- `src_runtime_combat_CombatState_hpp.md`
- `src_runtime_combat_CombatSystem_hpp.md`
- `src_runtime_combat_CombatSystem_cpp.md`
- `src_runtime_ai_AiState_hpp.md`
- `src_runtime_ai_AiSystem_hpp.md`
- `src_runtime_ai_AiSystem_cpp.md`
- `src_runtime_objective_ObjectiveState_hpp.md`
- `src_runtime_objective_ObjectiveSystem_hpp.md`
- `src_runtime_objective_ObjectiveSystem_cpp.md`

## Runtime: Save Replay Multiplayer Diagnostics

- `src_runtime_save_SaveEnvelope_hpp.md`
- `src_runtime_save_SaveCodec_hpp.md`
- `src_runtime_save_SaveCodec_cpp.md`
- `src_runtime_save_SaveCompatibility_hpp.md`
- `src_runtime_save_SaveCompatibility_cpp.md`
- `src_runtime_save_SaveLoad_hpp.md`
- `src_runtime_save_SaveLoad_cpp.md`
- `src_runtime_replay_CommandLog_hpp.md`
- `src_runtime_replay_CommandLog_cpp.md`
- `src_runtime_replay_CommandReplay_hpp.md`
- `src_runtime_replay_CommandReplay_cpp.md`
- `src_runtime_replay_StateHash_hpp.md`
- `src_runtime_replay_StateHash_cpp.md`
- `src_runtime_multiplayer_Authority_hpp.md`
- `src_runtime_multiplayer_Authority_cpp.md`
- `src_runtime_multiplayer_ReplicationPacket_hpp.md`
- `src_runtime_multiplayer_ReplicationCodec_hpp.md`
- `src_runtime_multiplayer_ReplicationCodec_cpp.md`
- `src_runtime_multiplayer_LocalMultiplayerSession_hpp.md`
- `src_runtime_multiplayer_LocalMultiplayerSession_cpp.md`
- `src_runtime_diagnostics_RuntimeEvent_hpp.md`
- `src_runtime_diagnostics_RuntimeSummary_hpp.md`
- `src_runtime_diagnostics_RuntimeSummary_cpp.md`
- `src_runtime_diagnostics_RuntimeMetrics_hpp.md`
- `src_runtime_diagnostics_RuntimeMetrics_cpp.md`

## Projection

- `src_projection_scene_SceneItem_hpp.md`
- `src_projection_scene_SceneProjection_hpp.md`
- `src_projection_scene_SceneProjection_cpp.md`
- `src_projection_debug_DebugProjection_hpp.md`
- `src_projection_debug_DebugProjection_cpp.md`

## Apps

- `apps_iggy3d_headless_demo_main_cpp.md`
- `apps_iggy3d_validate_package_main_cpp.md`
- `apps_iggy3d_replay_tool_main_cpp.md`

## Tests

- `tests_unit_math_tests_cpp.md`
- `tests_unit_package_loader_tests_cpp.md`
- `tests_unit_world_state_tests_cpp.md`
- `tests_unit_clock_tests_cpp.md`
- `tests_unit_camera_mode_policy_tests_cpp.md`
- `tests_unit_command_admission_tests_cpp.md`
- `tests_unit_session_state_tests_cpp.md`
- `tests_unit_movement_system_tests_cpp.md`
- `tests_unit_target_reach_tests_cpp.md`
- `tests_unit_interaction_system_tests_cpp.md`
- `tests_unit_inventory_system_tests_cpp.md`
- `tests_unit_combat_system_tests_cpp.md`
- `tests_unit_ai_system_tests_cpp.md`
- `tests_unit_objective_system_tests_cpp.md`
- `tests_unit_save_load_tests_cpp.md`
- `tests_unit_replay_state_hash_tests_cpp.md`
- `tests_unit_multiplayer_authority_tests_cpp.md`
- `tests_unit_projection_tests_cpp.md`
- `tests_acceptance_complete_runtime_demo_tests_cpp.md`
