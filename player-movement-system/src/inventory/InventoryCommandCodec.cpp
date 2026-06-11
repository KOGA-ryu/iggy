#include "InventoryCommandCodec.hpp"

#include "inventory/InventoryCommandPacketByteCodec.hpp"
#include "inventory/InventoryCommandPacketValidator.hpp"

namespace dev {

InventoryCommandPacket InventoryCommandCodec::toPacket(const InventoryCommand &command) const
{
	InventoryCommandPacket packet;
	packet.commandType = static_cast<uint8_t>(command.type);
	if (command.itemId.has_value()) {
		packet.hasItemId = 1;
		packet.itemId = *command.itemId;
	}
	if (command.slot.has_value()) {
		packet.hasSlot = 1;
		packet.slot = static_cast<uint8_t>(*command.slot);
	}
	return packet;
}

std::optional<InventoryCommand> InventoryCommandCodec::fromPacket(const InventoryCommandPacket &packet) const
{
	if (!InventoryCommandPacketValidator {}.isValid(packet))
		return std::nullopt;

	InventoryCommand command {
		.type = static_cast<InventoryCommandType>(packet.commandType),
	};
	if (packet.hasItemId != 0U)
		command.itemId = packet.itemId;
	if (packet.hasSlot != 0U)
		command.slot = static_cast<EquipmentSlot>(packet.slot);
	return command;
}

InventoryCommandBytes InventoryCommandCodec::encode(const InventoryCommandPacket &packet) const
{
	return InventoryCommandPacketByteCodec {}.encode(packet);
}

std::optional<InventoryCommandPacket> InventoryCommandCodec::decode(const InventoryCommandBytes &bytes) const
{
	return InventoryCommandPacketByteCodec {}.decode(bytes);
}

} // namespace dev
