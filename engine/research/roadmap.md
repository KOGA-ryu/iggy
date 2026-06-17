# Iggy Roadmap

This is the internal project roadmap checkpoint. It is broader than the
authoring batch bucket and should be updated after project-shaping batches, not
after every small slice.

## Current Checkpoint

- The TOML authored-scenario lane has moved from a CLI harness into reusable
  runtime facades and projections.
- The package boundary is implemented as a thin wrapper over the single-file
  TOML scenario facade.
- The read-only authoring preview model exists for explicit TOML files and
  explicit package paths.
- Full verification has stayed green through the recent stretches; the latest
  preview model batch reported 340/340 tests.
- Short-term implementation packets live in
  `engine/research/authoring_batches/`.

Recently completed optimized stretches:
- Facade/diagnostics/output contracts: `16`, `31`, `46`, `59`, `28`, `47`,
  `48`.
- Fixture/expectation hardening: `32`, `49`, `33`, `56`, `57`, `58`, `34`.
- Source-plan contract hardening and cleanup: `21`, `22`, `39`, `52`, `38`,
  `13`, `24`.
- Package boundary: `14`, `17`, `18`, `44`, `50`, `51`, `24`.
- Editor read-only preview: `15`, `19`, `26`, `23`.

## Have

### Engine Foundation

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
  flow, drops, pickup policy, inventory transfer, and required-item interaction
  gating.
- Runtime gameplay/session/scenario infrastructure: command queues, player input,
  interaction, pickup, NPC movement, orchestrated frames, profile scenarios, and
  scenario runners.
- Save/load foundations: session/gameplay snapshots, chunk archive/envelope,
  file IO, save slots, and autosave rotation.

### Authoring And Scenario Execution

- TOML source-plan authoring lane with grid, legend, cells, regions, controls,
  profiles, interaction targets/effects, item drops, player commands, safety
  flags, promotion policy, expectations, and trace expectations.
- Source-plan format/version policy pinned to
  `format_id = "iggy:ascii-source-plan"` and `version = 1`.
- Test-owned TOML schema snapshot for supported tables/keys.
- TOML subset stress coverage for supported authoring patterns and unsupported
  shapes.
- Source-plan validator and TOML diagnostics with table/key/line/index metadata.
- Runtime authoring adapter and source-plan-to-profile-scenario converter.
- Region-to-AI-map promotion, profile scenario validation, profile scenario run,
  and final debug row projection.
- Runtime `RuntimeGameplayTomlScenarioFacade` for run/lint/check/trace over
  explicit one-file TOML scenarios.
- Runtime `RuntimeGameplayTomlScenarioPackageFacade` for explicit package
  directory or manifest paths, delegating to the TOML scenario facade.
- Runtime `RuntimeGameplayAuthoringPreviewModel` for read-only preview over TOML
  files and packages.
- Runtime authoring diagnostic projection with stable printable error-code
  strings.
- Runtime summary projection for CLI/facade summary and final rows.

### CLI, Fixtures, And Packages

- `iggy_scenario_toml_runner` supports explicit TOML files and explicit package
  directories/manifests.
- CLI modes: run, `--trace`, `--check`, and `--lint`.
- CLI output contracts are locked with path-normalized snapshots.
- Canonical fixture manifest and manifest-driven sweep tests.
- Canonical fixtures cover movement, multi-frame movement, player/NPC movement,
  interaction, pickup, mixed progression, locked door/key, collision/blocked
  movement, and package equivalents.
- Negative fixture catalog covers parser/type/source/conversion/profile-style
  failures through checked-in regression assets.
- Expectations cover final rows, summary counts, trace frames, inventory stacks,
  interaction target state, actor final tile, and player final tile.
- Package boundary is implemented with `package.toml`, one main scenario TOML,
  display-only metadata, fixture pack, and one-file/package parity tests.

### Planning And Workflow

- Authoring batch bucket with numbered packets and hard stops.
- Finisher worktree/bucket exists for docs/coordination/cleanup work.
- Roadmap, API index, compatibility debt notes, smell dashboard, and merge
  protocol documents exist.

## Sorta Have

- Runtime orchestration is broad and effective, but has many wrappers, counters,
  reports, and runners that can grow ceremony if not actively pruned.
- Legacy `modules/npc_ai` still coexists with newer `scene/ai` and `scene/npc`.
  It is compatibility-bearing and not safely deletable yet.
- Derived cache migration still has compatibility mirrors.
- Save/load works for runtime state, but authored package save/load roundtrip is
  not proven.
- UI exists as shell/models, but not as an authoring editor or playable product
  shell.
- Read-only preview model exists, but no editor UI consumes it yet.
- Render is command/resource/cache level, not product-grade presentation.
- Audio is effectively unbuilt.
- Physics is enough for current AABB movement/query constraints, not a broad
  simulation engine.
- Package mode is intentionally narrow: one manifest, one main scenario, no
  discovery/dependencies/assets.
- TOML remains a supported subset, not a full TOML implementation.

## Need

### Immediate Authoring Stabilization

- Update bucket status for recently completed editor-preview packets if not
  already reflected.
- Decide whether Authoring v1 is now release-candidate ready or needs remaining
  fixture packs/perf checks first.
- Run a prune/staleness pass over the batch queue and roadmap after the preview
  stretch.
- Keep API index current for facade, package, preview, check/lint/trace, and
  package CLI behavior.

### Remaining Authoring Work

- AI-map region fixture examples.
- Multi-actor/profile fixture coverage.
- Existing interaction effect fixture pack.
- ResourceId namespace policy.
- Frame ordering policy.
- Authoring warning-channel decision.
- Authoring size/performance budget.
- Source-plan compatibility/migration gate.
- Authoring diff report.
- Authoring release-candidate acceptance set.

### Product And Runtime Work

- Runtime cleanup after authoring contracts stabilize.
- Legacy NPC migration plan with explicit deletion prerequisites.
- Product runtime loop: package load, device input, gameplay tick, render
  presentation, save/load UX, and shell integration.
- Rendering backend/presentation layer.
- Audio server boundary.
- Save/load productization and authored package roundtrip.
- Editor UI consuming the read-only preview model.
- Gameplay semantics beyond the current set: inspect/talk/event gates, region
  triggers, equipment, combat, progression, win/fail conditions.
- Performance and scale policy for authored content and runtime hot paths.

## Avoid / Defer

- Do not build UI/Edi mutation before the read-only preview model is consumed
  safely.
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
- Do not add combat/dialogue/quest systems before product-loop and authoring
  boundaries stabilize.
- Do not make package mode a content management system: no recursive discovery,
  dependencies, asset registry, or multi-scenario package semantics yet.

## Roadmap Phases

### 1. Authoring V1 Closure

Status: mostly complete.

Objective: make TOML single-file and package authoring stable,
self-documenting, and self-checking.

Done:
- Facade-backed run/lint/check/trace.
- Stable CLI output contracts.
- Diagnostic and summary projections.
- Canonical fixture manifest and manifest sweep.
- Final-row, trace, inventory, interaction, actor, and player expectations.
- Negative fixture catalog.
- Source-plan version/schema/subset hardening.
- Package runner/metadata/parity.
- Read-only preview model.

Remaining exit work:
- Decide Authoring v1 release-candidate acceptance.
- Add any final missing fixture packs that are needed for confidence.
- Update stale docs/API index.
- Prune or defer remaining authoring bucket packets.

Likely next packets:
- `29_release_candidate_authoring_v1`
- `24_batch_queue_maintenance`
- `60_authoring_bucket_prune_gate`
- `27_authored_scenario_perf_budget`

### 2. Runtime/API Cleanup

Status: ready for a gate, not for broad deletion.

Objective: reduce bloat after authoring contracts stabilized.

Exit criteria:
- Duplicated wrapper/report/helper patterns are consolidated or marked
  intentional.
- Count/report projection growth is contained.
- Legacy debt has explicit removal conditions.
- No gameplay behavior changes are introduced as cleanup.

Candidate packets:
- `25_runtime_authoring_cleanup_gate`
- `59_authoring_run_summary_projection` is complete and should be treated as the
  first successful cleanup pattern.
- A focused counter/report projection cleanup packet from smell-audit evidence.
- A focused CMake/test registration hygiene packet if useful.

Hard stops:
- Do not delete compatibility-bearing `modules/npc_ai`.
- Do not alter save/load chunks.
- Do not change CLI output contracts as cleanup.

### 3. Editor Readiness

Status: backend model complete; UI handoff not started.

Objective: let tools preview authored TOML/package scenarios without mutation.

Done:
- `15_editor_handoff_gate`
- `19_editor_preview_model_gate`
- `26_minimal_editor_model`
- `23_fixtures_from_cli_examples`
- UI workspace profile notes are recorded in
  `engine/research/ui_integration_profiles.md`.

Remaining exit work:
- Decide first consumer: CLI-only preview inspection, Qt shell read-only panel,
  or separate editor prototype.
- Add UI-facing examples only if a consumer exists.
- Keep editing/mutation APIs out until a separate gate approves them.

Likely next packets:
- A read-only preview consumer gate.
- A UI ownership gate for Qt shell vs separate editor.
- A preview-model API index update.

### 4. Package And Content Pipeline

Status: narrow package boundary complete.

Objective: support explicit authored scenario packages without becoming a package
manager.

Done:
- Layout proposal.
- Package runner gate.
- Package runner implementation.
- Metadata manifest.
- Package fixture pack.
- File/package parity.

Remaining exit work:
- Package save/load roundtrip decision.
- Package examples/docs polish.
- Package compatibility/version policy only if package content evolves.

Likely next packets:
- `12_authored_scenario_save_load_roundtrip` only after a narrow gate.
- `45_authoring_compatibility_migration_gate`.
- Package-to-product-shell loader gate later.

### 5. Gameplay Semantics V1

Status: one narrow semantic landed; remaining semantics need gates.

Objective: add small, fixture-backed gameplay semantics without creating a
scripting system.

Done:
- Locked door/key through required-item interaction gating.

Candidate gates:
- `41_emit_event_semantics_gate`
- `42_talk_interaction_semantics_gate`
- `43_region_trigger_semantics_gate`
- `53_terrain_authoring_policy_gate`

Hard stops:
- No generic condition language.
- No event bus framework unless explicitly approved.
- No dialogue trees, quests, or combat until product-loop boundaries exist.

### 6. Product Loop

Status: not started.

Objective: move from engine harness to playable/editor-backed game loop.

Exit criteria:
- Load a package or explicit scenario.
- Bind device input to player intents.
- Run gameplay frames.
- Produce render frames.
- Present frames in an app shell.
- Save/load user-facing state.

First gates:
- Product shell target: CLI harness, Qt shell, or separate app.
- Input binding ownership.
- Render/presentation ownership.
- Save-slot UX ownership.

### 7. Systems Expansion

Status: deferred.

Objective: build missing game systems only after the product loop is real.

Candidate gates:
- Audio server boundary.
- Equipment/inventory expansion.
- Combat/damage.
- Progression/win/fail conditions.
- Encounter AI budget.
- UI menus.

## Near-Term Recommended Order

1. Synchronize roadmap/API/bucket docs after the preview stretch.
2. Run `29_release_candidate_authoring_v1` to define the Authoring v1 acceptance
   set.
3. Run `27_authored_scenario_perf_budget` to set lightweight size/cost checks.
4. Run `25_runtime_authoring_cleanup_gate` to choose the next cleanup seam.
5. Run `60_authoring_bucket_prune_gate` to remove stale/superseded queue items.
6. Decide whether to do a small fixture pack (`11`, `54`, `55`) or move to
   product-loop/editor-consumer gating.
7. Gate save/load roundtrip only if Authoring v1 requires persistence proof.

## Open Decisions

- Is Authoring v1 now release-candidate ready, or does it need AI-map,
  multi-actor, interaction-effect, and perf-budget packets first?
- Is the first UI milestone a read-only Qt shell panel, a separate editor
  prototype, or no UI until product loop?
- Should authored packages be part of Authoring v1, or an adjacent tooling
  feature?
- Do save/load roundtrip tests belong in Authoring v1 or later productization?
- When does legacy `modules/npc_ai` become active migration work instead of
  compatibility debt?
- How much runtime wrapper cleanup should happen before new gameplay semantics?
- Which semantic gate is highest priority: emit event, talk, region trigger, or
  terrain policy?
- Is TOML the long-term hand-authored format or the v1/source-fixture format?
- What authored content size should v1 support?
- What does "near finished" mean for this project: engine, tooling, playable
  slice, or editor-backed game?

## Update Policy

- Update this file after project-shaping batches, not after every small slice.
- Keep short-term implementation details in `engine/research/authoring_batches/`.
- Keep API facts in `engine/research/api_index.md`.
- Keep compatibility-removal prerequisites in
  `engine/research/compatibility_debt.md`.
