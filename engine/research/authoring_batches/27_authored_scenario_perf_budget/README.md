# Batch 27: Authored Scenario Perf Budget

Status: complete.

## Goal
Add lightweight performance/size guardrails for authored scenario execution.

## Current State
Scenarios are small, but future fixtures can grow.

## Slices
1. Measure current canonical fixture run time in a stable, non-flaky way or add static size counts if timing is too fragile.
2. Add optional debug output or tests for row count, frame count, target/drop/control counts.
3. Avoid strict wall-clock tests unless local style already supports them.
4. Document practical authoring limits.

## Verification
Focused tests if added. Full verification at batch end.

## Hard Stops
No profiler framework. No flaky timing gates.

## Expected Result
The authored lane has visible scale assumptions.

## Completed Coverage
- Added a deterministic canonical fixture budget test instead of wall-clock
  timing.
- The budget covers source file bytes, grid size, source fact counts, authored
  frame/input counts, interaction/drop counts, and expectation rows.
- Documented the Authoring V1 fixture scale budget as a test/dev-harness
  envelope, not a runtime cap.
