#pragma once

#include "inventory/InventoryCommandPacket.hpp"

namespace dev {

class InventoryCommandPacketValidator {
public:
	[[nodiscard]] bool isValid(const InventoryCommandPacket &packet) const;

private:
	[[nodiscard]] bool isBoolean(uint8_t value) const;
	[[nodiscard]] bool isValidCommandType(uint8_t value) const;
	[[nodiscard]] bool isValidEquipmentSlot(uint8_t value) const;
	[[nodiscard]] bool hasOnlyExpectedPayload(const InventoryCommandPacket &packet) const;
};

} // namespace dev
