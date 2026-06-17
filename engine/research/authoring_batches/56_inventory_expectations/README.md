# Batch 56: Inventory Expectations

Status: complete.

## Goal
Extend authored expectations to verify final inventory facts after pickup scenarios.

## Current State
Active Batch 04 work appears to add final rows and summary-count expectations. Pickup state exists in runtime inventory.

## Slices
1. Add expectation facts for final player inventory stacks only if Batch 04 expectation shape lands cleanly.
2. Validate expectation shape and item ids/counts without altering gameplay.
3. Compare final runtime inventory after scenario run.
4. Add one pickup fixture expectation test.

## Verification
Focused source-plan/TOML/CLI check tests.

## Hard Stops
No inventory item catalog expansion, equipment, containers, or save/load changes.

## Expected Result
Pickup scenarios can self-check inventory, not just final rows/counts.

## Completed Coverage

- Added optional `[[expect_inventory_stacks]]` source-plan facts for expected final inventory item id and count.
- Validates inventory expectation shape for missing item ids, zero counts, and duplicate item ids without changing gameplay.
- Check mode compares expected stacks against final `RuntimeGameplayState.inventory.inventory.stacks`.
- `locked_door_key_room.toml` now self-checks that picking up the key leaves `item:key` in final inventory.
