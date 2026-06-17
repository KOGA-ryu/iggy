# Batch 31: Authoring Facade Parity Audit

Status: complete.

## Goal
Ensure the engine facade, CLI runner, and canonical fixture path all use one equivalent read-adapt-run-project sequence.

## Current State
Batch 16 is expected to extract the CLI pipeline into a runtime helper. The CLI originally owned direct sequencing in `IggyScenarioTomlRunner.cpp`.

## Slices
1. Compare CLI code path, facade/helper path, and tests after Batch 16.
2. Add focused parity tests if helper and CLI can drift.
3. Remove only local duplication that is proven redundant.
4. Update API index only if the public helper surface changed.

## Verification
Focused facade and CLI tests. Full verification at batch end.

## Hard Stops
No new report framework, directory scanning, UI/Edi, or gameplay semantics.

## Expected Result
CLI and future tools share the same authoring execution semantics without duplicate pipeline code.

## Completed Coverage

- Confirmed the CLI now delegates read/adapt/lint/run/project behavior to `RuntimeGameplayTomlScenarioFacade`.
- Added CLI parity coverage that derives expected summary values, final rows, and trace frame blocks from a facade execution and asserts the CLI output matches those facade projections.
- No additional gameplay semantics or reporting framework were added.
