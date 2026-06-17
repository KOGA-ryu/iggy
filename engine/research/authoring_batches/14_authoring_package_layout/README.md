# Batch 14: Authoring Package Layout

Status: complete.

## Goal
Define a future scenario package shape without implementing directory scanning.

## Current State
CLI reads exactly one TOML file.

## Slices
1. Propose a package shape in docs, for example:
   - `scenario.toml`
   - `README.md`
   - optional `expected.txt`
2. Add one non-executed example package directory only if tests/docs benefit.
3. Document why CLI still reads one explicit file.
4. Identify what would be needed before directory mode.

## Verification
Docs diff check and any fixture tests if files are executable.

## Hard Stops
No directory scanning implementation in this batch.

## Expected Result
A clear package proposal for future UI/Edi or content workflow discussions.

## Completed Coverage
- Added `engine/research/authoring_package_layout.md`.
- Defined a minimal package shape using `package.toml`, one main
  `scenario.toml`, and optional `README.md` docs.
- Kept package semantics as a wrapper around the existing one-file
  `RuntimeGameplayTomlScenarioFacade` flow.
- Documented non-goals: no discovery, dependencies, assets, save/load, UI/Edi,
  runtime autorun, parser fork, or new gameplay semantics.
