# Batch 19: Editor Preview Model Gate

Status: complete.

## Goal
Decide whether an engine-only preview model should exist before UI/Edi integration.

## Current State
The CLI can render final rows and trace rows. UI/Edi is still out of scope.

## Slices
1. Review current final/trace result data and CLI formatting.
2. Identify the minimal engine data model a UI would need: rows, frames, statuses, issue list, counts.
3. Decide whether to add a backend-neutral preview model or keep CLI-only for now.
4. If approved, write the next implementation packet.

## Verification
Read-only review unless docs are updated.

## Hard Stops
No UI/Edi implementation in this gate.

## Expected Result
A go/no-go for a reusable preview model.

## Gate Decision
GO for a minimal engine/runtime-owned, read-only authoring preview model.

The model is a backend-neutral projection over existing authoring facades:
- TOML file input delegates to `RuntimeGameplayTomlScenarioFacade`.
- Package directory or `package.toml` input delegates to
  `RuntimeGameplayTomlScenarioPackageFacade`.
- Package metadata, diagnostics, run summary, final rows, trace frames, and
  expectation comparison are copied from existing facade results.

## Ownership
- Runtime/tooling owns the preview data model under `engine/src/runtime`.
- Future UI/Edi code may read the model but must not own parser, converter,
  runner, package, diagnostic, or projection logic.
- The model may expose status and diagnostics as existing facade/package
  surface data. It must not require a shared status redesign or new flattened
  diagnostic framework.

## Non-Goals
- UI widgets, editor shell integration, or view-model mutation.
- Editing APIs or write-back behavior.
- Save/load, runtime autorun, or product-loop integration.
- Package discovery or directory scanning beyond explicit package paths already
  supported by the package facade.
- New parser, converter, runner, CLI output, or gameplay semantics.
- New preview facts that require traversing runtime state beyond current facade
  projections.
