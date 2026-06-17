# Batch 55: Interaction Effect Fixture Pack

## Goal
Cover existing interaction effect kinds in authored fixtures without adding new effect semantics.

## Current State
Toggle and pickup are visible in canonical fixtures. `InspectText` and event-like facts may be less visible.

## Slices
1. Inventory existing `InteractionEffect2D` support and source-plan effect promotion.
2. Add fixtures/tests only for effect kinds already implemented safely.
3. Keep `EmitEvent` out unless Batch 41 approves semantics.
4. Assert final rows/counts and relevant report/ledger facts if exposed.

## Verification
Focused interaction, converter, and CLI tests.

## Hard Stops
No new event bus, dialogue, quest, UI, or scripting behavior.

## Expected Result
Existing effect promotion is demonstrated clearly in authored TOML.
