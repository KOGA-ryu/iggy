# Batch 26: Minimal Editor Model

Status: complete.

## Goal
Implement an engine-only authoring preview model only if Batch 19 passes.

## Current State
Requires editor preview model gate approval.

## Slices
1. Add backend-neutral preview data for source rows, final rows, frame trace rows, statuses, and issues.
2. Build it from existing authoring facade/runner results.
3. Add focused tests over canonical fixtures.
4. Do not touch UI/Edi.

## Verification
Focused preview model tests. Full verification at batch end.

## Hard Stops
Do not proceed without gate approval. No UI code.

## Expected Result
A reusable engine model that a future UI can read without owning runtime logic.

## Completed Coverage
- Added `RuntimeGameplayAuthoringPreviewModel` under `engine/src/runtime`.
- Preview inputs are explicit TOML file paths or explicit package
  directories/`package.toml` paths.
- TOML file previews delegate to `RuntimeGameplayTomlScenarioFacade`; package
  previews delegate to `RuntimeGameplayTomlScenarioPackageFacade`.
- The model copies existing projections only: package metadata, input/source
  paths, status, package issues, authoring diagnostics, run summary, final rows,
  trace frames, and expectation comparison.
- Focused tests cover TOML file run previews, package run previews, trace,
  check expectations, package manifest failure, and delegated package scenario
  failure.
