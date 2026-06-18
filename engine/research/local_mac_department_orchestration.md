# Local Mac Department Orchestration

This is Iggy's local version of the department workflow. It borrows the useful
structure from the `edi` orchestration notes, but it is not the same setup:
there is no Linux worker box and no SSH dependency. Work happens on this Mac
through local tmux department windows, local git worktrees, Codex threads,
short-lived subagents, and repo-owned handoff docs.

## Operating Model

The hub is the conductor. In practice, the hub is the current planning session,
the local tmux control room, and the integration worktree:

- Integration worktree: `/Users/kogaryu/iggy`
- Integration branch: `master`
- Hub responsibilities: own the whole roadmap, assign departments, keep buckets
  current, build helper scripts/macros, decide merge order, run the integration
  gate, and recycle stale context at clean boundaries.

Departments do the work in isolated local branches. Each department has a
planner, builder, researcher, reviewer, finisher, and apprentice/Spark slot,
even if some slots are staffed by a persistent Codex thread and some are
short-lived subagents.

| Department | Typical role | Work area |
| --- | --- | --- |
| Runtime | gameplay loop, sessions, save/load, frame runners, reports | one topic worktree per runtime stretch |
| AI/NPC | profiles, AI maps, actor movement, navigation, legacy NPC migration | one topic worktree per AI/NPC stretch |
| Authoring | TOML/package/preview/facade/content fixtures | one topic worktree per authoring stretch |
| UI/Product | shell, preview consumption, editor surfaces, play/debug UX | one topic worktree per UI stretch |
| Platform/Integration | CMake, CI lanes, scripts/macros, merge/release hygiene | integration branch plus tooling worktrees |

The current worktree topology is always discovered with:

```sh
git worktree list
```

The local tmux control room is created with:

```sh
engine/tools/iggy-dept-up.sh
```

The helper opens one tmux session with windows for hub, runtime, AI/NPC,
authoring, UI/product, platform, integration, and research. The windows are
command surfaces and status dashboards; the workers still communicate durable
results through commits, bucket docs, and Codex briefs.

Do not treat a hardcoded snapshot in any doc as authoritative. Verify the local
state before assigning, rebasing, or merging.

## Local Bus

The bus is a message plus durable state.

In this repo, the message doorbell is one of:

- a Codex thread prompt to a persistent worker;
- a subagent task for short-lived scouting;
- a commit-end brief from a worker back to the hub.

The durable state is on disk:

- `engine/research/roadmap.md` for project state;
- `engine/research/departments/` for department charters and buckets;
- `engine/research/authoring_batches/` for legacy/builder authoring packets;
- `engine/research/finisher_batches/` for cleanup packets;
- focused research docs such as API indexes, boundary notes, closeout notes, and
  smell/audit notes;
- commits and branch history.

Thread memory is useful but not authoritative. If context is lost, a fresh
session should be able to recover by reading the roadmap, the relevant bucket,
recent commits, and the current branch status.

## Bucket And Scoop

The hub should not drip-feed one tiny packet at a time when the ownership
surface is clear. The better pattern is:

1. Build or refresh a bucket of related packets.
2. Have reviewer/researcher scope repo fit and conflict risk.
3. Assign a department planner a scoped stretch with file ownership and hard
   stops.
4. The department planner uses its researcher/reviewer to refine semantics,
   data ownership, compute costs, and Codex-vs-Spark split.
5. The department builder/finisher/apprentice executes coherent packets on the
   department branch.
6. Merge only through the hub after verification and review.

Use the raw bucket order only when there is no better scoped stretch. A
planner-scoped stretch overrides raw packet order until it completes or blocks.

## Department Branch Rules

Each department gets its own worktree and branch. Use the helper when creating
a new local fork:

```sh
engine/tools/iggy-dept-worktree.sh runtime report-cleanup
engine/tools/iggy-dept-worktree.sh ai region-ai-map
```

Branch names should describe the department and topic, for example:

- `codex/runtime-report-cleanup`
- `codex/ai-region-ai-map`
- `codex/ui-preview-consumption`
- `codex/authoring-diff-report`

Rules:

- Departments do not work directly on integration `master`.
- Departments do not edit another department's worktree.
- Department planners own their local bucket and dispatch.
- Builders own feature behavior and fixtures for their department branch.
- Finishers own cleanup, bloat reduction, docs, and test-support work for their
  department branch.
- Reviewers and researchers are read-only unless explicitly assigned a docs
  packet.
- Apprentices/Spark scouts are normally read-only or narrowly scoped test/docs
  workers and are closed after returning results.

If two branches need the same file, the hub serializes that file's work or
assigns one branch as owner and makes the other wait.

## Work Flow

The default flow for a substantial feature or cleanup lane is:

```text
research/reviewer scout
  -> hub scopes department order
  -> department planner refines and dispatches
  -> builder, finisher, or apprentice branch work
  -> focused branch verification
  -> department batch-end brief
  -> reviewer merge gate
  -> hub merge to integration
  -> full integration verification
  -> roadmap/bucket sync when needed
```

Some work should stay serialized on the integration branch:

- public authoring facade/package/preview/diagnostic API changes;
- CLI output strings, exit codes, and output-contract snapshots;
- runtime report/count field shape when it affects facade, CLI, or preview
  projections;
- save/load/session snapshot and legacy `modules/npc_ai` compatibility code;
- CMake test registration when multiple branches are adding tests;
- gameplay semantics, including AI-map policy behavior, region semantics, NPC
  movement/reservation, locked door/key behavior, and new interaction effects.

## Verification Lanes

The integration branch still needs the full gate. Department branches can use
focused lanes during the slice, then broaden at batch end.

Engine lanes:

```sh
ctest -L "core|modules|servers" --test-dir engine/build --output-on-failure
ctest -L "scene-ai|scene-npc|scene-level" -LE "integration|acceptance|pipeline" --test-dir engine/build --output-on-failure
ctest -L runtime -LE "integration|acceptance|pipeline" --test-dir engine/build --output-on-failure
ctest -L integration --test-dir engine/build --output-on-failure
ctest -L acceptance --test-dir engine/build --output-on-failure
ctest -L save-load --test-dir engine/build --output-on-failure
```

Player-movement lanes are currently regex-based because that CMake tree does
not yet label tests:

```sh
ctest --test-dir player-movement-system/build --tests-regex "^(movement_tests|artifact_(output|text)_tests|input_(routing|drain)_tests|command_drain_tests|source_drain_tests|frame_(source|simulation|lifecycle)_tests|run_result_policy_tests)$" --output-on-failure
ctest --test-dir player-movement-system/build --tests-regex "^(game_loop_|runtime_|session_|file_store_tests|inventory_command_persistence_tests|save_snapshot_persistence_tests)" --output-on-failure
```

Integration gate:

```sh
cmake -S engine -B engine/build
cmake --build engine/build
ctest --test-dir engine/build --output-on-failure
git diff --check
```

Add domain-specific scans when a packet already requires them, such as forbidden
reference scans or dependency-direction greps.

## Merge Order

Before merging, compare branch diffs:

```sh
git diff --name-status master...HEAD
```

Default merge order when multiple departments are active:

1. Merge feature branches that add fixtures or domain behavior if they do not
   touch shared report/projection surfaces.
2. Merge cleanup branches that consume or simplify the now-current surfaces.
3. Run reviewer gate over the integrated state.
4. Land docs/test-support harmonization last.
5. Run full integration verification.

If a cleanup branch changes shared summary/count projection, merge it before a
feature branch that adds golden count assertions, then rebase the feature branch.

Manual review is required even for clean git merges when these files are touched:

- `engine/src/runtime/RuntimeGameplayTomlScenarioFacade.*`
- `engine/src/runtime/RuntimeGameplayTomlScenarioPackageFacade.*`
- `engine/src/runtime/*SummaryProjection*`
- `engine/apps/scenario_toml_runner/IggyScenarioTomlRunner.cpp`
- `engine/cmake/iggy_runtime_sources.cmake`
- `engine/cmake/iggy_runtime_tests.cmake`
- `engine/tests/runtime_gameplay_toml_scenario_*`
- `engine/tests/iggy_scenario_toml_runner*`
- `engine/research/roadmap.md`
- `engine/research/authoring_batches/README.md`
- `engine/research/finisher_batches/README.md`

## Tick And Recycle

The tick is the context boundary. It is where a stale session adopts the latest
workflow.

At a tick, a fresh or recycled hub reads:

1. this document;
2. `engine/research/roadmap.md`;
3. active department bucket READMEs;
4. recent branch commits and `git worktree list`;
5. latest worker briefs from persistent Codex threads.

Workers should leave enough durable state that they can be replaced:

- commit messages;
- packet README status;
- batch-end brief;
- exact verification commands;
- explicit blocker if blocked.

If a task cannot be resumed from those artifacts, the task is too large or too
implicit and should be split.

## Hard Stops

- Do not delete `modules/npc_ai` as cleanup.
- Do not change save/load formats without a specific packet and reviewer gate.
- Do not change CLI output or exit codes without output-contract tests.
- Do not add TOML dependencies or JSON surfaces unless a packet explicitly opens
  that policy.
- Do not add generic scripting or inferred region/event semantics.
- Do not make package mode recursive discovery or content management.
- Do not let CMake/test-registration cleanup run in parallel with branches that
  add tests.

## Current Department Pattern

Use this as the current preferred pattern:

- Head planner/hub: owns the complete roadmap and departments.
- Runtime department: one branch per runtime/session/save/report stretch.
- AI/NPC department: one branch per AI-map/profile/NPC/navigation stretch.
- Authoring department: one branch per TOML/package/preview/content stretch.
- UI/Product department: planner/designers first, then UI implementation.
- Platform/Integration department: tools, CMake, CI lanes, merge hygiene.
- Reviewer/researcher lanes: feed departments and gates, normally read-only.
- Apprentice/Spark: temporary scouts or tiny bounded work packets.
- Hub: keeps departments fed, prevents shared-file collisions, and integrates.

The goal is not maximum parallel edits. The goal is maximum independent
throughput with clear ownership and cheap integration.
