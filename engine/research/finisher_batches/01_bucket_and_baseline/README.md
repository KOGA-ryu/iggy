# 01 Bucket And Baseline

Status: complete.

Goal: establish the finisher bucket and baseline status.

Slices:
- Create `engine/research/finisher_batches/README.md`.
- Record finisher branch/worktree.
- Record current builder baseline and known in-flight risk.
- Run lightweight status checks.

Baseline:
- Finisher worktree: `/Users/kogaryu/iggy-finisher`
- Finisher branch: `codex/finisher-roadmap`
- Builder worktree: `/Users/kogaryu/iggy`
- Builder branch at setup: `master`
- Builder baseline at setup: `c6c2800a` (`Mark authoring facade API packet complete`)
- Builder status at setup: `## master...origin/master [ahead 135]`
- Known in-flight risk: authoring facade work was just pulled forward and marked
  complete at the setup baseline. Finisher should treat
  `IggyScenarioTomlRunner.cpp` and `iggy_scenario_toml_runner_tests.cpp` as
  collision-prone until the builder facade follow-up settles.

Verification:
- `git status --short --branch`
- `git diff --check`

Result:
- Bucket directory and initial packet queue were created.
- Lightweight status checks passed before commit.
