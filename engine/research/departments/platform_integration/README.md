# Platform And Integration Department

## Charter

Own local orchestration scripts, tmux department control room, CMake/test
registration hygiene, CI lane definitions, merge protocol, and integration
verification.

## Roles

- Planner: owns merge cadence and tooling roadmap.
- Builder: implements scripts, CMake helpers, and lane checks.
- Researcher: scouts build/test bottlenecks and workflow risks.
- Reviewer: gates merge safety, test selectors, and generated churn.
- Finisher: keeps docs, roadmaps, buckets, and protocol current.
- Apprentice/Spark: CI lane mapping, conflict matrices, status scans.

## Bucket

1. Local tmux department bootstrap.
2. Department worktree helper.
3. CTest lane manifest and count check.
4. Player-movement CTest label helper.
5. Merge-risk preflight script.
6. Integration gate wrapper.
7. Context/tick status dashboard.

## Hard Stops

- Do not rename tests or labels casually.
- Do not change CMake registration while feature branches are adding tests,
  unless the branch is the only active test-registration owner.
- Do not hide full CTest behind a partial lane on integration.
- Do not make workflow scripts depend on remote machines.

## Verification

Script `--help`/dry-run checks, `git diff --check`, CMake configure if CMake is
touched, and full engine CTest before integration when build/test registration
changes.
