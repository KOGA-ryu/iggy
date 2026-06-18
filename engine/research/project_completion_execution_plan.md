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

Status: after read-only UI preview Milestone 1; ready for docs/scout before
product-loop code.

Goal: define the first playable loop boundary.

Gate questions:
- Does product loop load package through package facade or a thinner runtime
  loader?
- Where does raw input map into player intents?
- Who owns camera/presentation state?
- Who owns pause/retry/reset?
- What state must save/load for the first playable slice?
- What does completion/failure mean in the first slice?

Output:
- one product-loop architecture packet;
- one implementation order packet;
- one verification plan.

Hard stops:
- do not make authoring facade own product runtime;
- do not hide raw input inside runtime session state;
- do not persist derived caches as save truth.

## Phase 5: Presentation / Render Integration

Status: after product loop gate.

Goal: turn runtime state into visible play state.

Packets:
1. Caller-driven render-frame step:
   - gameplay/session state + camera/presentation input -> render frame result.
2. Debug overlay projection:
   - trace/final rows, AI map, collision, path, interactions, inventory.
3. UI presentation adapter:
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
1. Product scenario loader.
2. Input binding.
3. Gameplay tick loop.
4. Render/presentation surface.
5. Pause/retry/reset.
6. Minimal save/load if approved.
7. Acceptance fixture/demo.

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
7. Choose product loop gate after those reports and integration results.

Do not start product-loop implementation until the first cleanup and read-only
UI preview Milestone 1 are complete, unless the user explicitly reprioritizes.
