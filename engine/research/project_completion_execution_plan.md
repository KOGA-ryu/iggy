# Iggy Project Completion Execution Plan

This plan converts `project_completion_todo.md` into an execution system. The
first finish line is a playable scenario: authored content loads through the
engine tooling, runs in a product loop, renders through a UI shell, and supports
the minimal save/load state needed for that slice. Editor-backed authoring comes
after that.

## Finish Line

Primary target: playable scenario first, editor-backed game second.

Playable scenario means:
- an explicit authored package is the content unit;
- package read/lint/check errors are visible before play;
- runtime can load the package into a product loop;
- player input can drive existing player intents;
- gameplay frames advance under caller control;
- render/presentation state can show the world, player, NPCs, items,
  interactions, and debug overlays when enabled;
- the slice can save/load the state needed to resume;
- tests prove the path without relying on manual UI use.

Editor-backed game means:
- the UI consumes the read-only preview model first;
- authoring mutation is gated later;
- far-left profiles can expose Play, Build, Script, Check, Actors, Items,
  Interactions, Package, Debug, and Docs/Examples without making UI own runtime
  truth.

## Execution Model

Use the old thread/packet workflow.

Roles:
- Head planner: owns this plan, roadmaps, phase order, worker dispatch, and
  integration decisions.
- Researcher: read-only scans, ownership maps, compute/cost notes, prior art,
  and risk surfaces.
- Reviewer: gates semantics, ownership, file boundaries, verification, and
  merge fit.
- Builder: implements bounded packets after planner/reviewer scope.
- Finisher: behavior-preserving cleanup, docs, test-support extraction, and
  bloat reduction.

Default chain:
```text
head planner
  -> researcher/reviewer scout if risk is unclear
  -> bounded packet
  -> builder or finisher
  -> focused verification
  -> reviewer/integration gate if needed
  -> merge/full verification
  -> roadmap sync
```

Do not restart native Terminal/tmux department automation as part of this plan.

## Phase 0: Baseline And Scout Prep

Status: ready.

Goal: make the next implementation packet evidence-backed.

Packets:
1. Runtime cleanup scout.
2. Legacy NPC compatibility map.
3. UI preview consumer gate. Complete for Milestone 1; later UI milestones
   remain gated.
4. Authoring maintenance status sweep.

Verification:
- Docs/scouts: `git diff --check`.
- If no code changes, no CMake/CTest required.

Exit criteria:
- One finisher-ready runtime cleanup packet exists.
- One legacy NPC migration map exists.
- UI preview Milestone 1 is integrated, and later UI milestones remain gated.
- Authoring bucket status does not conflict with roadmap.

## Phase 1: Runtime Cleanup And Debt Containment

Status: starts after Phase 0 scout.

Goal: reduce bloat without behavior changes before product-loop work builds on
runtime surfaces.

Likely packets:
1. Report/count duplication cleanup:
   - extract or consolidate one literal copy/projection path;
   - preserve public fields;
   - add parity tests.
2. Authoring test-support trim:
   - remove redundant lower-level ceremony only where manifest/CLI coverage
     owns equivalent behavior;
   - keep diagnostics/override tests.
3. CMake/test registration hygiene:
   - group or clarify tests only after active test additions slow down.

Candidate files to scout:
- `engine/src/runtime/RuntimeGameplayScenarioRunner.*`
- `engine/src/runtime/RuntimeGameplayScenarioLedger.*`
- `engine/src/runtime/RuntimeGameplayFrameReport.*`
- `engine/src/runtime/RuntimeGameplayFrameRunner.*`
- `engine/src/runtime/RuntimeGameplayOrchestratedFrame*`
- `engine/src/runtime/RuntimeNpcOrchestrationAggregates.*`
- `engine/tests/support/*`
- `engine/cmake/iggy_runtime_tests.cmake`

Hard stops:
- no save/load format changes;
- no CLI output changes unless output-contract tests are updated intentionally;
- no runtime behavior changes;
- no deletion of `modules/npc_ai`.

Focused verification:
- affected runtime scenario/frame/orchestrated tests;
- summary/facade/CLI output-contract tests if projection is touched;
- `git diff --check`.

Batch-end verification:
```sh
cmake -S engine -B engine/build
cmake --build engine/build
ctest --test-dir engine/build --output-on-failure
git diff --check
```

## Phase 2: Legacy NPC Compatibility Plan

Status: scout first, implementation later.

Goal: identify exactly what blocks old NPC module removal and remove only
accidental dependencies.

Packets:
1. Compatibility map:
   - group all `npc_ai::NpcAgentEntry`, `NpcAgentTickConfig`, and
     `modules/npc_ai` uses by ownership.
2. Wrong-direction dependency cleanup:
   - choose one non-save/session dependency that can move to scene/runtime
     ownership.
3. Deletion prerequisite list:
   - define what must migrate before each old module file can disappear.

Likely blockers:
- `LevelRuntimeState::npcAgents`;
- runtime tick/session flow;
- session snapshot chunk codec;
- save/load tests;
- render projection compatibility;
- old module tests.

Hard stops:
- no deletion of `modules/npc_ai`;
- no snapshot/save chunk changes;
- no actor-state migration without a separate gate.

Focused verification:
- old NPC module tests;
- runtime session/save tests if touched;
- render-frame tests if projection is touched.

## Phase 3: UI Preview Consumer

Status: Milestone 1 implemented and integrated in the existing Qt shell;
remaining UI/editor milestones stay gated.

Goal: consume existing read-only preview model in a UI surface without creating
editor mutation.

Milestone 1 completed:
- first consumer is the existing Qt shell;
- `iggy_qt_shell --preview PATH` accepts one explicit TOML file or package path;
- `--preview-mode run|trace|check|lint` selects the read-only preview mode;
- `UiAuthoringPreviewPanelModel` projects
  `RuntimeGameplayAuthoringPreviewModel` into UI sections/rows;
- `panel:authoring_preview` renders the read-only preview panel.

Milestone 1 display surface:
- selected path;
- package metadata when present;
- read/adapt/convert/run/check status;
- diagnostics with source location;
- summary counts;
- final rows;
- trace frames;
- expectation result;
- actor/player final state where available;
- interaction target and inventory expectation state where available.

Remaining later UI milestones:
- source-linked diagnostics;
- richer visual trace playback and expectation inspection;
- product/play shell boundary;
- build canvas and structured authoring controls only after separate gates;
- source/TOML roundtrip only after explicit mutation/roundtrip approval.

Hard stops:
- no editing;
- no TOML/source mutation;
- no file watcher;
- no package scanning;
- no UI-owned parsing;
- no new gameplay semantics.

Focused verification:
- preview model tests;
- UI model/projection tests;
- shell smoke for explicit preview options;
- screenshot/browser/UI checks only when an interactive visual QA packet opens
  that scope.

Exit criteria:
- a user can open one explicit TOML/package path and inspect status/diagnostics
  and final/trace state from the UI.

## Phase 4: Product Loop Gate

Status: Packets 1, 2, 3A, 3B-A, 4-A, 4-B, and the app-neutral play-surface
frame complete; next work is product shell/play mode and focused input
ownership, optional Qt/raw-device event adaptation after shell/focus ownership
is scoped, camera lifecycle policy, render projection gaps, automatic app/tick
loop ownership, and first-play UX policy gates.

Goal: define and build the first playable loop boundary without making authoring
or session state own product concerns.

Packet 1 complete:
- `RuntimeGameplayProductScenarioLoader` supports explicit TOML files, package
  directories, and `package.toml` paths.
- The loader reads TOML, adapts through
  `RuntimeGameplayScenarioAuthoringAdapter`, validates with
  `RuntimeGameplayProfileScenarioValidator`, and exposes
  `RuntimeGameplayProfileScenarioDefinition` plus initial state.
- `RuntimeGameplayTomlScenarioPackageReader` owns shared package
  manifest/path reading for the package facade and product loader.
- The loader is load-only: no frame execution, final rows, trace, check,
  expectation comparison, CLI output, UI, raw input mapping, render/camera,
  save/load, package discovery/scanning/watching, source mutation, or new
  gameplay semantics.

Packet 2 complete:
- `RuntimeGameplayProductLoop` builds product loop state from a successful
  `RuntimeGameplayProductScenarioLoadResult`.
- Product loop state copies identity/package metadata, definition, lowered
  scenario from `load.validation.build.scenario`, initial/current state, and
  starts at `nextFrameIndex = 0`.
- Each step consumes exactly one lowered frame, replaces authored/scripted
  player frame intents with caller-provided `PlayerInputIntent2D`, runs one
  `RuntimeGameplayOrchestratedFrameStep`, carries current state forward, and
  increments `nextFrameIndex`.
- Guard statuses cover failed load, not-loaded state, and exhausted frames.
- The explicit collision-world overload delegates to existing frame-step
  behavior without building or caching worlds.

Packet 3A complete:
- `PlayerInputBinding2D` maps device-agnostic normalized actions to
  `PlayerInputIntent2D`.
- `PlayerInputContext2D` is carried through as data.
- Supported action shapes include move point/tile/delta, interact/inspect target
  fallback, wait, cancel, no-op, and issue reporting.
- Target fallback order is explicit action target, then selected target, then
  hovered target; missing target emits `MissingTarget` and no intent.
- Tile delta requires current player tile; missing current tile emits
  `MissingCurrentPlayerTile` and no intent.
- Emitted intents preserve action order and exclude no-op/issues.
- Binding does not apply gate rules or command mapping; `Inspect` and `Cancel`
  remain intents only.

Packet 3B-A complete:
- `RuntimeGameplayProductInputAdapter` maps transient product input events to
  `PlayerInputBindingAction2D` actions plus carried
  `PlayerInputBindingContext2D`.
- The adapter is device-agnostic runtime/product surface, not Qt shell wiring or
  product shell launch/play mode.
- Release/no-op policy, cardinal movement controls to tile deltas,
  interact/inspect target copy or fallback handoff, wait/cancel, primary
  tile/point payload validation, unsupported-control issues, stable counts,
  order preservation, and input immutability are covered.
- Downstream binding to player intents remains owned by `PlayerInputBinding2D`.
- The adapter does not call the product loop, step gameplay, execute gates or
  commands, map Qt/OS events, own camera/render data, or add save/persistence
  semantics.

Packet 4-A complete:
- `RuntimeGameplayProductLoopStepInput` supports an optional per-step
  `PlayerInputContext2D` override.
- Default behavior is unchanged: when no override is set, product loop keeps the
  authored/lowered frame context while replacing current state and caller
  intents.
- When the override is set, normal and explicit collision-world step paths pass
  it into the existing lower-level frame-step/gate path.
- The override is per-step policy only; it is not persisted in product loop
  state, runtime session/gameplay state, save snapshots, or UI models.
- Product-loop load behavior, frame indexing, collision-world behavior, and
  caller-intent replacement semantics are unchanged beyond the optional context
  override.

Packet 4-B complete:
- `RuntimeGameplayProductPresentationFrame` is a projection-only
  runtime/product presentation wrapper.
- Input is product loop state plus caller-owned `CameraState` plus
  `LevelRenderFrame2DConfig`.
- Loaded product loop state projects `state.currentState.session.level` through
  existing `LevelRenderFrame2D::build(...)` and returns a
  `LevelRenderFrame2DResult` with `Rendered` status.
- Not-loaded product loop state returns `NotLoaded`, echoes/copies the camera,
  and leaves the level frame default.
- The wrapper does not step gameplay, change product-loop state/index/context
  behavior, own camera lifecycle, persist presentation data, render player
  sprites, render modern `RuntimeGameplayState::npcActors`, perform IO, touch
  UI/CLI, or add save/load behavior.

Play-surface frame complete:
- `RuntimeGameplayProductPlaySurfaceFrame` is a runtime-only app-neutral facade
  for one caller-requested play frame.
- It composes existing public surfaces: `RuntimeGameplayProductInputAdapter`,
  `PlayerInputBinding2D`, `RuntimeGameplayProductLoop` one-frame step with
  per-step context override, and `RuntimeGameplayProductPresentationFrame`.
- Focused flow is product input events to binding actions, then player
  intents/context, then exactly one product-loop step, then presentation
  projection using the returned step state.
- Not-loaded flow skips adapter/binding/step, delegates to the presentation
  wrapper, and counts input events as ignored.
- No-frame/exhausted flow skips adapter/binding/step, synthesizes a no-frame
  step guard result, renders current loaded state through the presentation
  wrapper, and counts input events as ignored.
- Unfocused flow ignores transient events before adapter semantics, runs
  adapter/binding on empty events/actions with carried context, steps once with
  empty intents/context override, and presents post-step state.
- The result exposes nested adapter, binding, step, and presentation results
  plus ignored input count.
- The facade is not a Qt shell, product launch mode, automatic tick loop, UI
  adapter, raw OS event mapper, camera lifecycle owner, or save/load surface.

Remaining input gate questions:
- What product shell/play mode owns focus and input routing around the
  app-neutral play-surface frame?
- Is a Qt/raw-device adapter needed after shell/focus ownership is scoped, and
  where does it convert raw input into transient product input events?
- Who owns camera lifecycle/follow/rig/clamp policy?
- When are player sprite and modern `RuntimeGameplayState::npcActors` render
  projections added?
- Who owns the automatic app/tick loop around caller-requested frames?
- Who owns pause/retry/reset?
- What state must save/load for the first playable slice?
- What does completion/failure mean in the first slice?

Output:
- one product shell/play mode and focused input ownership packet;
- one optional Qt/raw-device adapter packet after product shell/focus ownership
  is scoped, if needed;
- one camera lifecycle/presentation policy packet;
- one automatic app/tick loop packet;
- one updated implementation order packet;
- one verification plan.

Hard stops:
- do not make authoring facade own product runtime;
- do not call TOML/package facades or file IO from the product loop;
- do not use full scenario/profile runners from the product loop;
- do not hide raw input inside runtime session state;
- do not add Qt/OS event types to scene/player or runtime product input
  surfaces;
- do not add gate execution or command mapping to `PlayerInputBinding2D`;
- do not change `RuntimeGameplayProductLoop` signatures or stepping as part of
  later input work;
- do not persist raw input or context overrides in runtime/session/product-loop
  state or snapshots;
- do not put presentation/camera state into gameplay truth;
- do not add camera lifecycle/follow/rig/clamp ownership to the projection
  wrapper;
- do not productize save/load in the next input or presentation packet;
- do not add pause/retry/reset or completion/failure/win/lose semantics in the
  input or presentation packet;
- do not treat `RuntimeGameplayProductPresentationFrame` as Qt shell/play mode;
- do not treat `RuntimeGameplayProductPlaySurfaceFrame` as Qt shell, product
  launch mode, automatic tick loop, or UI adapter;
- do not persist raw input in runtime session/gameplay/product-loop state,
  snapshots, saves, or UI models;
- do not add player sprite or modern NPC actor render projection without a
  separate gate;
- do not persist derived caches as save truth.

## Phase 5: Presentation / Render Integration

Status: projection-only product presentation wrapper and runtime-only
app-neutral play-surface frame integrated; Qt shell/play mode, raw-device
adapter, camera lifecycle policy, player sprite projection, modern NPC actor
projection, automatic app/tick loop, and UI presentation adapter remain separate
gates.

Goal: turn runtime state into visible play state.

Packets:
1. Caller-driven product presentation frame. Complete:
   - loaded product loop state + caller-owned camera/config -> level render
     frame result.
2. Runtime product play-surface frame. Complete:
   - transient product input events + focus/context/camera/config -> nested
     adapter, binding, one-step, and presentation results.
3. Camera lifecycle/presentation policy:
   - follow/rig/clamp ownership outside gameplay truth; screen/world transforms
     remain separate.
4. Debug overlay projection:
   - trace/final rows, AI map, collision, path, interactions, inventory.
5. UI presentation adapter:
   - convert render frame data into the chosen shell/app surface.

Hard stops:
- no automatic render generation inside gameplay ticks unless approved;
- no backend-specific assumptions in runtime;
- no camera state hidden in save/session until gated.

Focused verification:
- render-frame tests;
- camera/presentation state tests;
- UI presentation tests if target exists.

## Phase 6: Save/Load Productization

Status: gate before code.

Goal: save and resume the first playable slice.

Packets:
1. Authored package save/load roundtrip gate.
2. Runtime state persistence decision:
   - player;
   - NPCs;
   - interaction targets;
   - inventory stacks/drops;
   - command queue if needed;
   - package identity/metadata if needed.
3. Save-slot UX policy:
   - explicit slots vs autosave;
   - overwrite behavior;
   - failure diagnostics.

Hard stops:
- no migration of existing snapshot chunks as cleanup;
- no cloud sync, compression, encryption, or platform storage expansion;
- no derived cache persistence.

Focused verification:
- existing save/load/snapshot tests;
- new roundtrip tests for only approved fields.

## Phase 7: First Playable Slice

Status: after product loop, presentation, and minimal persistence gates.

Goal: produce the first real playable vertical slice.

Default content:
- one package fixture promoted to product demo content;
- player movement;
- one NPC;
- one item pickup;
- one keyed interaction;
- one checkable expectation path;
- visible success/failure/reset behavior.

Implementation packets:
1. Product scenario loader. Complete.
2. Product-owned loop state/step. Complete.
3A. Scene/player normalized input binding. Complete.
3B-A. Runtime/product transient input event adapter. Complete.
3B-B. Optional Qt/raw-device adapter into transient product input events.
4-A. Product loop per-step input context override. Complete.
4-B. Projection-only product presentation frame. Complete.
4-C. Runtime product play-surface frame. Complete.
4-D. Product shell/play mode and focused input ownership.
4-E. Camera lifecycle/presentation policy.
5. Automatic app/tick loop.
6. Player sprite and modern NPC actor render projection.
7. Pause/retry/reset.
8. Minimal save/load if approved.
9. Acceptance fixture/demo.

Verification:
- focused runtime/product tests;
- UI smoke if app surface exists;
- full CTest;
- manual run command documented.

Exit criteria:
- user can run the slice locally and play through it.

## Phase 8: Editor-Backed Game

Status: after playable slice.

Goal: move from read-only preview to controlled authoring surfaces.

Milestones:
1. Source-linked diagnostics.
2. Visual trace playback.
3. Build canvas for placement.
4. Structured controls for existing facts.
5. Source/TOML roundtrip gate.
6. Editable authoring with tests.

Hard stops:
- no arbitrary source mutation before roundtrip gate;
- no UI-owned parser;
- no scripting language.

## Phase 9: Gameplay Semantics Expansion

Status: only after product loop is real unless user explicitly prioritizes.

Candidate gates:
- emit event;
- talk/inspect;
- region triggers;
- terrain metadata;
- equipment;
- combat;
- progression/win/fail conditions.

Rules:
- one semantic gate at a time;
- one tiny fixture-backed implementation at a time;
- no generic condition/effect language without explicit approval.

## Worker Packet Templates

### Scout Packet

```text
Objective:
Files/surfaces to inspect:
Questions to answer:
Hard stops:
Expected output:
Verification:
```

### Builder Packet

```text
Objective:
Files likely touched:
Behavior to add/change:
Behavior to preserve:
Hard stops:
Focused tests:
Batch-end verification:
Required brief:
```

### Reviewer Gate

```text
Scope:
Ownership ruling:
Risks:
Missing tests:
Merge conflicts:
PASS/BLOCK:
```

## Verification Matrix

| Lane | Focused verification | Full verification trigger |
| --- | --- | --- |
| Runtime cleanup | touched runtime tests, summary/facade/CLI tests if projection changes | every cleanup batch |
| Legacy NPC | old module, runtime session, save/load, render tests as touched | any production compatibility change |
| Authoring | source-plan, TOML reader, converter, facade, CLI, manifest sweep | every authoring behavior batch |
| Package | package facade, CLI package tests, file/package parity | every package behavior batch |
| Preview/UI | preview model, UI model/projection, UI smoke when target exists | any UI target or model change |
| Product loop | runtime product-loop tests, input/render/save focused tests | every product-loop batch |
| Save/load | snapshot/chunk/slot/store tests plus new roundtrip tests | every persistence batch |
| Docs-only | `git diff --check` | full build optional unless roadmap-shaping |

Default full verification:
```sh
cmake -S engine -B engine/build
cmake --build engine/build
ctest --test-dir engine/build --output-on-failure
git diff --check
git ls-files '*Devilution*' '*devilution*' '*DevilutionX*' '*master.zip' '*godot*'
git ls-files --others --exclude-standard '*Devilution*' '*devilution*' '*DevilutionX*' '*master.zip' '*godot*'
```

## First Dispatch Sequence

1. Send researcher/reviewer a runtime cleanup scout packet.
2. Convert scout output into one finisher packet.
3. Let finisher build the behavior-preserving cleanup.
4. Merge and run integration verification.
5. Send researcher/reviewer a legacy NPC compatibility map packet.
6. Use the integrated read-only UI preview consumer as product-loop input.
7. Use the integrated load-only product scenario loader as Product Loop Packet 1.
8. Product Loop Packet 2 product-owned loop state/step is integrated.
9. Product Loop Packet 3A scene/player normalized input binding is integrated.
10. Product Loop Packet 3B-A runtime/product input adapter is integrated.
11. Product Loop Packet 4-A per-step input context override is integrated.
12. Product Loop Packet 4-B projection-only product presentation frame is
    integrated.
13. Runtime product play-surface frame is integrated.
14. Dispatch product shell/play mode and focused input ownership, camera
    lifecycle policy, automatic app/tick loop, or optional Qt/raw-device
    adapter, depending on planner scope.

Do not broaden the next Product Loop packet into pause/retry/reset,
completion/failure, save/load productization, product-loop signature changes,
command/gate execution, camera lifecycle/follow/rig/clamp ownership, Qt/UI/CLI
behavior, raw OS event types, player sprite/modern NPC actor render projection,
or new gameplay semantics unless the user explicitly reprioritizes.
