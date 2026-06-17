# Batch 25: Runtime Authoring Cleanup Gate

Status: complete.

## Goal
Review runtime authoring wrappers now that CLI/facade paths exist, and identify safe cleanup targets.

## Current State
Runtime authoring/scenario/ascii families grew quickly. Some wrappers may now be test/tooling-only.

## Slices
1. Grep runtime authoring/scenario/ascii wrappers and tests.
2. Classify permanent API boundaries vs convenience/test wrappers.
3. Propose a cleanup batch only for low-risk duplication.
4. Do not remove code in this gate.

## Verification
Read-only review unless docs are updated.

## Hard Stops
No wrapper deletion in this gate.

## Expected Result
A concrete cleanup plan based on current usage, not frustration.

## Review Evidence
- Runtime authoring now has durable public/tooling boundaries:
  - `RuntimeGameplayTomlScenarioFacade` for explicit TOML file run/lint/check/trace.
  - `RuntimeGameplayTomlScenarioPackageFacade` for explicit package manifest
    validation and delegation.
  - `RuntimeGameplayAuthoringPreviewModel` for read-only tooling/editor
    projection.
  - `RuntimeGameplayAuthoringDiagnostics` and
    `RuntimeGameplayAuthoringErrorCodes` for user-facing diagnostic surfaces.
  - `RuntimeGameplayTomlScenarioSummaryProjection` and
    `RuntimeGameplayAsciiSourcePlanFinalDebugRows` for stable projection data.
- The lower-level source-plan stack remains real engine ownership, not cleanup
  debris: `RuntimeGameplayAsciiSourcePlan`, TOML reader/file reader, validator,
  authoring adapter, profile scenario converter, and AI-map promoter all still
  have focused tests.
- Test coverage is intentionally dense around parser, validator, converter,
  facade, package facade, CLI output contracts, manifest sweeps, expectations,
  preview model, schema snapshots, version policy, and negative fixtures.

## Cleanup Candidates
- Repeated CLI/preview/package status text mapping can be centralized behind a
  small projection helper if output and exit-code contracts remain unchanged.
- Summary/final-row counters appear in runtime reports, facade summaries, CLI
  printing, manifest tests, and preview projections. Future cleanup should reuse
  the existing summary projection rather than add another report layer.
- Test helper ceremony is now the best low-risk target: fixture path builders,
  CLI invocation wrappers, final-row block formatting, and repeated mode helpers
  can move into test support once the assertions stay at the same layer.
- CMake authoring-test density is visible but not harmful. A grouping helper for
  authoring tests may be worthwhile only if it does not hide fixture directory
  compile definitions.
- Package/file facade parity helpers in tests are cleanup candidates, but the
  parity assertions themselves should stay.

## Cleanup Non-Targets
- Do not remove source-plan parser, validator, converter, adapter, diagnostics,
  package facade, preview model, or CLI output-contract tests as part of a
  cleanup pass.
- Do not merge package execution into file execution in a way that changes
  explicit package-path validation or package diagnostics.
- Do not introduce a broad report/ledger framework while deduplicating
  projections.
- Do not change TOML, package, gameplay, save/load, UI/Edi, or runtime autorun
  semantics under cleanup.

## Proposed Follow-Up Packet
Add a focused cleanup packet for behavior-preserving authoring test/support
deduplication:

1. Extract test-only fixture path and CLI invocation helpers used by CLI,
   manifest, package-facade, and preview tests.
2. Extract test-only final-rows/summary assertion helpers where duplicated
   literal formatting already follows the locked CLI contract.
3. Optionally add a tiny runtime projection helper only if it replaces repeated
   status/summary text mapping without changing CLI output or facade result
   shapes.

Anything beyond that should return to planner with a concrete diff target and
risk assessment before implementation.
