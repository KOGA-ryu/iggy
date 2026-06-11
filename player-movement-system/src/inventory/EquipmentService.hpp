#pragma once

#include <cstddef>
#include <optional>

#include "inventory/Inventory.hpp"

namespace dev {

enum class EquipmentResultType {
	Equipped,
	Unequipped,
	MissingItem,
	NotEquippable,
	WrongSlot,
	InventoryFull,
	EmptySlot,
};

struct EquipmentResult {
	EquipmentResultType type = EquipmentResultType::MissingItem;
	TargetId itemId = 0;
	EquipmentSlot slot = EquipmentSlot::Weapon;
};

class EquipmentService {
public:
	[[nodiscard]] EquipmentResult equip(Inventory &inventory, TargetId itemId) const;
	[[nodiscard]] EquipmentResult unequip(Inventory &inventory, EquipmentSlot slot) const;

private:
	[[nodiscard]] std::optional<std::size_t> findInventoryItem(const Inventory &inventory, TargetId itemId) const;
	[[nodiscard]] std::optional<Item> &slotRef(Equipment &equipment, EquipmentSlot slot) const;
	[[nodiscard]] const std::optional<Item> &slotRef(const Equipment &equipment, EquipmentSlot slot) const;
};

} // namespace dev
