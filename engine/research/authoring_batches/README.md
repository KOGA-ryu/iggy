# Authoring Batch Bucket

This directory is the builder work bucket for the TOML authored-scenario lane.

Builder workflow:
- Pull the lowest-numbered packet that is not complete.
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

Queue:
1. `01_trace_mode` - per-frame CLI trace output.
2. `02_fixture_contracts` - fixture contracts and canonical fixture hygiene.
3. `03_cli_diagnostics_matrix` - complete CLI failure diagnostics coverage.
4. `04_scenario_expectations` - TOML-authored expected results.
5. `05_golden_compare_mode` - CLI comparison against expected results.
6. `06_lint_mode` - validate without running.
7. `07_mixed_scenario_pack` - richer canonical examples.
8. `08_locked_door_key_gate` - review gate for locked-door/key semantics.
9. `09_locked_door_key_implementation` - implementation only if gate passes.
10. `10_npc_collision_reservation_fixtures` - movement collision/reservation examples.
11. `11_ai_map_region_fixtures` - AI map region examples.
12. `12_authored_scenario_save_load_roundtrip` - persistence roundtrip proof.
13. `13_fixture_ceremony_cleanup` - remove redundant C++ fixture ceremony.
14. `14_authoring_package_layout` - proposed scenario package shape.
15. `15_editor_handoff_gate` - final UI/Edi integration gate.
