# iggy3d Complete File Plan Priority

Updated: 2026-06-20

This is the complete-build fill order. It ranks every file-specific document
for the standalone `iggy3d` runtime. Later tiers must consume earlier ownership
contracts, not redefine them.

## Tier 0: Repo Contract And Build Shell

Why: these files prevent `iggy3d` from becoming another branch of old `iggy`.
They define target names, no-legacy dependency rules, test registration, and
quality gates.

1. `docs_architecture_md.md`
2. `docs_ownership_md.md`
3. `docs_roadmap_md.md`
4. `COMPLETE_BUILD_SURFACE.md`
5. `CMakeLists_txt.md`
6. `cmake_iggy3d_options_cmake.md`
7. `cmake_iggy3d_warnings_cmake.md`
8. `cmake_iggy3d_tests_cmake.md`

## Tier 1: Core Contracts

Why: every runtime subsystem depends on stable ids, math, diagnostics, result
status, and hashing.

9. `src_core_result_Result_hpp.md`
10. `src_core_diagnostics_Diagnostic_hpp.md`
11. `src_core_ids_EntityId_hpp.md`
12. `src_core_hash_StableHash_hpp.md`
13. `src_core_hash_StableHash_cpp.md`
14. `src_core_math_Vec3_hpp.md`
15. `src_core_math_Vec3_cpp.md`
16. `src_core_math_Transform3_hpp.md`
17. `src_core_math_Transform3_cpp.md`
18. `src_core_math_Aabb3_hpp.md`
19. `src_core_math_Aabb3_cpp.md`
20. `src_core_math_Ray3_hpp.md`
21. `src_core_math_Ray3_cpp.md`
22. `src_core_math_Plane_hpp.md`
23. `src_core_math_Plane_cpp.md`
24. `src_core_math_Mat4_hpp.md`
25. `src_core_math_Mat4_cpp.md`
26. `tests_unit_math_tests_cpp.md`

## Tier 2: Content And Configuration

Why: a complete runtime needs explicit package/fixture input and config before
session creation can be meaningful.

27. `src_config_RuntimeConfig_hpp.md`
28. `src_app_AppConfig_hpp.md`
29. `src_app_AppConfig_cpp.md`
30. `src_app_CliParser_hpp.md`
31. `src_app_CliParser_cpp.md`
32. `src_content_PackageManifest_hpp.md`
33. `src_content_PackageLoader_hpp.md`
34. `src_content_PackageLoader_cpp.md`
35. `src_content_PackageValidator_hpp.md`
36. `src_content_PackageValidator_cpp.md`
37. `fixtures_demos_first_room_package_iggy3d_toml.md`
38. `fixtures_demos_first_room_scenario_iggy3d_toml.md`
39. `src_content_FixtureScenarioLoader_hpp.md`
40. `src_content_FixtureScenarioLoader_cpp.md`
41. `tests_unit_package_loader_tests_cpp.md`

## Tier 3: Player World Authority

Why: the runtime becomes real only when player slots, entities, transforms, and
world mutation are owned and testable.

42. `src_runtime_player_PlayerSlot_hpp.md`
43. `src_runtime_player_PlayerRoster_hpp.md`
44. `src_runtime_player_PlayerRoster_cpp.md`
45. `src_runtime_world_EntityState_hpp.md`
46. `src_runtime_world_WorldState_hpp.md`
47. `src_runtime_world_WorldState_cpp.md`
48. `tests_unit_world_state_tests_cpp.md`

## Tier 4: Time Camera Command Session Base

Why: realtime play, slow-time tactical view, command records, and session
lifecycle are the core runtime loop.

49. `src_runtime_clock_ClockState_hpp.md`
50. `src_runtime_clock_Clock_hpp.md`
51. `src_runtime_clock_Clock_cpp.md`
52. `tests_unit_clock_tests_cpp.md`
53. `src_runtime_camera_CameraState_hpp.md`
54. `src_runtime_camera_CameraModePolicy_hpp.md`
55. `src_runtime_camera_CameraModePolicy_cpp.md`
56. `tests_unit_camera_mode_policy_tests_cpp.md`
57. `src_runtime_command_Command_hpp.md`
58. `src_runtime_command_CommandAdmission_hpp.md`
59. `src_runtime_command_CommandAdmission_cpp.md`
60. `tests_unit_command_admission_tests_cpp.md`
61. `src_runtime_replay_CommandLog_hpp.md`
62. `src_runtime_replay_CommandLog_cpp.md`
63. `src_runtime_session_SessionState_hpp.md`
64. `src_runtime_session_Session_hpp.md`
65. `src_runtime_session_Session_cpp.md`
66. `tests_unit_session_state_tests_cpp.md`

## Tier 5: Playable Gameplay Systems

Why: target discovery, reach gating, movement, interaction, inventory, combat,
AI, and objectives are required for a finished playable loop.

67. `src_runtime_movement_MovementCommand_hpp.md`
68. `src_runtime_movement_MovementSystem_hpp.md`
69. `src_runtime_movement_MovementSystem_cpp.md`
70. `tests_unit_movement_system_tests_cpp.md`
71. `src_runtime_targeting_TargetQuery_hpp.md`
72. `src_runtime_targeting_TargetQuery_cpp.md`
73. `src_runtime_targeting_ReachQuery_hpp.md`
74. `src_runtime_targeting_ReachQuery_cpp.md`
75. `tests_unit_target_reach_tests_cpp.md`
76. `src_runtime_inventory_InventoryState_hpp.md`
77. `src_runtime_inventory_InventorySystem_hpp.md`
78. `src_runtime_inventory_InventorySystem_cpp.md`
79. `tests_unit_inventory_system_tests_cpp.md`
80. `src_runtime_interaction_InteractionDefinition_hpp.md`
81. `src_runtime_interaction_InteractionSystem_hpp.md`
82. `src_runtime_interaction_InteractionSystem_cpp.md`
83. `tests_unit_interaction_system_tests_cpp.md`
84. `src_runtime_combat_CombatState_hpp.md`
85. `src_runtime_combat_CombatSystem_hpp.md`
86. `src_runtime_combat_CombatSystem_cpp.md`
87. `tests_unit_combat_system_tests_cpp.md`
88. `src_runtime_ai_AiState_hpp.md`
89. `src_runtime_ai_AiSystem_hpp.md`
90. `src_runtime_ai_AiSystem_cpp.md`
91. `tests_unit_ai_system_tests_cpp.md`
92. `src_runtime_objective_ObjectiveState_hpp.md`
93. `src_runtime_objective_ObjectiveSystem_hpp.md`
94. `src_runtime_objective_ObjectiveSystem_cpp.md`
95. `tests_unit_objective_system_tests_cpp.md`

## Tier 6: Full Session Loop Diagnostics Projection

Why: complete runtime work needs ordered ticking, event reporting, scene/debug
projection, and deterministic summaries before save/replay acceptance.

96. `src_runtime_diagnostics_RuntimeEvent_hpp.md`
97. `src_runtime_diagnostics_RuntimeMetrics_hpp.md`
98. `src_runtime_diagnostics_RuntimeMetrics_cpp.md`
99. `src_runtime_session_SessionTick_hpp.md`
100. `src_runtime_session_SessionTick_cpp.md`
101. `src_runtime_session_SessionRunner_hpp.md`
102. `src_runtime_session_SessionRunner_cpp.md`
103. `src_projection_scene_SceneItem_hpp.md`
104. `src_projection_scene_SceneProjection_hpp.md`
105. `src_projection_scene_SceneProjection_cpp.md`
106. `src_projection_debug_DebugProjection_hpp.md`
107. `src_projection_debug_DebugProjection_cpp.md`
108. `tests_unit_projection_tests_cpp.md`
109. `src_runtime_diagnostics_RuntimeSummary_hpp.md`
110. `src_runtime_diagnostics_RuntimeSummary_cpp.md`

## Tier 7: Durability Replay Multiplayer

Why: a finished runtime must save/load, replay deterministically, expose state
hashes, and support multiplayer-ready command/session authority.

111. `src_runtime_save_SaveEnvelope_hpp.md`
112. `src_runtime_save_SaveCompatibility_hpp.md`
113. `src_runtime_save_SaveCompatibility_cpp.md`
114. `src_runtime_save_SaveCodec_hpp.md`
115. `src_runtime_save_SaveCodec_cpp.md`
116. `src_runtime_save_SaveLoad_hpp.md`
117. `src_runtime_save_SaveLoad_cpp.md`
118. `tests_unit_save_load_tests_cpp.md`
119. `src_runtime_replay_StateHash_hpp.md`
120. `src_runtime_replay_StateHash_cpp.md`
121. `src_runtime_replay_CommandReplay_hpp.md`
122. `src_runtime_replay_CommandReplay_cpp.md`
123. `tests_unit_replay_state_hash_tests_cpp.md`
124. `src_runtime_multiplayer_Authority_hpp.md`
125. `src_runtime_multiplayer_Authority_cpp.md`
126. `src_runtime_multiplayer_ReplicationPacket_hpp.md`
127. `src_runtime_multiplayer_ReplicationCodec_hpp.md`
128. `src_runtime_multiplayer_ReplicationCodec_cpp.md`
129. `src_runtime_multiplayer_LocalMultiplayerSession_hpp.md`
130. `src_runtime_multiplayer_LocalMultiplayerSession_cpp.md`
131. `tests_unit_multiplayer_authority_tests_cpp.md`

## Tier 8: Product Proof And Tools

Why: these prove the complete build end-to-end and provide the tools needed to
validate content and replay state without renderer or old legacy dependencies.

132. `fixtures_demos_first_room_expected_summary_txt.md`
133. `docs_acceptance_demo_md.md`
134. `apps_iggy3d_headless_demo_main_cpp.md`
135. `apps_iggy3d_validate_package_main_cpp.md`
136. `apps_iggy3d_replay_tool_main_cpp.md`
137. `tests_acceptance_complete_runtime_demo_tests_cpp.md`

