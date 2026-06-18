# Platform Department

## Charter

Own local developer workflow: tmux planner floor, helper scripts, worktree
creation, CMake/CTest lanes, status scans, and macros that make department work
cheaper.

## Planner Job

The platform planner turns repeated hub/operator moves into scripts, templates,
and checks. It should reduce friction without changing gameplay, UI behavior,
CLI output, or test semantics.

## On-Demand Workers

- Builder: scripts, CMake helpers, lane checks.
- Researcher: build/test bottleneck scans and workflow comparisons.
- Reviewer: merge-safety and command-safety gates.
- Finisher: docs, templates, and protocol cleanup.
- Apprentice/Spark: short scans, count summaries, mechanical grep work.

## Bucket

1. Planner-only tmux department floor.
2. Department bus templates and status helpers.
3. Merge-risk preflight script.
4. CTest lane manifest and test-count monitor.
5. Context/tick status dashboard.
6. Worktree helper hardening.

## Hard Stops

- Do not depend on SSH or remote machines.
- Do not rename tests or labels casually.
- Do not hide full CTest behind partial lanes on integration.
- Do not change CMake registration while feature branches are adding tests
  unless platform owns that active branch.

## Verification

Script syntax checks, dry-run checks where available, `git diff --check`, CMake
configure if CMake changes, and full engine CTest only when build/test
registration changes.
