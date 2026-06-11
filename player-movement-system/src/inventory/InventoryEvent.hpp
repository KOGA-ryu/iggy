#pragma once

#include "inventory/InventoryCommand.hpp"

namespace dev {

enum class InventoryEventType {
	Equipped,
	Unequipped,
	Rejected,
};

struct InventoryEvent {
	InventoryEventType type = InventoryEventType::Rejected;
	InventoryCommandType commandType = InventoryCommandType::EquipItem;
	InventoryCommandResultType commandResult = InventoryCommandResultType::Rejected;
	EquipmentResultType equipmentResult = EquipmentResultType::MissingItem;
	TargetId itemId = 0;
	EquipmentSlot slot = EquipmentSlot::Weapon;
};

} // namespace dev
