# Batch 36: ResourceId Namespace Policy For Authoring

## Goal
Tighten or document authoring `ResourceId` naming conventions without forcing broad engine-wide ID policy.

## Current State
Fixtures use prefixes such as `npc:`, `profile:`, `target:`, `drop:`, `item:`, and `frame:`.

## Slices
1. Inventory IDs used by canonical and regression fixtures.
2. Decide which prefixes are conventions vs validation requirements.
3. Add validation only for source-plan-local ambiguity that causes real failures; otherwise keep docs-only.
4. Add tests for any newly enforced local rule.

## Verification
Focused source-plan validator/TOML tests if behavior changes; docs diff if docs-only.

## Hard Stops
No global `ResourceId` rewrite, save/load ID migration, or gameplay semantics.

## Expected Result
Authors get predictable ID guidance without over-constraining engine internals.
