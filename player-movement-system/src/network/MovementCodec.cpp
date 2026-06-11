#include "MovementCodec.hpp"

#include <cstddef>

namespace dev {

namespace {

constexpr std::size_t PacketSize = 26;

void WriteU8(PacketBytes &bytes, uint8_t value)
{
	bytes.push_back(value);
}

void WriteU16(PacketBytes &bytes, uint16_t value)
{
	bytes.push_back(static_cast<uint8_t>(value & 0xFF));
	bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void WriteU32(PacketBytes &bytes, uint32_t value)
{
	WriteU16(bytes, static_cast<uint16_t>(value & 0xFFFF));
	WriteU16(bytes, static_cast<uint16_t>((value >> 16) & 0xFFFF));
}

uint8_t ReadU8(const PacketBytes &bytes, std::size_t &offset)
{
	return bytes[offset++];
}

uint16_t ReadU16(const PacketBytes &bytes, std::size_t &offset)
{
	const uint16_t value = static_cast<uint16_t>(bytes[offset])
	    | (static_cast<uint16_t>(bytes[offset + 1]) << 8);
	offset += 2;
	return value;
}

uint32_t ReadU32(const PacketBytes &bytes, std::size_t &offset)
{
	const uint32_t low = ReadU16(bytes, offset);
	const uint32_t high = ReadU16(bytes, offset);
	return low | (high << 16);
}

bool IsValidCommandType(uint8_t value)
{
	return value <= static_cast<uint8_t>(MovementCommandType::Stop);
}

bool IsValidActionType(uint8_t value)
{
	return value <= static_cast<uint8_t>(DestinationActionType::Interact);
}

bool IsValidTargetType(uint8_t value)
{
	return value <= static_cast<uint8_t>(TargetType::Object);
}

} // namespace

MovementPacket MovementCodec::toPacket(const MovementCommand &command) const
{
	MovementPacket packet;
	packet.commandType = static_cast<uint8_t>(command.type);
	packet.playerId = command.playerId;
	packet.destinationX = static_cast<int16_t>(command.destination.x);
	packet.destinationY = static_cast<int16_t>(command.destination.y);

	if (command.destinationAction.has_value()) {
		const DestinationAction &action = *command.destinationAction;
		packet.hasDestinationAction = 1;
		packet.actionType = static_cast<uint8_t>(action.type);
		packet.targetType = static_cast<uint8_t>(action.target.type);
		packet.targetId = action.target.id;
		packet.targetX = static_cast<int16_t>(action.target.tile.x);
		packet.targetY = static_cast<int16_t>(action.target.tile.y);
		packet.rangeTiles = static_cast<int16_t>(action.rangeTiles);
	}

	return packet;
}

std::optional<MovementCommand> MovementCodec::fromPacket(const MovementPacket &packet) const
{
	if (!IsValidCommandType(packet.commandType))
		return std::nullopt;
	if (packet.hasDestinationAction > 1)
		return std::nullopt;
	if (packet.hasDestinationAction != 0 && (!IsValidActionType(packet.actionType) || !IsValidTargetType(packet.targetType)))
		return std::nullopt;

	MovementCommand command {
		.type = static_cast<MovementCommandType>(packet.commandType),
		.playerId = packet.playerId,
		.destination = { packet.destinationX, packet.destinationY },
	};

	if (packet.hasDestinationAction != 0) {
		command.destinationAction = DestinationAction {
			static_cast<DestinationActionType>(packet.actionType),
			Target {
				static_cast<TargetType>(packet.targetType),
				packet.targetId,
				{ packet.targetX, packet.targetY },
			},
			packet.rangeTiles,
		};
	}

	return command;
}

PacketBytes MovementCodec::encode(const MovementPacket &packet) const
{
	PacketBytes bytes;
	bytes.reserve(PacketSize);
	WriteU8(bytes, packet.commandType);
	WriteU8(bytes, packet.playerId);
	WriteU16(bytes, static_cast<uint16_t>(packet.destinationX));
	WriteU16(bytes, static_cast<uint16_t>(packet.destinationY));
	WriteU8(bytes, packet.hasDestinationAction);
	WriteU8(bytes, packet.actionType);
	WriteU8(bytes, packet.targetType);
	WriteU8(bytes, 0); // reserved for version/flags
	WriteU32(bytes, packet.targetId);
	WriteU16(bytes, static_cast<uint16_t>(packet.targetX));
	WriteU16(bytes, static_cast<uint16_t>(packet.targetY));
	WriteU16(bytes, static_cast<uint16_t>(packet.rangeTiles));
	WriteU16(bytes, 0); // reserved
	WriteU32(bytes, 0); // reserved
	return bytes;
}

std::optional<MovementPacket> MovementCodec::decode(const PacketBytes &bytes) const
{
	if (bytes.size() != PacketSize)
		return std::nullopt;

	std::size_t offset = 0;
	MovementPacket packet;
	packet.commandType = ReadU8(bytes, offset);
	packet.playerId = ReadU8(bytes, offset);
	packet.destinationX = static_cast<int16_t>(ReadU16(bytes, offset));
	packet.destinationY = static_cast<int16_t>(ReadU16(bytes, offset));
	packet.hasDestinationAction = ReadU8(bytes, offset);
	packet.actionType = ReadU8(bytes, offset);
	packet.targetType = ReadU8(bytes, offset);
	ReadU8(bytes, offset); // reserved
	packet.targetId = ReadU32(bytes, offset);
	packet.targetX = static_cast<int16_t>(ReadU16(bytes, offset));
	packet.targetY = static_cast<int16_t>(ReadU16(bytes, offset));
	packet.rangeTiles = static_cast<int16_t>(ReadU16(bytes, offset));
	ReadU16(bytes, offset); // reserved
	ReadU32(bytes, offset); // reserved

	if (!fromPacket(packet).has_value())
		return std::nullopt;
	return packet;
}

} // namespace dev
