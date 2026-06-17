# Batch 07: Mixed Scenario Pack

## Goal
Add richer canonical examples that combine existing authored semantics.

## Current State
Canonical fixtures cover movement, interaction, pickup, and a mixed mini scenario.

## Slices
1. Audit canonical fixtures for coverage gaps.
2. Add one or two small mixed scenarios using only existing semantics:
   - player moves and picks up
   - player interacts
   - NPC moves in a separate frame
3. Add CLI golden tests for final rows/counts.
4. If trace mode exists, add trace assertions for per-frame progression.
5. Update fixture README with short descriptions.

## Verification
Focused CLI tests. Full verification at batch end.

## Hard Stops
No locked-door/key semantics here. No new gameplay behavior.

## Expected Result
The fixture pack contains realistic small examples without opening new systems.
