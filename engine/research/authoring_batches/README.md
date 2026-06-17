# Authoring Batch Bucket

This directory is the builder work bucket for the TOML authored-scenario lane.

For the broader project roadmap, see `../roadmap.md`.

Builder workflow:
- Pull the lowest-numbered packet that is not complete.
- If the planner gives a scoped stretch, follow that explicit order before
  returning to raw queue order.
- Execute one slice at a time.
- Commit after each slice that changes files.
- Send only one short batch-end brief to the planner unless blocked.
- Stop early only for a true semantic fork, a verification failure that changes scope, or explicit planner/user redirection.

Default verification:
- Focused build/tests after each slice.
- Full `cmake -S engine -B engine/build`, `cmake --build engine/build`, `ctest --test-dir engine/build --output-on-failure`, `git diff --check`, and reference scans at batch end.

Global hard stops:
- No UI/Edi integration.
- No save/load changes unless the packet explicitly opens that lane.
- No runtime autorun.
- No TOML dependency/library.
- No directory scanning unless a packet explicitly opens that lane.
- No generic scripting language.
- No broad wrapper/report/ledger additions.

Status key:
- `complete` - landed and verified.
- `open` - actionable within the packet's hard stops.
- `gated` - review/decision packet, or implementation packet waiting on a gate.
- `deferred` - valid future lane, but not part of the current authoring hardening
  focus.

Current raw next packet:
- `11_ai_map_region_fixtures`, unless a planner-scoped stretch overrides raw
  order.

Queue:
1. `[complete]` `01_trace_mode` - per-frame CLI trace output.
2. `[complete]` `02_fixture_contracts` - fixture contracts and canonical fixture hygiene.
3. `[complete]` `03_cli_diagnostics_matrix` - complete CLI failure diagnostics coverage.
4. `[complete]` `04_scenario_expectations` - TOML-authored expected results.
5. `[complete]` `05_golden_compare_mode` - CLI comparison against expected results.
6. `[complete]` `06_lint_mode` - validate without running.
7. `[complete]` `07_mixed_scenario_pack` - richer canonical examples.
8. `[complete]` `08_locked_door_key_gate` - review gate for locked-door/key semantics.
9. `[complete]` `09_locked_door_key_implementation` - implementation only if gate passes.
10. `[complete]` `10_npc_collision_reservation_fixtures` - movement collision/reservation examples.
11. `[open]` `11_ai_map_region_fixtures` - AI map region examples.
12. `[deferred]` `12_authored_scenario_save_load_roundtrip` - persistence roundtrip proof.
13. `[complete]` `13_fixture_ceremony_cleanup` - remove redundant C++ fixture ceremony.
14. `[deferred]` `14_authoring_package_layout` - proposed scenario package shape.
15. `[gated]` `15_editor_handoff_gate` - final UI/Edi integration gate.
16. `[complete]` `16_authoring_facade_api` - reusable engine API for file-to-run flow.
17. `[gated]` `17_package_runner_gate` - review gate for directory/package execution.
18. `[gated]` `18_package_runner_implementation` - package runner only if gate passes.
19. `[gated]` `19_editor_preview_model_gate` - gate for engine-only preview model shape.
20. `[open]` `20_authoring_diff_report` - compare two scenario runs without new semantics.
21. `[complete]` `21_source_plan_version_policy` - explicit source-plan version compatibility.
22. `[complete]` `22_schema_snapshot_tests` - snapshot supported TOML table/key surface.
23. `[open]` `23_fixtures_from_cli_examples` - derive docs/examples from canonical fixtures.
24. `[complete]` `24_batch_queue_maintenance` - keep this bucket pruned and current.
25. `[gated]` `25_runtime_authoring_cleanup_gate` - review cleanup targets after v1 hardening.
26. `[gated]` `26_minimal_editor_model` - engine-only model if prior gates pass.
27. `[open]` `27_authored_scenario_perf_budget` - lightweight performance/size budget checks.
28. `[complete]` `28_content_error_codes` - stable user-facing authoring error-code pass.
29. `[open]` `29_release_candidate_authoring_v1` - freeze an Authoring v1 acceptance set.
30. `[gated]` `30_post_v1_semantics_gate` - decide next gameplay semantics after v1.
31. `[complete]` `31_authoring_facade_parity_audit` - keep CLI/facade execution paths equivalent.
32. `[complete]` `32_canonical_fixture_manifest` - one test-owned canonical fixture source of truth.
33. `[complete]` `33_trace_expectations` - optional per-frame expectation checks.
34. `[complete]` `34_negative_fixture_catalog` - first-class failure fixture catalog.
35. `[gated]` `35_authoring_warning_channel_gate` - decide whether warnings are needed.
36. `[open]` `36_resource_id_namespace_policy` - authoring ResourceId convention policy.
37. `[open]` `37_frame_ordering_policy` - deterministic authored frame ordering checks.
38. `[complete]` `38_no_hidden_defaults_audit` - enforce self-contained canonical fixtures.
39. `[complete]` `39_toml_subset_stress_pack` - parser subset edge regression pack.
40. `[gated]` `40_authoring_size_limits_gate` - decide source-plan scale limits.
41. `[gated]` `41_emit_event_semantics_gate` - gate authored emit-event behavior.
42. `[gated]` `42_talk_interaction_semantics_gate` - gate authored talk behavior.
43. `[gated]` `43_region_trigger_semantics_gate` - gate region trigger behavior.
44. `[gated]` `44_package_metadata_manifest` - package metadata after package runner approval.
45. `[gated]` `45_authoring_compatibility_migration_gate` - gate source-plan migration support.
46. `[complete]` `46_authoring_facade_mode_matrix` - shared facade modes for run/lint/check/trace.
47. `[complete]` `47_authoring_diagnostic_entries` - flattened user-facing diagnostic entries.
48. `[complete]` `48_cli_output_contract_snapshots` - lock CLI textual output contracts.
49. `[complete]` `49_manifest_sweep_test_target` - manifest-driven canonical fixture sweeps.
50. `[gated]` `50_package_fixture_pack` - package examples after package runner approval.
51. `[gated]` `51_file_package_parity` - parity between one-file and package scenarios.
52. `[complete]` `52_no_claims_boundary_regression` - safety flag regression fixtures.
53. `[gated]` `53_terrain_authoring_policy_gate` - gate terrain metadata expansion.
54. `[open]` `54_multi_actor_profile_fixture_pack` - multi-actor/profile fixture coverage.
55. `[open]` `55_interaction_effect_fixture_pack` - fixtures for existing interaction effects.
56. `[complete]` `56_inventory_expectations` - final inventory expectation checks.
57. `[complete]` `57_interaction_state_expectations` - final interaction state expectations.
58. `[complete]` `58_actor_state_expectations` - final player/NPC state expectations.
59. `[complete]` `59_authoring_run_summary_projection` - shared summary projection helper.
60. `[gated]` `60_authoring_bucket_prune_gate` - prune stale/superseded bucket packets.
