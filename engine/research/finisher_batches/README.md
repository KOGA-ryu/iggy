# Finisher Batch Bucket

This directory is the finisher work bucket for behavior-preserving cleanup,
roadmap maintenance, and design gates that should not collide with the active
builder worktree.

For the broader project roadmap, see `../roadmap.md`.
For the builder bucket, see `../authoring_batches/`.

Worktree:
- Path: `/Users/kogaryu/iggy-finisher`
- Branch: `codex/finisher-roadmap`
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
13. `13_package_facade_parity_assertion_helper` - pending. Extract package/file parity assertion helpers.
14. `14_runtime_report_duplication_audit` - pending. Audit repeated runtime count/report fields.
15. `15_projection_status_mapping_audit` - pending. Audit duplicate projection/status mapping.
16. `16_cmake_authoring_test_hygiene_gate` - pending. Assess authoring CMake test registration hygiene.
17. `17_finisher_merge_hygiene_update` - pending. Update finisher merge/rebase protocol after cleanup stretch.
