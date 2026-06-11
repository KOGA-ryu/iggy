#include "EquipmentService.hpp"

#include <cstddef>

namespace dev {

EquipmentResult EquipmentService::equip(Inventory &inventory, TargetId itemId) const
{
	const std::optional<std::size_t> index = findInventoryItem(inventory, itemId);
	if (!index.has_value())
		return { .type = EquipmentResultType::MissingItem, .itemId = itemId };

	Item item = inventory.items[*index];
	if (!item.equipmentSlot.has_value())
		return { .type = EquipmentResultType::NotEquippable, .itemId = itemId };

	inventory.items.erase(inventory.items.begin() + static_cast<std::ptrdiff_t>(*index));

	std::optional<Item> &slot = slotRef(inventory.equipment, *item.equipmentSlot);
	if (slot.has_value())
		inventory.items.push_back(*slot);
	slot = item;

	return {
		.type = EquipmentResultType::Equipped,
		.itemId = itemId,
		.slot = *item.equipmentSlot,
	};
}

EquipmentResult EquipmentService::unequip(Inventory &inventory, EquipmentSlot slot) const
{
	std::optional<Item> &equipped = slotRef(inventory.equipment, slot);
	if (!equipped.has_value())
		return { .type = EquipmentResultType::EmptySlot, .slot = slot };

	if (inventory.full())
		return {
			.type = EquipmentResultType::InventoryFull,
			.itemId = equipped->id,
			.slot = slot,
		};

	const TargetId itemId = equipped->id;
	inventory.items.push_back(*equipped);
	equipped.reset();
	return {
		.type = EquipmentResultType::Unequipped,
		.itemId = itemId,
		.slot = slot,
	};
}

std::optional<std::size_t> EquipmentService::findInventoryItem(const Inventory &inventory, TargetId itemId) const
{
	for (std::size_t index = 0; index < inventory.items.size(); ++index) {
		if (inventory.items[index].id == itemId)
			return index;
	}
	return std::nullopt;
}

std::optional<Item> &EquipmentService::slotRef(Equipment &equipment, EquipmentSlot slot) const
{
	switch (slot) {
	case EquipmentSlot::Weapon:
		return equipment.weapon;
	case EquipmentSlot::Armor:
		return equipment.armor;
	case EquipmentSlot::Accessory:
		return equipment.accessory;
	}
	return equipment.weapon;
}

const std::optional<Item> &EquipmentService::slotRef(const Equipment &equipment, EquipmentSlot slot) const
{
	switch (slot) {
	case EquipmentSlot::Weapon:
		return equipment.weapon;
	case EquipmentSlot::Armor:
		return equipment.armor;
	case EquipmentSlot::Accessory:
		return equipment.accessory;
	}
	return equipment.weapon;
}

} // namespace dev
