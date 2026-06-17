# Batch 11: AI Map Region Fixtures

## Goal
Add canonical fixtures using existing region-to-AiMap promotion and map-aware NPC behavior.

## Current State
Source-plan regions can promote to AiMap through explicit policies. NPC profile/control planning can use AiMap facts.

## Slices
1. Identify the smallest existing AiMap policy needed by current converter config or source-plan facts.
2. Add a self-contained fixture with regions and NPC behavior that changes due to AiMap query facts.
3. Add CLI or focused acceptance tests proving final behavior.
4. Update fixture README with region examples.

## Verification
Focused AiMap/source-plan/converter/CLI tests. Full verification at batch end.

## Hard Stops
No inferred region semantics. No graph/link semantics unless already supported.

## Expected Result
Region-authored AI map behavior is demonstrated without new AI semantics.
