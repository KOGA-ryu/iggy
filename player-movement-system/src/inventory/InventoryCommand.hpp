#pragma once

#include <optional>

#include "inventory/EquipmentService.hpp"

namespace dev {

enum class InventoryCommandType {
	EquipItem,
	UnequipSlot,
};

struct InventoryCommand {
	InventoryCommandType type = InventoryCommandType::EquipItem;
	std::optional<TargetId> itemId;
	std::optional<EquipmentSlot> slot;
};

enum class InventoryCommandResultType {
	Applied,
	Rejected,
};

struct InventoryCommandResult {
	InventoryCommandResultType type = InventoryCommandResultType::Rejected;
	InventoryCommand command;
	EquipmentResult equipmentResult;
};

} // namespace dev
