# Batch 11: AI Map Region Fixtures

Status: complete.

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

## Completed Coverage
- Added `region_ai_map_room.toml` as a regression-only fixture for existing
  `[[regions]]` parsing.
- Added focused coverage that reads the fixture, asserts exact region facts,
  promotes the region through explicit C++ `RuntimeGameplayAsciiSourcePlanRegionAiMapPromoter`
  policy, and verifies node id/tags/weights/position/radius with no invented
  links.
- Added explicit converter-config coverage proving `promoteRegionAiMap = true`
  wires the promoted AI map into profile scenario `aiMap` and `refreshAiMap`
  frames, plus runner preservation.
- Did not add CLI/default facade region behavior, TOML policy tables, triggers,
  proximity, scripting, or new gameplay semantics.
