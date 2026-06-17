# Batch 54: Multi-Actor Profile Fixture Pack

## Goal
Add canonical or regression fixtures proving multiple actors, shared profiles, distinct profiles, and profile catalog validation.

## Current State
Current fixtures are guard-room scale and mostly single-actor.

## Slices
1. Add a small two-NPC fixture with shared profile and deterministic movement.
2. Add a two-profile fixture if it exercises existing profile resolution.
3. Add one negative fixture for missing/unknown profile only if not already covered.
4. Add CLI golden or converter tests depending on whether the fixture is canonical or regression-only.

## Verification
Focused converter/profile scenario/CLI tests.

## Hard Stops
No new AI behavior, profile inheritance, ranks, combat stats, or broad behavior pools.

## Expected Result
Profile scenario conversion is covered beyond one actor.
