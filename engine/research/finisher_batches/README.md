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
