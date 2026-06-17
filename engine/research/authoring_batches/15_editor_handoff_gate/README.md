# Batch 15: Editor Handoff Gate

Status: complete.

## Goal
Decide whether the authored scenario lane is stable enough to hand off to UI/Edi integration planning.

## Current State
Requires prior authoring/CLI/debugging batches to be green.

## Slices
1. Review current CLI, fixtures, diagnostics, lint/check/trace support if present.
2. Identify the exact API surface a UI/editor would call.
3. Identify missing safety features before UI exposure.
4. Produce a go/no-go decision and an integration checklist.

## Verification
Read-only review unless docs are updated. No UI implementation.

## Hard Stops
Do not build UI/Edi in this gate.

## Expected Result
A concrete handoff decision and requirements list for future editor work.

## Gate Decision
READY for engine-owned, read-only preview modeling. NOT READY for UI/Edi
integration, editing, mutation, save/load, or runtime autorun.

Prerequisites now met:
- `RuntimeGameplayTomlScenarioFacade` provides file read/adapt/validate/run,
  lint/check/trace modes, diagnostics, summary projection, final rows, trace
  frames, and expectation comparison.
- `RuntimeGameplayTomlScenarioPackageFacade` wraps explicit package
  directories or `package.toml` files and delegates to the file facade without
  adding gameplay semantics.
- Canonical fixture manifests, expectation checks, package fixture coverage,
  file/package parity tests, and stable CLI output contracts are in place.
- User-facing diagnostics and content error codes are available through the
  existing authoring facade/diagnostic surfaces.

Future UI/editor callers should depend on a runtime/tooling projection over
these facades rather than calling parser, converter, runner, or CLI formatting
code directly.

## Handoff Requirements
- Use explicit TOML file paths or explicit package paths only; no directory
  scanning or package discovery.
- Treat the preview as read-only. No editing API, mutation API, save/load, or
  game-loop autorun belongs in this handoff.
- Preserve existing facade behavior and CLI output contracts.
- Package metadata is display-only; gameplay facts remain owned by the main
  source-plan TOML file.
