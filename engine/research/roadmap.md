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
- Authoring V1 is release-candidate defined for engine/test/dev-tool use, with
  fixture-scale budgets and cleanup/prune gates recorded.
- Full verification has stayed green through the recent stretches; the latest
  Authoring V1 closure packet reported 341/341 tests.
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
- Authoring V1 release-candidate closure: `29`, `27`, `25`, `60`, `24`.

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
- Local Mac department orchestration is documented in
  `engine/research/local_mac_department_orchestration.md`: hub-led planning,
  local worktrees, builder/finisher/reviewer/researcher departments, apprentice
  scouts, merge gates, verification lanes, and context ticks.
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

- Keep API index current when facade, package, preview, check/lint/trace, or
  package CLI behavior changes.
- Pull the next authoring packet from `engine/research/authoring_batches/`
  unless a planner-scoped stretch overrides raw queue order.
- Use the local department workflow for substantial work: scope with
  reviewer/researcher, assign builder/finisher to isolated worktrees, and merge
  through the integration hub after focused plus integration verification.

### Remaining Authoring Work

- AI-map region fixture examples.
- Multi-actor/profile fixture coverage.
- Existing interaction effect fixture pack.
- ResourceId namespace policy.
- Frame ordering policy.
- Authoring warning-channel decision.
- Source-plan compatibility/migration gate.
- Authoring diff report.
- Product/source-plan size-limit policy gate if fixture budgets prove
  insufficient.

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

Status: complete for engine/test/dev-tool use.

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
- Release-candidate acceptance set.
- Deterministic fixture-scale budget checks.
- Runtime authoring cleanup gate.
- Batch queue prune gate.

Remaining adjacent work:
- AI-map region fixture examples.
- Multi-actor/profile fixture coverage.
- Existing interaction effect fixture pack.
- ResourceId namespace and frame-ordering policy.

### 2. Runtime/API Cleanup

Status: cleanup gate complete; implementation is not started.

Objective: reduce bloat after authoring contracts stabilized.

Exit criteria:
- Duplicated wrapper/report/helper patterns are consolidated or marked
  intentional.
- Count/report projection growth is contained.
- Legacy debt has explicit removal conditions.
- No gameplay behavior changes are introduced as cleanup.

Candidate packets:
- `59_authoring_run_summary_projection` is complete and should be treated as the
  first successful cleanup pattern.
- A focused authoring test/support deduplication packet from Batch 25 evidence.
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

1. Return to the bucket raw next packet, `11_ai_map_region_fixtures`, unless a
   planner-scoped stretch overrides raw order.
2. Keep no-new-semantics fixture/policy packets ahead of new gameplay semantics
   unless the planner explicitly opens a semantics gate.
3. Use Batch 25 evidence to plan any behavior-preserving authoring test/support
   cleanup before broad runtime wrapper work.
4. Gate save/load roundtrip only if a later productization stretch explicitly
   requires persistence proof.

## Open Decisions

- Which remaining fixture/policy packet should follow Authoring V1 closure:
  AI-map regions, multi-actor/profile coverage, existing interaction-effect
  fixtures, ResourceId policy, or frame-ordering policy?
- Is the first UI milestone a read-only Qt shell panel, a separate editor
  prototype, or no UI until product loop?
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
