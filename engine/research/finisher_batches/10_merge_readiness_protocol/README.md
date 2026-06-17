# 10 Merge Readiness Protocol

Status: complete.

Goal: define how the finisher branch merges back while builder continues.

Slices:
- Document rebase/merge protocol.
- Document conflict rules.
- Document verification gates.

Verification:
- Docs diff review.

Branch posture:
- Finisher branch: `codex/finisher-roadmap`
- Finisher worktree: `/Users/kogaryu/iggy-finisher`
- Builder worktree: `/Users/kogaryu/iggy`
- Finisher changes are intentionally docs-only in the initial packet set.

Before rebasing or merging:
- Check builder status from `/Users/kogaryu/iggy` with
  `git status --short --branch`.
- Check finisher status from `/Users/kogaryu/iggy-finisher` with
  `git status --short --branch`.
- Do not rebase over uncommitted finisher work.
- Do not overwrite builder worktree changes.
- If builder has active edits in a file finisher also changed, pause and let the
  planner choose ordering.

Preferred sync protocol:
1. In `/Users/kogaryu/iggy-finisher`, fetch/review the current target baseline
   if a remote is available.
2. Rebase `codex/finisher-roadmap` onto the builder's chosen integration commit,
   not onto an unrelated remote fork.
3. Resolve conflicts only in finisher-owned docs unless the planner explicitly
   approves source/test conflict resolution.
4. Re-run verification gates.
5. Merge or fast-forward into the planner-selected integration branch.

Conflict rules:
- Prefer builder source/test changes over finisher docs if both mention a newly
  completed implementation packet.
- Preserve finisher packet history and status notes unless they are superseded by
  newer builder facts.
- Never resolve conflicts by deleting `modules/npc_ai`.
- Never introduce package/directory scanning, JSON, TOML dependency changes,
  save/load behavior, UI/Edi behavior, or gameplay semantics during merge
  conflict cleanup.
- If a conflict touches `IggyScenarioTomlRunner.cpp`,
  `RuntimeGameplayTomlScenarioFacade.*`, or
  `iggy_scenario_toml_runner_tests.cpp`, treat it as a builder-owned conflict
  unless the planner explicitly transfers it to finisher.

Verification gates:
- Docs-only merge:
  - `git status --short --branch`
  - `git diff --check`
  - Manual docs diff review of changed roadmap/bucket files.
- Source/test merge:
  - Focused build/tests for touched targets.
  - `cmake -S engine -B engine/build`
  - `cmake --build engine/build`
  - `ctest --test-dir engine/build --output-on-failure`
  - `git diff --check`
- Output-contract merge:
  - All source/test gates.
  - Explicit review of CLI output and exit-code tests.
  - No textual output change unless tests lock the new contract.

Result:
- Merge readiness protocol documented as docs only.
- No branch operation, source change, or merge performed by this packet.
