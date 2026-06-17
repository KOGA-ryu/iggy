# Batch 37: Frame Ordering Policy

## Goal
Make authored frame ordering deterministic and explicitly tested across controls and player commands.

## Current State
Authored controls/player commands carry frame ids and declaration indexes; the converter groups them into scenario frames.

## Slices
1. Audit current converter frame grouping/order behavior.
2. Add tests for declaration order, repeated frame ids, interleaved NPC/player commands, and no-frame defaults.
3. Add diagnostics only for ambiguous cases that are currently unsafe.
4. Update fixture README if order rules are clarified.

## Verification
Focused converter, profile scenario, and CLI trace tests.

## Hard Stops
No scripting language, conditional frames, or loops.

## Expected Result
Frame order remains deterministic as authored scenarios grow.
