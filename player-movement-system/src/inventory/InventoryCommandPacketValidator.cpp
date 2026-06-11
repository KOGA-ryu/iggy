#include "InventoryCommandPacketValidator.hpp"

#include "inventory/InventoryCommand.hpp"

namespace dev {

bool InventoryCommandPacketValidator::isValid(const InventoryCommandPacket &packet) const
{
	return isValidCommandType(packet.commandType)
	    && isBoolean(packet.hasItemId)
	    && isBoolean(packet.hasSlot)
	    && (packet.hasSlot == 0U || isValidEquipmentSlot(packet.slot))
	    && hasOnlyExpectedPayload(packet);
}

bool InventoryCommandPacketValidator::isBoolean(uint8_t value) const
{
	return value <= 1U;
}

bool InventoryCommandPacketValidator::isValidCommandType(uint8_t value) const
{
	return value <= static_cast<uint8_t>(InventoryCommandType::UnequipSlot);
}

bool InventoryCommandPacketValidator::isValidEquipmentSlot(uint8_t value) const
{
	return value <= static_cast<uint8_t>(EquipmentSlot::Accessory);
}

bool InventoryCommandPacketValidator::hasOnlyExpectedPayload(const InventoryCommandPacket &packet) const
{
	const InventoryCommandType type = static_cast<InventoryCommandType>(packet.commandType);
	switch (type) {
	case InventoryCommandType::EquipItem:
		return packet.hasItemId == 1U && packet.hasSlot == 0U;
	case InventoryCommandType::UnequipSlot:
		return packet.hasItemId == 0U && packet.hasSlot == 1U;
	}

	return false;
}

} // namespace dev
