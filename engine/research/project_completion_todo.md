# Iggy Project Completion TODO

This is the broad project completion backlog. It is intentionally larger than
the authoring packet bucket. Use it to decide what to scout, gate, build,
finish, or defer as the project moves from engine tooling toward a finished
playable/editor-backed product.

Short-term packet queues can still live in focused buckets such as
`authoring_batches/` and `finisher_batches/`. This document is the higher-level
source of truth for what remains.

For phase order, dispatch templates, and verification gates, use
`engine/research/project_completion_execution_plan.md`.

## Operating Rules

- Keep the old thread/packet workflow unless a new orchestration setup is
  explicitly rebuilt and proven.
- Head planner owns total roadmap and dispatch.
- Researcher/reviewer scout semantics, ownership, file paths, compute cost, and
  verification before risky work.
- Builder receives bounded implementation packets only.
- Finisher owns cleanup, docs, test-support extraction, and behavior-preserving
  reduction.
- Integration happens on `/Users/kogaryu/iggy` after focused verification and
  merge review.
- Do not bother the user for small "yes, continue" decisions. Escalate only
  when a semantic/product choice changes project direction or a real blocker
  appears.

## Status Buckets

### Complete Enough To Stand On

- Engine CMake/test foundation with focused targets and full CTest gates.
- Core math/resource/server primitives for render, navigation, and physics
  queries.
- Level/map/cache foundations: tile map, runtime state, render cache,
  collision cache, derived cache updates, and mutation helpers.
- Scene player intent/command planning foundations.
- Scene AI/NPC profile traits, AI maps, actor state, control state, and
  movement planning/application lanes.
- Interaction and inventory foundations: targets, effects, reach/query/plan,
  drops, pickup policy, inventory transfer, and required-item interaction
  gating.
- Runtime gameplay frame/session/scenario infrastructure.
- Save/load foundations for session/gameplay snapshots, archives, save slots,
  and autosave rotation.
- Authoring V1 single-file TOML path: source-plan parse, validate, convert,
  run, trace, check, lint, diagnostics, and expectations.
- Package path: manifest, display metadata, one main scenario, facade
  delegation, examples, and file/package parity.
- Read-only authoring preview model over explicit TOML file paths and explicit
  package paths.
- CLI harness with stable `status:`, `summary:`, `expectation:`,
  `final_rows:`, `frames:`, and package sections.
- Canonical and negative fixture catalogs.
- Fixture budgets and schema/subset/version tests.
- UI integration profile note for read-only preview and later profile tabs.

### Sorta Have, Needs Tightening

- Runtime orchestration works but still has many wrappers, reports, counters,
  and mirroring layers.
- Legacy `modules/npc_ai` still coexists with newer `scene/ai` and
  `scene/npc`; it is active compatibility state, not dead code.
- Authoring lane is powerful but large inside `runtime`.
- TOML is a focused supported subset, not a complete TOML implementation.
- Package mode is deliberately narrow and not a package manager.
- Preview model exists but no UI consumes it yet.
- UI/product shell is conceptually planned, not implemented.
- Render is command/cache oriented, not a polished presentation backend.
- Audio is not established.
- Physics is query/AABB/movement support, not a broad simulation engine.
- Save/load exists, but authored package save/load roundtrip is not proven.
- Test count is healthy but large enough that focused runs matter.

### Needs Further Detailing Before Build

- What "finished" means for the first playable slice.
- First UI consumer: existing Qt shell, separate authoring tool, or CLI-only
  for another stretch.
- Runtime product loop ownership: who owns package load, input, tick, render,
  save/load, and app shell.
- Legacy NPC migration plan and exact deletion prerequisites.
- Whether `RuntimeGameplayState` becomes the durable top-level gameplay packet.
- Save/load ownership for interaction/inventory/authored scenario state.
- Region trigger semantics: no-go, AI metadata only, or explicit trigger fact.
- Talk target semantics: metadata, inspect text, report-only, or later dialog.
- Emit-event semantics: metadata, report-only event, or no-go.
- Terrain authoring policy: walkability only vs typed terrain metadata.
- ResourceId namespace policy: convention vs validation.
- Frame ordering policy across authored NPC/player commands.
- Authoring warning channel policy.
- Source-plan/package compatibility and migration policy.
- Product performance budget and content scale target.

### Needs Building

- Behavior-preserving runtime cleanup packets.
- Legacy NPC compatibility map and follow-up migration packets.
- Remaining authoring fixture/policy packets that are still useful.
- Read-only UI preview consumer.
- Product runtime loop.
- Presentation/render integration.
- User-facing save/load flow.
- Audio boundary.
- Later gameplay systems: inspect/talk/event, region triggers, equipment,
  combat, progression, win/fail conditions.
- Editor/build surface after read-only preview is working.

## Immediate Recommended Work

### 1. Runtime Cleanup Scout

Goal: pick one narrow bloat-reduction packet that does not change behavior.

Owner: researcher/reviewer scout, then finisher.

Tasks:
- Re-scan runtime wrapper/report/count duplication.
- Identify one file boundary where projection is copied mechanically.
- Confirm affected tests and public result fields.
- Write a finisher packet with exact files, non-goals, and verification.
- Build only after reviewer says the seam is behavior-preserving.

Hard stops:
- No save/load format changes.
- No CLI output changes unless output-contract tests are part of the packet.
- No removal of `modules/npc_ai`.
- No gameplay semantics.

### 2. Legacy NPC Compatibility Map

Goal: turn "old NPC lane is still real" into an actionable migration map.

Owner: researcher/reviewer first.

Tasks:
- Inventory `npc_ai::NpcAgentEntry`, `NpcAgentTickConfig`, and
  `modules/npc_ai` references across source, tests, and CMake.
- Group references by blocker: session state, save/load, render projection,
  runtime tick, test-only helper, docs.
- Identify the smallest wrong-direction dependency still removable.
- Propose deletion prerequisites for each old module component.

Hard stops:
- No code deletion in the scout.
- Do not touch snapshot/save chunks.
- Do not migrate `LevelRuntimeState::npcAgents` without a separate gate.

### 3. UI Preview Consumer Gate

Goal: decide the first UI milestone now that the preview model exists.

Owner: UI/Product planner + reviewer.

Tasks:
- Confirm whether the first consumer is the existing Qt shell or a separate
  authoring preview app.
- Define the exact read-only data displayed from
  `RuntimeGameplayAuthoringPreviewModel`.
- Map far-left workspace profiles to product surfaces.
- Define invalid/read/conversion/run/check states.
- Decide whether source-linked diagnostics are in milestone 1 or 2.

Hard stops:
- No editing or TOML mutation.
- No file watching.
- No package scanning.
- No UI-owned parsing.

### 4. Authoring Maintenance Sweep

Goal: keep Authoring V1 stable while broader work resumes.

Owner: builder only when a concrete packet exists; finisher for docs/test
cleanup.

Tasks:
- Mark `11_ai_map_region_fixtures` status accurately if merged.
- Finish or prune stale authoring batch entries.
- Add ResourceId namespace and frame-ordering policy only if still open.
- Keep fixture README and manifest aligned.
- Keep API index current for facade/package/preview/diagnostic surfaces.

Hard stops:
- No new TOML dependency.
- No broad scripting.
- No package discovery/dependency management.

## Runtime And Systems TODO

### Runtime Reports, Runners, And Counts

- Audit count fields in runtime scenario, frame, orchestrated, NPC movement,
  ledger, reporter, facade, and summary layers.
- Extract projection helpers only when they replace literal copy blocks.
- Preserve public result structs until call sites migrate.
- Add parity tests before removing any mirrored count.
- Track line count in affected files before/after cleanup.
- Keep runtime reports readable; do not collapse diagnostics into opaque blobs.

Candidate files to scout:
- `engine/src/runtime/RuntimeGameplayScenarioRunner.*`
- `engine/src/runtime/RuntimeGameplayScenarioLedger.*`
- `engine/src/runtime/RuntimeGameplayFrameReport.*`
- `engine/src/runtime/RuntimeGameplayFrameRunner.*`
- `engine/src/runtime/RuntimeGameplayOrchestratedFrame*`
- `engine/src/runtime/RuntimeNpcOrchestrationAggregates.*`
- `engine/src/runtime/RuntimeGameplayTomlScenarioSummaryProjection.*`

### Runtime Product Loop

- Define a top-level product loop packet.
- Load explicit package or TOML scenario through existing facades.
- Bind raw input to existing player intents outside core runtime.
- Run frames on a caller-controlled tick.
- Produce render-frame data from gameplay/session state.
- Present render commands in an app shell.
- Surface diagnostics/check failures in product UI.
- Decide where pause/retry/reset state lives.
- Decide how scenario completion/failure is represented.

Hard stops:
- Do not hide input binding inside save/session state.
- Do not make authoring facade responsible for product runtime loop.
- Do not persist derived caches as truth.

### Save/Load And Persistence

- Gate authored scenario save/load roundtrip.
- Decide whether interaction targets/effects and inventory drops become saved
  gameplay state.
- Keep existing snapshot chunk compatibility intact.
- Add package path/metadata persistence only if product loop needs it.
- Add explicit save-slot UX policy before UI work.
- Avoid cloud sync/compression/encryption until product need exists.

### Render And Presentation

- Define presentation state: camera, viewport, zoom, debug overlays.
- Decide whether camera state is runtime-owned or sibling presentation state.
- Add caller-driven render-frame step from gameplay state and camera state.
- Keep render frame generation out of gameplay tick unless explicitly approved.
- Add visual/debug overlay policy for final rows, trace, AI maps, collision,
  paths, and interaction targets.

### Audio

- Gate audio server ownership.
- Decide immediate audio needs: UI sounds, pickup/interaction feedback,
  movement, ambience, or none.
- Keep audio out of core gameplay state until events/reports can drive it.
- Add no audio code until one product surface needs it.

### Physics And Navigation

- Keep current AABB/query/movement policy stable.
- Add benchmarks only if movement/query cost becomes a blocker.
- Avoid broad dynamic-body simulation.
- Keep navigation server NPC-agnostic.
- Gate any future LevelTileMap/navigation inversion separately.

## AI / NPC TODO

### AI Map And Regions

- Keep region AI-map promotion explicit unless product semantics require
  defaults.
- Add or confirm regression-only region fixtures.
- Decide if regions remain AI/editor metadata or become runtime triggers.
- Keep trigger/proximity behavior out until a gate approves it.

### Profiles, Traits, And Multi-Actor Coverage

- Add small multi-NPC fixtures:
  - shared profile;
  - distinct profiles;
  - missing/unknown profile negative case if not already covered.
- Keep profile traits data-only.
- Avoid inheritance/ranks/combat stats until approved.

### NPC Behavior And Movement

- Review next-slice backlog for NPC play-to-control and trait-card pipeline.
- Keep AI decision ownership in `scene/ai`.
- Keep actor/control mutation ownership in `scene/npc`.
- Keep runtime sequencing explicit.
- Do not merge old `modules/npc_ai` and new actor/control lanes without a
  compatibility gate.

### Collision And Reservation

- Preserve current blocked/same-destination fixture behavior.
- Do not change reservation/capacity semantics as cleanup.
- Add clearer policy only after gameplay need appears.

## Authoring TODO

### Stable Contract Maintenance

- Keep format id and version pinned.
- Keep schema snapshot tests aligned with supported tables/keys.
- Keep TOML subset docs aligned with parser behavior.
- Keep negative fixtures named and documented.
- Keep canonical fixtures self-contained and manifest-backed.
- Keep package fixtures parallel with one-file fixtures.

### Remaining Policy Packets

- ResourceId namespace policy.
- Frame ordering policy.
- Authoring warning-channel gate.
- Authoring compatibility/migration gate.
- Authoring size-limit gate if fixture budget checks become insufficient.
- No-hidden-defaults audit after new canonical fixtures.

### Semantics Gates

- Emit event:
  - Decide metadata-only, report-only, or no-go.
  - No event bus framework.
- Talk:
  - Decide metadata-only, inspect text, or future dialog hook.
  - No dialogue trees or localization.
- Regions:
  - Decide AI/editor metadata vs explicit trigger facts.
  - No continuous proximity scanning hidden in actor logic.
- Terrain:
  - Decide walkability-only vs typed metadata.
  - No material/biome/damage semantics in the gate.

## UI / Product TODO

### First Read-Only UI Milestone

- Input: explicit TOML file path or explicit package path.
- Use `RuntimeGameplayAuthoringPreviewModel`.
- Display package metadata when present.
- Display read/adapt/convert/run/check status.
- Display diagnostics with source location.
- Display final rows.
- Display trace frames.
- Display expectation results.
- Display summary counts.
- Display actor/player final state if available.
- Display interaction target state and inventory expectations if available.

Hard stops:
- No editing.
- No save mutation.
- No file watcher.
- No package scanning.
- No UI-owned parser.
- No new gameplay semantics.

### Far-Left Workspace Profiles

- Play:
  - World view.
  - Player/NPC/item/door presentation.
  - Minimal HUD.
  - Inventory quick view.
  - Interaction prompt.
- Build:
  - Spatial placement canvas.
  - Terrain/wall/floor placement.
  - Player start, NPCs, item drops, interaction targets, regions.
- Script:
  - Frame flow.
  - Player commands.
  - NPC controls.
  - Interaction/pickup timing.
  - Frame ids/order.
- Check:
  - Lint/check/run status.
  - Expected vs actual.
  - Diagnostics.
  - Final rows and trace playback.
- Actors:
  - Actor list.
  - Profiles/traits.
  - Positions.
  - Control preview.
- Items:
  - Drops.
  - Pickup targets.
  - Inventory state/expectations.
- Interactions:
  - Targets/effects.
  - Enabled/disabled.
  - Required item ids.
- Package:
  - Title, description, version, main scenario path.
  - Package diagnostics.
- Debug:
  - Runtime state tree.
  - Command queue.
  - NPC control registries.
  - Interaction/inventory state.
  - AI map, collision, path, save/load overlays.
- Docs / Examples:
  - Canonical fixtures.
  - Regression fixtures.
  - Package examples.
  - TOML table/key shape.

### Later UI Milestones

- Source-linked diagnostics.
- Visual trace playback.
- Read-only package browser for explicitly opened package only.
- Build canvas for placement.
- Structured controls for existing facts.
- Source/TOML roundtrip gate.
- Product play shell.

## Package And Content Pipeline TODO

- Keep package runner explicit-path only.
- Keep metadata display-only.
- Add package compatibility/version policy only when package shape changes.
- Gate package save/load roundtrip.
- Add package-to-product-shell loader later.
- Avoid recursive discovery, dependencies, asset registries, and multi-scenario
  package semantics.

## Testing And Verification TODO

- Continue focused target tests per slice.
- Full configure/build/CTest at batch end or integration.
- Track total CTest count.
- Track runtime/npc/scenario/ascii/movement/save test counts.
- Keep CLI output-contract snapshots narrow and stable.
- Keep manifest sweep separate from targeted negative diagnostics.
- Keep lower-level parser/converter tests for diagnostics and override
  behavior.
- Remove duplicate fixture ceremony only when manifest/CLI coverage fully owns
  the same behavior.
- Add labels only if they reduce repeated full-suite cost without hiding
  coverage.

## Platform / Workflow TODO

- Keep old thread/packet workflow.
- Do not restart native Terminal department automation without a fresh gate.
- Maintain clean worktrees before dispatch.
- Use separate worktrees for real branch work.
- Keep builder packets bounded by files, behavior, tests, and hard stops.
- Keep reviewer packets read-only unless explicitly docs-only.
- Use finisher for behavior-preserving cleanup and roadmap sync.
- Keep batch-end briefs concise:
  - commits;
  - files;
  - behavior;
  - verification;
  - boundaries;
  - next/blocker.

## Integration TODO

- Before merge:
  - check branch status;
  - inspect `git diff --name-status`;
  - run focused tests;
  - get reviewer gate when ownership or semantics matter.
- During merge:
  - preserve unrelated user changes;
  - resolve conflicts by current roadmap, not stale branch docs.
- After merge:
  - `cmake -S engine -B engine/build`;
  - `cmake --build engine/build`;
  - `ctest --test-dir engine/build --output-on-failure`;
  - `git diff --check`;
  - reference/forbidden scans;
  - roadmap/bucket sync if project-shaping.

## Metrics To Track

- `ctest --test-dir engine/build -N` total.
- Runtime/npc/scenario/ascii/movement/save test counts.
- `RuntimeGameplayAscii*` file count and total lines.
- `RuntimeGameplayScenario*` file count.
- `RuntimeNpc*` file count.
- `IggyScenarioTomlRunner.cpp` line count.
- TOML reader/converter/validator line counts.
- Fixture file count and total fixture lines.
- `npc_ai::NpcAgent`, `NpcAgentTickConfig`, `modules/npc_ai` reference count.
- Count/report projection reference count.
- Open authoring/finisher packet count.

## Suggested Work Order

1. Runtime cleanup scout.
2. One finisher cleanup packet from that scout.
3. Integration gate/full verification.
4. Legacy NPC compatibility map.
5. UI preview consumer gate.
6. First read-only UI preview consumer.
7. Authoring maintenance sweep for remaining policy/fixture packets.
8. Product loop gate.
9. Caller-driven presentation/render-frame step.
10. Save/load productization gate.
11. Gameplay semantics gate selection: emit event, talk, region, or terrain.
12. One tiny approved gameplay semantic with fixtures.
13. Product playable slice definition.
14. Product playable slice implementation.
15. Near-finished reassessment and backlog prune.

## Hard No For Now

- No generic scripting language.
- No broad UI editor/mutation.
- No save/load format migration as cleanup.
- No deletion of `modules/npc_ai`.
- No TOML dependency.
- No JSON output mode.
- No recursive package discovery.
- No package dependency resolver.
- No broad runtime wrapper deletion.
- No combat/dialogue/quest system without gates.
- No automatic runtime autorun hidden in authoring tools.
- No new Terminal/tmux department automation until separately approved.
