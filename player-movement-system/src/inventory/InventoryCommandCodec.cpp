#include "InventoryCommandCodec.hpp"

#include <cstddef>

namespace dev {

namespace {

constexpr std::size_t PacketSize = 16;

void WriteU8(InventoryCommandBytes &bytes, uint8_t value)
{
	bytes.push_back(value);
}

void WriteU16(InventoryCommandBytes &bytes, uint16_t value)
{
	bytes.push_back(static_cast<uint8_t>(value & 0xFFU));
	bytes.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
}

void WriteU32(InventoryCommandBytes &bytes, uint32_t value)
{
	WriteU16(bytes, static_cast<uint16_t>(value & 0xFFFFU));
	WriteU16(bytes, static_cast<uint16_t>((value >> 16U) & 0xFFFFU));
}

uint8_t ReadU8(const InventoryCommandBytes &bytes, std::size_t &offset)
{
	return bytes[offset++];
}

uint16_t ReadU16(const InventoryCommandBytes &bytes, std::size_t &offset)
{
	const uint16_t value = static_cast<uint16_t>(bytes[offset])
	    | (static_cast<uint16_t>(bytes[offset + 1U]) << 8U);
	offset += 2U;
	return value;
}

uint32_t ReadU32(const InventoryCommandBytes &bytes, std::size_t &offset)
{
	const uint32_t low = ReadU16(bytes, offset);
	const uint32_t high = ReadU16(bytes, offset);
	return low | (high << 16U);
}

bool IsBoolean(uint8_t value)
{
	return value <= 1U;
}

bool IsValidCommandType(uint8_t value)
{
	return value <= static_cast<uint8_t>(InventoryCommandType::UnequipSlot);
}

bool IsValidEquipmentSlot(uint8_t value)
{
	return value <= static_cast<uint8_t>(EquipmentSlot::Accessory);
}

bool HasOnlyExpectedPayload(const InventoryCommandPacket &packet)
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

} // namespace

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
	if (!IsValidCommandType(packet.commandType)
	    || !IsBoolean(packet.hasItemId)
	    || !IsBoolean(packet.hasSlot)
	    || (packet.hasSlot != 0U && !IsValidEquipmentSlot(packet.slot))
	    || !HasOnlyExpectedPayload(packet))
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
	InventoryCommandBytes bytes;
	bytes.reserve(PacketSize);
	WriteU8(bytes, packet.commandType);
	WriteU8(bytes, packet.hasItemId);
	WriteU32(bytes, packet.itemId);
	WriteU8(bytes, packet.hasSlot);
	WriteU8(bytes, packet.slot);
	WriteU16(bytes, 0); // reserved
	WriteU32(bytes, 0); // reserved
	WriteU16(bytes, 0); // reserved
	return bytes;
}

std::optional<InventoryCommandPacket> InventoryCommandCodec::decode(const InventoryCommandBytes &bytes) const
{
	if (bytes.size() != PacketSize)
		return std::nullopt;

	std::size_t offset = 0;
	InventoryCommandPacket packet;
	packet.commandType = ReadU8(bytes, offset);
	packet.hasItemId = ReadU8(bytes, offset);
	packet.itemId = ReadU32(bytes, offset);
	packet.hasSlot = ReadU8(bytes, offset);
	packet.slot = ReadU8(bytes, offset);
	ReadU16(bytes, offset); // reserved
	ReadU32(bytes, offset); // reserved
	ReadU16(bytes, offset); // reserved

	if (!fromPacket(packet).has_value())
		return std::nullopt;
	return packet;
}

} // namespace dev
