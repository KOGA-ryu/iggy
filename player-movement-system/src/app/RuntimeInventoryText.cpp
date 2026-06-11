#include "RuntimeInventoryText.hpp"

#include <sstream>

namespace dev {

namespace {

const char *ToString(InventoryCommandResultType type)
{
	switch (type) {
	case InventoryCommandResultType::Applied:
		return "Applied";
	case InventoryCommandResultType::Rejected:
		return "Rejected";
	}
	return "Unknown";
}

const char *ToString(InventoryCommandType type)
{
	switch (type) {
	case InventoryCommandType::EquipItem:
		return "EquipItem";
	case InventoryCommandType::UnequipSlot:
		return "UnequipSlot";
	}
	return "Unknown";
}

const char *ToString(EquipmentResultType type)
{
	switch (type) {
	case EquipmentResultType::Equipped:
		return "Equipped";
	case EquipmentResultType::Unequipped:
		return "Unequipped";
	case EquipmentResultType::MissingItem:
		return "MissingItem";
	case EquipmentResultType::NotEquippable:
		return "NotEquippable";
	case EquipmentResultType::WrongSlot:
		return "WrongSlot";
	case EquipmentResultType::InventoryFull:
		return "InventoryFull";
	case EquipmentResultType::EmptySlot:
		return "EmptySlot";
	}
	return "Unknown";
}

const char *ToString(EquipmentSlot slot)
{
	switch (slot) {
	case EquipmentSlot::Weapon:
		return "Weapon";
	case EquipmentSlot::Armor:
		return "Armor";
	case EquipmentSlot::Accessory:
		return "Accessory";
	}
	return "Unknown";
}

const char *ToString(InventoryEventType type)
{
	switch (type) {
	case InventoryEventType::Equipped:
		return "Equipped";
	case InventoryEventType::Unequipped:
		return "Unequipped";
	case InventoryEventType::Rejected:
		return "Rejected";
	}
	return "Unknown";
}

} // namespace

std::string RuntimeInventoryText::formatResult(std::string_view label, const InventoryCommandResult &result) const
{
	std::ostringstream line;
	line << label << " type=" << ToString(result.type)
	     << " command=" << ToString(result.command.type)
	     << " equipment=" << ToString(result.equipmentResult.type)
	     << " item=" << result.equipmentResult.itemId
	     << " slot=" << ToString(result.equipmentResult.slot);
	return line.str();
}

std::string RuntimeInventoryText::formatEvent(std::string_view label, const InventoryEvent &event) const
{
	std::ostringstream line;
	line << label << " type=" << ToString(event.type)
	     << " command=" << ToString(event.commandType)
	     << " result=" << ToString(event.commandResult)
	     << " equipment=" << ToString(event.equipmentResult)
	     << " item=" << event.itemId
	     << " slot=" << ToString(event.slot);
	return line.str();
}

} // namespace dev
