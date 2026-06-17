# Iggy Roadmap

This is the internal project roadmap checkpoint. It is broader than the TOML
authoring batch bucket and should be updated when a batch changes the project
shape, not after every small slice.

## Current Checkpoint

- Authoring batches 01-08 are complete.
- Batch 09 locked-door/key semantics is in progress.
- The authored TOML lane supports parser validation, conversion, CLI run, trace,
  check, lint, canonical fixtures, and scenario expectations.
- The builder bucket remains the source of short-term slice work:
  `engine/research/authoring_batches/`.

## Have

- CMake/test foundation with focused subsystem test targets and full CTest
  verification.
- Core math/resource types and backend-neutral server primitives for render,
  navigation, and physics queries.
- Level/map/cache lanes for tile maps, runtime state, render cache, collision
  cache, derived caches, and level mutation/cache updates.
- Newer scene AI/NPC/player lanes for profile traits, AI maps, actor/control
  state, movement planning/application, player input intents, and command
  planning.
- Interaction and inventory foundations: targets, effects, reach/query/plan/apply
  flow, drops, pickup policy, and inventory transfer.
- Runtime gameplay/session/scenario infrastructure: command queues, player input,
  interaction, pickup, NPC movement, orchestrated frames, profile scenarios, and
  scenario runners.
- Save/load foundations: session/gameplay snapshots, chunk archive/envelope,
  file IO, save slots, and autosave rotation.
- TOML source-plan authoring lane: grid, legend, cells, regions, controls,
  profiles, interaction targets/effects, item drops, player commands, safety
  flags, promotion policy, and expectations.
- Authoring conversion lane: source-plan validator, TOML reader/file reader,
  authoring adapter, region-to-AI-map promoter, source-plan-to-profile-scenario
  converter, profile validator/runner, and final debug row projection.
- CLI/dev harness: `iggy_scenario_toml_runner` with run, `--trace`, `--check`,
  and `--lint` modes.
- Canonical and regression TOML fixtures with a fixture contract.
- Authoring batch bucket with numbered implementation packets.

## Sorta Have

- Authoring v1 is functional through CLI, but the reusable engine facade is not
  extracted yet.
- Check/lint/trace modes exist, but the CLI still owns most orchestration.
- Expectations exist, but mainly cover final rows and summary counts.
- TOML reading is an intentional subset, not a full TOML implementation.
- Runtime orchestration has many wrappers, counters, reports, and runners that
  are useful for tests but carry bloat risk.
- Legacy `modules/npc_ai` still coexists with newer `scene/ai` and `scene/npc`.
  It is compatibility-bearing and not safely deletable yet.
- Derived cache migration still has compatibility mirrors.
- UI exists as shell/models, not an authoring editor.
- Save/load exists for runtime state, but authored package roundtrip is not done.
- Render is command/resource/cache level, not product-grade presentation.
- Audio is effectively unbuilt.
- Physics is enough for current AABB movement/query constraints, not a broad
  simulation engine.

## Need

- Shared authoring facade for file/text/source-plan to lint/check/trace/run.
- Stable diagnostic projection over nested file/TOML/source/converter/profile/run
  diagnostics.
- Canonical fixture manifest and manifest-driven sweeps.
- Negative fixture catalog.
- Richer expectations: inventory, interaction state, player/NPC state, and
  per-frame trace expectations.
- Package boundary: layout, metadata, runner gate, package runner, and file/package
  parity.
- Editor readiness through read-only preview/diagnostics before mutation.
- Runtime cleanup after authoring contracts stabilize.
- Legacy NPC migration plan with explicit deletion prerequisites.
- Product runtime loop: package load, device input, gameplay tick, render
  presentation, save/load UX, and shell integration.
- Rendering backend/presentation layer.
- Audio server boundary.
- Gameplay semantics beyond fixtures: locked door/key, inspect/talk/event gates,
  region triggers, equipment, combat, progression, win/fail conditions.
- Performance and scale policy for authored content and runtime hot paths.

## Avoid / Defer

- Do not build UI/Edi before facade, package boundary, diagnostics, and read-only
  preview model.
- Do not add a generic scripting/event language to TOML.
- Do not add JSON or a TOML dependency unless a packet explicitly opens that
  policy.
- Do not delete `modules/npc_ai` until compatibility users and tests have a
  migration path.
- Do not expand runtime wrapper/report families without a cleanup gate.
- Do not persist derived caches as save truth.
- Do not add broad ECS/entity registry work yet.
- Do not build full physics simulation before gameplay needs exceed current
  movement/query constraints.
- Do not add combat/dialogue/quest systems before Authoring v1 and product-loop
  boundaries stabilize.

## Roadmap Phases

### 1. Authoring V1 Closure

Objective: make existing TOML single-file authoring stable and self-documenting.

Exit criteria:
- Canonical fixtures are self-contained and manifest-driven.
- CLI output and modes are stable.
- Diagnostics are predictable.
- Bucket and docs are current.

First packets:
- `07_mixed_scenario_pack`
- `13_fixture_ceremony_cleanup`
- `24_batch_queue_maintenance`
- `28_content_error_codes`

### 2. Authoring Facade And Diagnostics

Objective: move CLI orchestration into reusable engine APIs without changing
semantics.

Exit criteria:
- CLI is a thin shell over facade behavior.
- Facade supports run, lint, check, and trace.
- User-facing diagnostic entries are projected consistently.

First packets:
- `16_authoring_facade_api`
- `31_authoring_facade_parity_audit`
- `46_authoring_facade_mode_matrix`
- `47_authoring_diagnostic_entries`

### 3. Expectation And Fixture Hardening

Objective: broaden checks from final rows/counts to explicit state facts.

Exit criteria:
- Fixtures can assert inventory, interaction, actor/player, and trace facts.
- Negative fixture behavior is first-class.

First packets:
- `32_canonical_fixture_manifest`
- `33_trace_expectations`
- `34_negative_fixture_catalog`
- `56_inventory_expectations`
- `57_interaction_state_expectations`
- `58_actor_state_expectations`

### 4. Package Boundary

Objective: define multi-file/package authoring without editor or save/load creep.

Exit criteria:
- Package layout and metadata are approved.
- Package runner is gated and implemented only if approved.
- One-file and package scenarios have parity tests.

First packets:
- `14_authoring_package_layout`
- `17_package_runner_gate`
- `18_package_runner_implementation`
- `44_package_metadata_manifest`
- `51_file_package_parity`

### 5. Runtime/API Cleanup

Objective: reduce bloat after contracts stabilize.

Exit criteria:
- Duplicated wrapper/report/helper patterns are consolidated or marked
  intentional.
- Compatibility debt has explicit removal conditions.

First packets:
- `25_runtime_authoring_cleanup_gate`
- `59_authoring_run_summary_projection`
- `60_authoring_bucket_prune_gate`
- A focused runtime wrapper consolidation packet from evidence.

### 6. Editor Readiness

Objective: create read-only authoring preview models before UI mutation.

Exit criteria:
- Engine can load a scenario/package, expose diagnostics, preview final rows and
  trace, and surface supported schema information.

First packets:
- `15_editor_handoff_gate`
- `19_editor_preview_model_gate`
- `26_minimal_editor_model`
- `23_fixtures_from_cli_examples`

### 7. Gameplay Semantics V1

Objective: add small, gated gameplay semantics with fixture-backed checks.

Exit criteria:
- Each semantic has source-plan schema, validation, conversion, runtime behavior,
  and expectation coverage.

First packets:
- `08_locked_door_key_gate`
- `09_locked_door_key_implementation`
- `41_emit_event_semantics_gate`
- `42_talk_interaction_semantics_gate`
- `43_region_trigger_semantics_gate`

### 8. Product Loop And Content

Objective: move from engine harness to playable/editor-backed game loop.

Exit criteria:
- Package load -> input -> runtime frame -> render/presentation -> save/load works
  in an app shell.

First packets:
- Input binding gate.
- Render/presentation facade.
- Package-to-shell loader.
- Save-slot UX gate.

### 9. Systems Expansion

Objective: build missing game systems only after the loop is real.

Exit criteria:
- Combat, equipment, progression, encounter AI, audio, and UI menus have
  ownership gates and acceptance fixtures.

First packets:
- Combat semantics gate.
- Equipment/inventory gate.
- AI encounter budget gate.
- Audio server boundary.

## Near-Term Recommended Order

1. Finish Batch 09 locked door/key.
2. Pull forward Batch 16 authoring facade.
3. Add facade parity audit.
4. Add diagnostic projection.
5. Add canonical fixture manifest.
6. Add trace expectations.
7. Add inventory expectations.
8. Add interaction state expectations.
9. Add actor/player state expectations.
10. Add negative fixture catalog.
11. Add output contract snapshots.
12. Gate package layout.
13. Implement package runner if approved.
14. Add file/package parity.
15. Begin editor read-only preview model.

## Open Decisions

- Is Authoring v1 complete at CLI stability, or does it require the reusable
  facade?
- Should packages come before more gameplay semantics?
- Should editor preview happen before product loop?
- What is the first product shell target: CLI, Qt shell, or another app?
- When does legacy `modules/npc_ai` become active migration work instead of
  compatibility debt?
- How much runtime wrapper cleanup should happen before new gameplay systems?
- What is the first gameplay vertical after locked door/key?
- Is TOML the long-term hand-authored format or the v1/source-fixture format?
- What content size should v1 support?
- What does "near finished" mean for this project: engine, tooling, playable
  slice, or editor-backed game?

## Update Policy

- Update this file after project-shaping batches, not after every small slice.
- Keep short-term implementation details in `engine/research/authoring_batches/`.
- Keep API facts in `engine/research/api_index.md`.
- Keep compatibility-removal prerequisites in
  `engine/research/compatibility_debt.md`.
