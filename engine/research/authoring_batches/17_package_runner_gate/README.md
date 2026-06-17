# Batch 17: Package Runner Gate

Status: complete.

## Goal
Review whether directory/package execution should be opened after the package layout proposal.

## Current State
The CLI intentionally reads exactly one TOML file. Batch 14 proposes package shape but does not implement scanning.

## Slices
1. Review the package layout proposal and canonical fixture needs.
2. Identify whether package mode needs `scenario.toml`, expected output files, or README-only docs.
3. Decide whether package mode should be a CLI feature, test helper, or not built yet.
4. Return a go/no-go and exact implementation packet if approved.

## Verification
Read-only review unless docs are updated.

## Hard Stops
No implementation in this gate unless explicitly redirected.

## Expected Result
A clear package-runner decision before directory semantics are added.

## Gate Decision
Go for Batch 18 under a narrow package wrapper.

## Approved Implementation Ownership
- Add a small engine runtime package facade/runner that reads one explicit
  package path, resolves `package.toml`, validates its `main` scenario path, and
  delegates execution to `RuntimeGameplayTomlScenarioFacade`.
- Keep `scenario.toml` as the only gameplay authoring file. The package layer
  must not parse source-plan facts, convert scenarios, run frames, or project
  rows itself.
- Expose package execution through the CLI only as an explicit path mode:
  existing one-file TOML paths keep current behavior; package paths route
  through the package facade and preserve the existing output sections.
- Use embedded `[expect]` facts from the main TOML for check mode; no separate
  expected-output file is needed for implementation.

## Required Guardrails For Batch 18
- No recursive discovery, registry, dependencies, assets, save/load, UI/Edi, or
  runtime autorun.
- Reject missing package manifests, invalid package format/version, empty or
  absolute `main`, and `main` paths that escape the package directory.
- Metadata beyond `format_id`, `version`, and `main` waits for Batch 44 and must
  remain display/read-only.
