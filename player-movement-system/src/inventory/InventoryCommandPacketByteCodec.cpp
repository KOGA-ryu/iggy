#include "InventoryCommandPacketByteCodec.hpp"

#include <cstddef>

#include "inventory/InventoryCommandByteStream.hpp"
#include "inventory/InventoryCommandPacketValidator.hpp"

namespace dev {

namespace {

constexpr std::size_t PacketSize = 16;

} // namespace

InventoryCommandBytes InventoryCommandPacketByteCodec::encode(const InventoryCommandPacket &packet) const
{
	InventoryCommandBytes bytes;
	bytes.reserve(PacketSize);
	InventoryCommandByteWriter writer { bytes };
	writer.writeU8(packet.commandType);
	writer.writeU8(packet.hasItemId);
	writer.writeU32(packet.itemId);
	writer.writeU8(packet.hasSlot);
	writer.writeU8(packet.slot);
	writer.writeU16(0); // reserved
	writer.writeU32(0); // reserved
	writer.writeU16(0); // reserved
	return bytes;
}

std::optional<InventoryCommandPacket> InventoryCommandPacketByteCodec::decode(const InventoryCommandBytes &bytes) const
{
	if (bytes.size() != PacketSize)
		return std::nullopt;

	InventoryCommandByteReader reader { bytes };
	InventoryCommandPacket packet;
	uint16_t reserved16 = 0;
	uint32_t reserved32 = 0;
	if (!reader.readU8(packet.commandType)
	    || !reader.readU8(packet.hasItemId)
	    || !reader.readU32(packet.itemId)
	    || !reader.readU8(packet.hasSlot)
	    || !reader.readU8(packet.slot)
	    || !reader.readU16(reserved16)
	    || !reader.readU32(reserved32)
	    || !reader.readU16(reserved16))
		return std::nullopt;

	if (!InventoryCommandPacketValidator {}.isValid(packet))
		return std::nullopt;
	return packet;
}

} // namespace dev
