# Inventory System

File:

- `/Users/kogaryu/iggy3d/src/runtime/inventory/InventoryState.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/inventory/InventorySystem.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/inventory/InventorySystem.cpp`

Verified at: `75e85fb5`

## Owns

- Runtime inventory packet shape: `InventoryStack`, `PlayerInventory`, and `InventoryState`.
- Inventory item request/result/status contract.
- Per-player inventory lookup, item presence checks, add item, and remove item operations.
- Stack validation for non-empty item ids and nonzero counts.

## Does Not Own

- Command admission policy for required items.
- Interaction effects that decide when items are added.
- Objective evaluation based on inventory contents.
- Save/load serialization of inventory.
- App receipt/HUD presentation of inventory state.

## Reads

- `InventoryState::players`, player slot ids, item ids, and stack counts.
- `InventoryItemRequest` player slot, item id, and count.

## Writes / Mutates

- `addItem` mutates an existing player inventory by increasing or appending a stack.
- `removeItem` mutates an existing player inventory by decreasing or erasing a stack.
- `hasItem` and `findInventory` are read-only.

## Calls Out To / Wires Out To

- Uses player slot validity from `PlayerSlot.hpp`.
- `InteractionSystem`, `ObjectiveSystem`, `CommandAdmission`, diagnostics, and app gameplay proof code read or mutate inventory through this surface.

## Called By / Entry Points

- `addItem(...)`
- `removeItem(...)`
- `hasItem(...)`
- `findInventory(...)`

## Invariants

- Invalid player slot, empty item id, or zero count rejects requests.
- Missing player inventory rejects mutation as `InvalidPlayerSlot`.
- Existing inventory state with invalid player slot, empty item id, or zero stack count rejects mutation as `InvalidState`.
- Add rejects unsigned count overflow.
- Remove rejects missing item and insufficient count without mutation.
- Stack count reaching zero on remove erases the stack.

## Tests / Proof Commands

- `rg -n "inventory_system_tests|addItem|removeItem|hasItem|findInventory" cmake/iggy3d_tests.cmake tests/unit src/runtime`
- `cmake/iggy3d_tests.cmake` registers `inventory_system_tests`.

## Nearby Files Usually Not Touched

- `/Users/kogaryu/iggy3d/src/runtime/interaction/InteractionSystem.*`
- `/Users/kogaryu/iggy3d/src/runtime/objective/ObjectiveSystem.*`
- `/Users/kogaryu/iggy3d/src/runtime/command/CommandAdmission.*`
- `/Users/kogaryu/iggy3d/src/runtime/save/SaveCodec.*`

## Update When

- Inventory packet fields, status meanings, stack validation, add/remove semantics, or item-query contracts change.

## Do Not Update When

- Only interaction effect routing, objective condition evaluation, save codec formatting, or app display changes.
