# Batch 57: Interaction State Expectations

## Goal
Extend authored expectations to verify final interaction target enabled states.

## Current State
Toggle interaction changes target state, but final rows only indirectly show behavior.

## Slices
1. Add expectation facts for target id plus expected enabled state.
2. Validate referenced target ids against authored/promoted interaction targets.
3. Compare final interaction target registry after run.
4. Add one toggle fixture expectation test and one bad-target diagnostic.

## Verification
Focused validator/converter/CLI check tests.

## Hard Stops
No new interaction effects, range/path semantics, or UI state.

## Expected Result
Interaction toggle scenarios can assert final target state explicitly.
