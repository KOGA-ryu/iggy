# Batch 41: Emit Event Semantics Gate

## Goal
Decide what authored `emit_event` effects mean, if anything, before implementing event behavior.

## Current State
`RuntimeGameplayAsciiSourcePlanInteractionEffectKind::EmitEvent` exists, but broad event semantics should not be inferred.

## Slices
1. Inspect current interaction effect application/reporting for event-like facts.
2. Decide whether `emit_event` should remain authoring metadata, produce a report-only event, or mutate state.
3. If approved, write a minimal implementation packet.

## Verification
Read-only gate unless docs are updated.

## Hard Stops
No generic scripting, event bus framework, or quest system.

## Expected Result
Event authoring does not drift into a hidden scripting system.
