# Batch 14: Authoring Package Layout

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
