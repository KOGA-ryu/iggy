# Finisher Batch Bucket

This directory is the finisher work bucket for behavior-preserving cleanup,
roadmap maintenance, and design gates that should not collide with the active
builder worktree.

For the broader project roadmap, see `../roadmap.md`.
For the builder bucket, see `../authoring_batches/`.

Worktree:
- Path: `/Users/kogaryu/iggy-finisher`
- Branch: `codex/finisher-runtime-cleanup`
- Created from builder baseline `c6c2800a` (`Mark authoring facade API packet complete`).

Finisher workflow:
- Pull the lowest-numbered packet that is not complete.
- Execute one slice at a time.
- Commit after each slice that changes files.
- Run full verification at batch end.
- Send one concise batch-end brief to the source thread unless blocked.

Default verification:
- For docs-only packets: docs diff review and `git diff --check`.
- For code packets: focused tests for touched behavior, then full
  `cmake -S engine -B engine/build`, `cmake --build engine/build`,
  `ctest --test-dir engine/build --output-on-failure`, `git diff --check`,
  and relevant reference scans at batch end.
- Always include `git status --short --branch` in batch-end reporting.

Coordination rules:
- Do not work in the builder active worktree.
- Avoid files that builder is actively editing.
- If a packet requires touching `IggyScenarioTomlRunner.cpp` or
  `iggy_scenario_toml_runner_tests.cpp`, first check current builder status and
  prefer a design gate until the facade batch settles.
- Keep changes behavior-preserving unless a packet explicitly opens an output
  contract update and tests lock that contract.

Scope:
- Behavior-preserving cleanup and consolidation.
- Roadmap and bucket maintenance.
- Authoring facade/projection/diagnostic extraction only when it does not
  collide with builder.
- Repo-smell cleanup packets from evidence.

Hard stops:
- No new gameplay semantics.
- No UI/Edi.
- No save/load changes.
- No package or directory scanning.
- No TOML dependency.
- No JSON.
- Do not delete `modules/npc_ai`.
- Do not change CLI output or exit codes unless the packet explicitly says
  output-contract update and tests lock it.

Queue:
1. `01_bucket_and_baseline` - complete. Establish finisher bucket and baseline status.
2. `02_roadmap_status_sync` - complete. Keep roadmap accurate after Batch 09/10 and facade pull-forward.
3. `03_authoring_bucket_status_sync` - complete. Reduce bucket workflow drift without renumbering packets.
4. `04_cli_output_contract_inventory` - complete. Inventory current CLI output and exit-code contract.
5. `05_authoring_projection_design_gate` - complete. Design summary/final-row/expectation projection helper.
6. `06_diagnostic_projection_design_gate` - complete. Design flattened diagnostic entries.
7. `07_fixture_manifest_design_gate` - complete. Design canonical fixture manifest source of truth.
8. `08_test_ceremony_trim_audit` - complete. Audit redundant C++ fixture/test ceremony.
9. `09_smell_signal_dashboard` - complete. Document commands for tracking repo smells.
10. `10_merge_readiness_protocol` - complete. Define merge protocol while builder continues.
11. `11_api_roadmap_drift_cleanup` - complete. Sync API/roadmap docs with completed authoring surfaces.
12. `12_authoring_test_support_extraction` - complete. Extract repeated authoring test helpers.
13. `13_package_facade_parity_assertion_helper` - complete. Extract package/file parity assertion helpers.
14. `14_runtime_report_duplication_audit` - complete. Audit repeated runtime count/report fields.
15. `15_projection_status_mapping_audit` - complete. Audit duplicate projection/status mapping.
16. `16_cmake_authoring_test_hygiene_gate` - complete. Assess authoring CMake test registration hygiene.
17. `17_finisher_merge_hygiene_update` - complete. Update finisher merge/rebase protocol after cleanup stretch.
18. `18_runtime_report_count_projection_cleanup` - complete. Extract repeated inventory-event count projection.
19. `19_runtime_scenario_ledger_npc_projection_cleanup` - complete. Extract shared scenario/ledger NPC aggregate projection.
20. `20_legacy_npc_tick_config_include_trim` - complete. Split NPC tick config into a narrow header and trim config-only controller includes.
21. `21_ui_preview_consumer_status_sync` - complete. Sync planning docs after read-only Qt preview consumer integration.
22. `22_product_loader_status_sync` - complete. Sync planning/API docs after load-only product scenario loader integration.
23. `23_product_loop_status_sync` - complete. Sync planning/API docs after product-owned one-frame loop integration.
24. `24_player_input_binding_status_sync` - complete. Sync planning/API docs after scene/player normalized input binding integration.
25. `25_product_input_adapter_status_sync` - complete. Sync planning/API docs after runtime/product input adapter integration.
26. `26_product_loop_context_override_status_sync` - complete. Sync planning/API docs after per-step product loop input context override integration.
27. `27_product_presentation_frame_status_sync` - complete. Sync planning/API docs after projection-only product presentation frame integration.
28. `28_product_play_surface_frame_status_sync` - complete. Sync planning/API docs after runtime-only product play-surface frame integration.
29. `29_product_play_mode_status_sync` - complete. Sync planning/API docs after runtime-only product play-mode state integration.
30. `30_product_play_ui_projection_status_sync` - complete. Sync planning/API docs after read-only product play UI projection integration.
31. `31_qt_product_play_launch_status_sync` - complete. Sync planning/API docs after Qt product play launch/load/build consumer integration.
32. `32_qt_product_play_focus_status_sync` - complete. Sync planning/API docs after Qt product play focus toggle integration.
33. `33_qt_product_input_mapping_status_sync` - complete. Sync planning/API docs after Qt-local product input key mapping integration.
34. `34_product_presentation_camera_status_sync` - complete. Sync planning/API docs after runtime/product presentation camera policy integration.
35. `35_product_frame_request_status_sync` - complete. Sync planning/API docs after runtime/product manual frame request wrapper integration.
36. `36_qt_product_manual_step_status_sync` - complete. Sync planning/API docs after Qt product manual Step consumer integration.
37. `37_product_input_context_projection_status_sync` - complete. Sync planning/API docs after product input binding context projection integration.
38. `38_product_input_accumulator_status_sync` - complete. Sync planning/API docs after product held input accumulator integration.
39. `39_qt_product_frame_pump_status_sync` - complete. Sync planning/API docs after Qt product frame pump toggle integration.
40. `40_product_actor_render_projection_status_sync` - complete. Sync planning/API docs after product gameplay actor render projection integration.
41. `41_product_pointer_projection_status_sync` - complete. Sync planning/API docs after product pointer projection and explicit primary input accumulator preservation.
42. `42_qt_product_viewport_owner_status_sync` - complete. Sync planning/API docs after Qt product viewport owner integration.
43. `43_qt_product_primary_tile_mouse_status_sync` - complete. Sync planning/API docs after thin Qt product primary-tile mouse consumer integration.
44. `44_interaction_target_spatial_query_status_sync` - complete. Sync planning/API docs after scene-only interaction target spatial query integration.
45. `45_product_interaction_target_query_status_sync` - complete. Sync planning/API docs after runtime/product interaction target query integration.
46. `46_product_input_target_context_status_sync` - complete. Sync planning/API docs after runtime/product input target context projection integration.
47. `47_product_input_frame_target_context_status_sync` - complete. Sync planning/API docs after runtime/product input frame target context enrichment integration.
48. `48_qt_product_input_frame_target_context_consumer_status_sync` - complete. Sync planning/API docs after Qt product input frame target context consumer integration.
49. `49_product_input_target_context_diagnostics_projection_status_sync` - complete. Sync planning/API docs after product input target-context diagnostics projection integration.
50. `50_qt_product_viewport_render_drawer_status_sync` - complete. Sync planning/API docs after thin Qt product viewport render command drawer integration.
51. `51_qt_product_target_highlight_overlay_status_sync` - complete. Sync planning/API docs after thin Qt product target highlight overlay integration.
52. `52_native_scripted_controls_debugger_status_sync` - complete. Sync planning/API docs after native scripted controls, debugger, expectations, and final-state dump integration.
53. `53_native_scene_draw_list_extraction_status_sync` - complete. Sync planning/API docs after native scene draw-list extraction integration.
54. `54_native_product_session_extraction_status_sync` - complete. Sync planning/API docs after native product session extraction integration.
55. `55_native_vulkan_renderer_skeleton_extraction_status_sync` - complete. Sync planning/API docs after native Vulkan renderer skeleton extraction integration.
56. `56_native_gpu_mesh_resource_wrapper_status_sync` - complete. Sync planning/API docs after native renderer-private GPU mesh resource wrapper integration.
57. `57_native_pipeline_shader_resource_wrapper_status_sync` - complete. Sync planning/API docs after native renderer-private pipeline/shader resource wrapper integration.
58. `58_native_model_slot_cube_fallback_registry_status_sync` - complete. Sync planning/API docs after native renderer-private model slot/cube fallback registry integration.
59. `59_native_static_mesh_asset_data_model_status_sync` - complete. Sync planning/API docs after native backend-free static mesh asset data model integration.
60. `60_native_procedural_bean_mesh_slot_binding_status_sync` - complete. Sync planning/API docs after native procedural bean mesh player slot binding integration.
61. `61_native_procedural_npc_mesh_slot_binding_status_sync` - complete. Sync planning/API docs after native procedural NPC marker mesh slot binding integration.
62. `62_native_static_mesh_text_loader_status_sync` - complete. Sync planning/API docs after native static mesh text loader integration.
63. `63_native_player_mesh_asset_binding_status_sync` - complete. Sync planning/API docs after native player mesh asset file binding integration.
64. `64_native_npc_mesh_asset_binding_status_sync` - complete. Sync planning/API docs after native NPC mesh asset file binding integration.
65. `65_native_floor_wall_mesh_asset_binding_status_sync` - complete. Sync planning/API docs after native floor/wall mesh asset file binding integration.
66. `66_native_static_model_policy_status_sync` - complete. Sync planning/API docs after native static model slot policy integration.
67. `67_native_static_model_load_report_status_sync` - complete. Sync planning/API docs after native static model load report integration.
68. `68_native_static_model_load_report_cli_status_sync` - complete. Sync planning/API docs after native static model load report CLI integration.
69. `69_native_static_mesh_text_writer_status_sync` - complete. Sync planning/API docs after native static mesh text writer integration.
70. `70_native_static_mesh_fixture_writer_roundtrip_status_sync` - complete. Sync planning/API docs after native static mesh fixture writer roundtrip test integration.
71. `71_native_static_mesh_built_in_export_cli_status_sync` - complete. Sync planning/API docs after native static mesh built-in export CLI integration.
72. `72_native_static_mesh_export_policy_status_sync` - complete. Sync planning/API docs after native static mesh export policy integration.
73. `73_native_static_mesh_output_directory_export_cli_status_sync` - complete. Sync planning/API docs after native static mesh output directory export CLI integration.
74. `74_native_static_mesh_built_in_batch_export_cli_status_sync` - complete. Sync planning/API docs after native static mesh built-in batch export CLI integration.
75. `75_native_static_mesh_export_report_cli_status_sync` - complete. Sync planning/API docs after native static mesh export report CLI integration.
76. `76_native_static_mesh_export_policy_validation_status_sync` - complete. Sync planning/API docs after native static mesh export policy validation integration.
77. `77_native_static_mesh_export_manifest_text_builder_status_sync` - complete. Sync planning/API docs after native static mesh export manifest text builder CLI integration.
78. `78_native_static_mesh_batch_manifest_sidecar_export_status_sync` - complete. Sync planning/API docs after native static mesh batch manifest sidecar export integration.
79. `79_native_static_mesh_export_directory_verification_cli_status_sync` - complete. Sync planning/API docs after native static mesh export directory verification CLI integration.
80. `80_native_static_mesh_export_verification_report_cli_status_sync` - complete. Sync planning/API docs after native static mesh export verification report CLI integration.
81. `81_native_static_mesh_export_package_policy_status_sync` - complete. Sync planning/API docs after native static mesh export package policy integration.
82. `82_native_static_mesh_export_package_manifest_text_builder_status_sync` - complete. Sync planning/API docs after native static mesh export package manifest text builder integration.
83. `83_native_static_mesh_batch_package_manifest_sidecar_export_status_sync` - complete. Sync planning/API docs after native static mesh batch package manifest sidecar export integration.
84. `84_native_static_mesh_package_sidecar_verification_status_sync` - complete. Sync planning/API docs after native static mesh package sidecar verification integration.
85. `85_native_static_mesh_verification_summary_sidecar_diagnostics_status_sync` - complete. Sync planning/API docs after native static mesh verification summary sidecar diagnostics integration.
86. `86_native_static_mesh_package_manifest_text_reader_status_sync` - complete. Sync planning/API docs after native static mesh package manifest text reader integration.
87. `87_native_static_mesh_package_manifest_file_reader_status_sync` - complete. Sync planning/API docs after native static mesh package manifest file reader integration.
88. `88_native_static_mesh_package_manifest_verification_reader_diagnostics_status_sync` - complete. Sync planning/API docs after native static mesh package manifest verification reader diagnostics integration.
89. `89_native_static_mesh_verification_package_read_issue_rows_status_sync` - complete. Sync planning/API docs after native static mesh verification package read issue rows integration.
90. `90_native_static_mesh_package_directory_reader_status_sync` - complete. Sync planning/API docs after native static mesh package directory reader integration.
91. `91_native_static_mesh_package_directory_report_builder_status_sync` - complete. Sync planning/API docs after native static mesh package directory report builder integration.
92. `92_native_static_mesh_package_directory_report_cli_status_sync` - complete. Sync planning/API docs after native static mesh package directory report CLI integration.
93. `93_native_static_mesh_package_directory_presence_diagnostics_status_sync` - complete. Sync planning/API docs after native static mesh package directory presence diagnostics integration.
94. `94_native_static_mesh_package_directory_file_fact_diagnostics_status_sync` - complete. Sync planning/API docs after native static mesh package directory file fact diagnostics integration.
95. `95_native_static_mesh_export_manifest_text_reader_status_sync` - complete. Sync planning/API docs after native static mesh export manifest text reader integration.
96. `96_native_static_mesh_export_manifest_file_reader_status_sync` - complete. Sync planning/API docs after native static mesh export manifest file reader integration.
97. `97_native_static_mesh_package_directory_manifest_read_diagnostics_status_sync` - complete. Sync planning/API docs after native static mesh package directory manifest read diagnostics integration.
98. `98_native_static_mesh_package_directory_manifest_asset_rows_status_sync` - complete. Sync planning/API docs after native static mesh package directory manifest asset rows integration.
99. `99_native_static_mesh_package_directory_manifest_row_comparison_diagnostics_status_sync` - complete. Sync planning/API docs after native static mesh package directory manifest row comparison diagnostics integration.
100. `100_native_static_mesh_package_directory_comparison_issue_count_diagnostics_status_sync` - complete. Sync planning/API docs after native static mesh package directory comparison issue count diagnostics integration.
101. `101_native_static_mesh_package_directory_manifest_comparison_helper_status_sync` - complete. Sync planning/API docs after native static mesh package directory manifest comparison helper integration.
102. `102_native_static_mesh_package_directory_structured_manifest_diagnostics_status_sync` - complete. Sync planning/API docs after native static mesh package directory structured manifest diagnostics integration.
103. `103_native_static_mesh_package_directory_structured_file_facts_status_sync` - complete. Sync planning/API docs after native static mesh package directory structured file facts integration.
104. `104_native_static_mesh_package_directory_report_text_renderer_extraction_status_sync` - complete. Sync planning/API docs after native static mesh package directory report text renderer extraction.
105. `105_native_static_mesh_package_directory_report_data_builder_extraction_status_sync` - complete. Sync planning/API docs after native static mesh package directory report data builder extraction.
106. `106_native_static_mesh_export_verification_report_text_renderer_extraction_status_sync` - complete. Sync planning/API docs after native static mesh export verification report text renderer extraction.
107. `107_native_static_mesh_export_verification_report_data_builder_extraction_status_sync` - complete. Sync planning/API docs after native static mesh export verification report data builder extraction.
108. `108_native_static_mesh_verification_report_builder_parity_coverage_status_sync` - complete. Sync planning/API docs after native static mesh verification report builder parity coverage.
109. `109_native_static_mesh_package_directory_report_builder_parity_coverage_status_sync` - complete. Sync planning/API docs after native static mesh package directory report builder parity coverage.
110. `110_native_static_mesh_package_manifest_read_issue_text_helper_extraction_status_sync` - complete. Sync planning/API docs after native static mesh package manifest read issue text helper extraction.
111. `111_native_static_mesh_export_manifest_read_issue_text_helper_extraction_status_sync` - complete. Sync planning/API docs after native static mesh export manifest read issue text helper extraction.
112. `112_native_static_mesh_package_directory_read_status_text_helper_extraction_status_sync` - complete. Sync planning/API docs after native static mesh package directory read status text helper extraction.
113. `113_native_static_mesh_export_directory_verification_status_text_helper_extraction_status_sync` - complete. Sync planning/API docs after native static mesh export directory verification status text helper extraction.
114. `114_native_static_mesh_file_export_status_text_helper_extraction_status_sync` - complete. Sync planning/API docs after native static mesh file export status text helper extraction.
115. `115_native_static_mesh_export_report_status_text_helper_extraction_status_sync` - complete. Sync planning/API docs after native static mesh export report status text helper extraction.
116. `116_native_static_mesh_export_manifest_status_text_helper_extraction_status_sync` - complete. Sync planning/API docs after native static mesh export manifest status text helper extraction.
117. `117_native_static_mesh_export_package_manifest_status_text_helper_extraction_status_sync` - complete. Sync planning/API docs after native static mesh export package manifest status text helper extraction.
118. `118_native_static_model_slot_text_helper_extraction_status_sync` - complete. Sync planning/API docs after native static model slot text helper extraction.
119. `119_native_static_model_load_status_text_helper_extraction_status_sync` - complete. Sync planning/API docs after native static model load status text helper extraction.
120. `120_native_static_model_fallback_kind_text_helper_extraction_status_sync` - complete. Sync planning/API docs after native static model fallback kind text helper extraction.
121. `121_native_static_model_load_report_text_renderer_extraction_status_sync` - complete. Sync planning/API docs after native static model load report text renderer extraction.
122. `122_native_static_mesh_export_report_text_renderer_extraction_status_sync` - complete. Sync planning/API docs after native static mesh export report text renderer extraction.
123. `123_native_static_mesh_single_file_export_success_text_renderer_extraction_status_sync` - complete. Sync planning/API docs after native static mesh single file export success text renderer extraction.
124. `124_native_static_mesh_single_file_export_failure_text_renderer_extraction_status_sync` - complete. Sync planning/API docs after native static mesh single file export failure text renderer extraction.
125. `125_native_static_mesh_batch_export_success_text_renderer_extraction_status_sync` - complete. Sync planning/API docs after native static mesh batch export success text renderer extraction.
126. `126_native_static_mesh_batch_export_failure_text_renderer_extraction_status_sync` - complete. Sync planning/API docs after native static mesh batch export failure text renderer extraction.
127. `127_native_static_mesh_export_manifest_failure_text_renderer_extraction_status_sync` - complete. Sync planning/API docs after native static mesh export manifest failure text renderer extraction.
128. `128_native_static_mesh_export_package_manifest_failure_text_renderer_extraction_status_sync` - complete. Sync planning/API docs after native static mesh export package manifest failure text renderer extraction.
129. `129_native_static_mesh_export_verification_success_text_renderer_extraction_status_sync` - complete. Sync planning/API docs after native static mesh export verification success text renderer extraction.
130. `130_native_static_mesh_export_verification_failure_text_renderer_extraction_status_sync` - complete. Sync planning/API docs after native static mesh export verification failure text renderer extraction.
131. `131_native_static_mesh_export_verification_report_failure_text_renderer_extraction_status_sync` - complete. Sync planning/API docs after native static mesh export verification report failure text renderer extraction.
132. `132_native_static_mesh_export_package_directory_report_failure_text_renderer_extraction_status_sync` - complete. Sync planning/API docs after native static mesh export package directory report failure text renderer extraction.
133. `133_native_static_mesh_asset_dump_writer_failure_text_renderer_extraction_status_sync` - complete. Sync planning/API docs after native static mesh asset dump writer failure text renderer extraction.
