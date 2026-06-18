# Iggy Roadmap

This is the internal project roadmap checkpoint. It is broader than the
authoring batch bucket and should be updated after project-shaping batches, not
after every small slice.

For the detailed completion backlog, use
`engine/research/project_completion_todo.md`.
For phase order and dispatch gates, use
`engine/research/project_completion_execution_plan.md`.

## Current Checkpoint

- The TOML authored-scenario lane has moved from a CLI harness into reusable
  runtime facades and projections.
- The package boundary is implemented as a thin wrapper over the single-file
  TOML scenario facade.
- The read-only authoring preview model exists for explicit TOML files and
  explicit package paths.
- The first read-only UI preview consumer is integrated in the existing Qt
  shell via explicit `--preview PATH` and `--preview-mode run|trace|check|lint`
  options.
- Authoring V1 is release-candidate defined for engine/test/dev-tool use, with
  fixture-scale budgets and cleanup/prune gates recorded.
- Full verification has stayed green through the recent stretches; the latest
  UI preview consumer integration reported 343/343 tests.
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
- Runtime `RuntimeGameplayTomlScenarioPackageReader` for shared explicit
  package directory/manifest path reading without package discovery or scanning.
- Runtime `RuntimeGameplayTomlScenarioPackageFacade` for explicit package
  directory or manifest paths, delegating to the TOML scenario facade.
- Runtime `RuntimeGameplayAuthoringPreviewModel` for read-only preview over TOML
  files and packages.
- Runtime `RuntimeGameplayProductScenarioLoader` for load-only product scenario
  setup over explicit TOML files, package directories, and `package.toml` paths;
  it reads, adapts, and validates without running frames.
- Runtime `RuntimeGameplayProductLoop` for product-owned one-frame stepping over
  successful loader output; it consumes caller-provided player input intents and
  carries current gameplay state without owning raw input, presentation, or
  persistence.
- Scene/player `PlayerInputBinding2D` for device-agnostic normalized action to
  `PlayerInputIntent2D` binding; it reports binding issues and preserves action
  order without applying gate rules or command mapping.
- Runtime `RuntimeGameplayProductInputAdapter` for product/app transient input
  events to normalized `PlayerInputBindingAction2D` actions plus carried binding
  context; it does not call the product loop, step gameplay, or own Qt/device,
  camera, render, save/load, gate, or command behavior.
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
- UI exists as shell/models plus a first read-only authoring preview panel in
  the Qt shell, but not as an authoring editor or playable product shell.
- Read-only preview model is consumed by the existing Qt shell for explicit
  TOML/package paths; source-linked diagnostics, visual trace playback, play
  shell integration, and editor mutation are still gated.
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
- Product scenario loading and product-owned one-frame stepping exist for
  explicit TOML/package paths, and the scene/player normalized input binding
  layer plus runtime/product transient input adapter exist; next product runtime
  work is optional Qt/raw-device adapter wiring, context integration decisions,
  presentation/camera adapter, pause/retry/reset policy, completion/failure
  evaluation, save/load UX, and shell integration.
- Rendering backend/presentation layer.
- Audio server boundary.
- Save/load productization and authored package roundtrip.
- Source-linked diagnostics and richer visual trace playback for the read-only
  preview panel.
- Product/play shell integration after the product-loop gate.
- Editor mutation only after explicit authoring roundtrip/source mutation gates.
- Gameplay semantics beyond the current set: inspect/talk/event gates, region
  triggers, equipment, combat, progression, win/fail conditions.
- Performance and scale policy for authored content and runtime hot paths.

## Avoid / Defer

- Do not turn the read-only preview consumer into UI/Edi mutation without an
  explicit source roundtrip/mutation gate.
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

Status: first read-only Qt shell preview consumer integrated; later UI/editor
work remains gated.

Objective: let tools preview authored TOML/package scenarios without mutation.

Done:
- `15_editor_handoff_gate`
- `19_editor_preview_model_gate`
- `26_minimal_editor_model`
- `23_fixtures_from_cli_examples`
- UI workspace profile notes are recorded in
  `engine/research/ui_integration_profiles.md`.
- Existing Qt shell accepts explicit `--preview PATH` plus
  `--preview-mode run|trace|check|lint` and renders a read-only
  `panel:authoring_preview`.
- `UiAuthoringPreviewPanelModel` projects
  `RuntimeGameplayAuthoringPreviewModel` into UI rows/sections without UI-owned
  parsing or mutation.

Remaining exit work:
- Add source-linked diagnostics and richer trace/expectation inspection.
- Decide when the preview panel should connect to a product play shell.
- Gate any build canvas, structured authoring controls, or source/TOML
  roundtrip separately.
- Keep editing/mutation APIs out until a separate gate approves them.

Likely next packets:
- A source-linked diagnostics projection/display packet.
- A visual trace playback packet.
- A product/play shell boundary packet.
- A future editor mutation/roundtrip gate only after read-only UI remains stable.

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

Status: Packets 1, 2, 3A, and 3B-A complete; product runtime loop has a load
boundary, caller-driven one-frame step, scene/player normalized input binding,
and a runtime/product input adapter for transient product input events, but no
Qt/raw-device shell adapter, presentation, UX policy, or save/load
productization yet.

Objective: move from engine harness to playable/editor-backed game loop.

Done:
- `RuntimeGameplayProductScenarioLoader` loads explicit TOML files, package
  directories, and `package.toml` paths into validated profile scenario
  definitions plus initial state.
- Loader is load-only: no frame execution, check/trace/final rows, raw input,
  presentation/camera, save/load productization, or UI behavior.
- Package manifest/path parsing is shared through
  `RuntimeGameplayTomlScenarioPackageReader`.
- `RuntimeGameplayProductLoop` builds product loop state from a successful load,
  starts at `nextFrameIndex = 0`, steps exactly one lowered scenario frame per
  call, replaces authored/scripted player frame intents with caller-provided
  `PlayerInputIntent2D`, runs `RuntimeGameplayOrchestratedFrameStep`, carries
  current state forward, and reports failed-load/not-loaded/exhausted-frame
  guard statuses.
- `PlayerInputBinding2D` maps device-agnostic normalized actions into
  `PlayerInputIntent2D`, carrying `PlayerInputContext2D` as data, preserving
  action order, reporting stable counts/issues, and excluding no-op/issues from
  emitted intents.
- `RuntimeGameplayProductInputAdapter` maps transient product input events into
  `PlayerInputBindingAction2D` actions plus carried `PlayerInputBindingContext2D`,
  preserving order and input immutability while reporting stable counts/issues
  for release/no-op policy, movement controls, target fallback handoff, payload
  validation, and unsupported controls.

Exit criteria:
- Load a package or explicit scenario.
- Bind device input to player intents.
- Run gameplay frames.
- Produce render frames.
- Present frames in an app shell.
- Save/load user-facing state.

First gates:
- Optional Qt/raw-device adapter into transient product input events.
- Product loop context integration decision if needed.
- Render/presentation ownership.
- Pause/retry/reset policy.
- Completion/failure evaluator.
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
