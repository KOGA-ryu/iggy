# DevilutionX Items / Inventory Inventory

Source inspected:
- DevilutionX-master/Source/inv.cpp
- DevilutionX-master/Source/inv.h
- DevilutionX-master/Source/items.cpp
- DevilutionX-master/Source/items.h
- DevilutionX-master/Source/items/validation.*
- DevilutionX-master/Source/stores.cpp
- player-movement-system/src/items
- player-movement-system/src/inventory

Useful names:
- Item
- InvBody
- InvList
- SpdList
- HoldItem
- inv_body_loc
- CanEquip
- ChangeEquipment
- RemoveEquipment
- AutoPlaceItemInInventory
- AutoPlaceItemInBelt
- StoreAutoPlace
- TakePlrsMoney
- RoomForGold
- CanUseItem
- _iIdentified
- _iDurability
- _iCharges

Ownership boundary:
- Item definitions belong in item/resource data.
- Inventory grid and equipment slots belong in inventory service.
- Store transactions consume inventory commands, not direct UI state.

Take:
- equipment slots as explicit enum.
- held item transaction boundary.
- inventory grid placement service.
- item usability/stat gate.
- gold as inventory/currency rule.

Skip:
- UI line selection.
- Diablo item field names.
- store-specific global state.
- save compatibility fields.

Defer:
- stash.
- repair/recharge/identify services.
- item affix generation.

Next local target:
- engine/src/scene/inventory/Inventory.*
- engine/src/scene/inventory/EquipmentSlots.*
- engine/tests/inventory_tests.cpp
