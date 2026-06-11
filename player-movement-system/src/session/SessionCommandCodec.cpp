#include "SessionCommandCodec.hpp"

#include <cstddef>

namespace dev {

namespace {

constexpr std::size_t PacketSize = 25;

void WriteU8(SessionCommandBytes &bytes, uint8_t value)
{
	bytes.push_back(value);
}

void WriteU16(SessionCommandBytes &bytes, uint16_t value)
{
	bytes.push_back(static_cast<uint8_t>(value & 0xFFU));
	bytes.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
}

void WriteU32(SessionCommandBytes &bytes, uint32_t value)
{
	WriteU16(bytes, static_cast<uint16_t>(value & 0xFFFFU));
	WriteU16(bytes, static_cast<uint16_t>((value >> 16U) & 0xFFFFU));
}

uint8_t ReadU8(const SessionCommandBytes &bytes, std::size_t &offset)
{
	return bytes[offset++];
}

uint16_t ReadU16(const SessionCommandBytes &bytes, std::size_t &offset)
{
	const uint16_t value = static_cast<uint16_t>(bytes[offset])
	    | (static_cast<uint16_t>(bytes[offset + 1U]) << 8U);
	offset += 2U;
	return value;
}

uint32_t ReadU32(const SessionCommandBytes &bytes, std::size_t &offset)
{
	const uint32_t low = ReadU16(bytes, offset);
	const uint32_t high = ReadU16(bytes, offset);
	return low | (high << 16U);
}

bool IsValidCommandType(uint8_t value)
{
	return value <= static_cast<uint8_t>(SessionCommandType::SetMode);
}

bool IsValidMode(uint8_t value)
{
	return value <= static_cast<uint8_t>(GameSessionMode::Inventory);
}

bool IsBoolean(uint8_t value)
{
	return value <= 1U;
}

bool HasOnlyExpectedPayload(const SessionCommandPacket &packet)
{
	const SessionCommandType type = static_cast<SessionCommandType>(packet.commandType);
	switch (type) {
	case SessionCommandType::StartNewGame:
		return packet.hasSlotId == 0U && packet.hasMode == 0U;
	case SessionCommandType::SaveSlot:
	case SessionCommandType::LoadSlot:
		return packet.hasNewGameSettings == 0U && packet.hasSlotId == 1U && packet.hasMode == 0U;
	case SessionCommandType::SetMode:
		return packet.hasNewGameSettings == 0U && packet.hasSlotId == 0U && packet.hasMode == 1U;
	}

	return false;
}

} // namespace

SessionCommandPacket SessionCommandCodec::toPacket(const SessionCommand &command) const
{
	SessionCommandPacket packet;
	packet.commandType = static_cast<uint8_t>(command.type);
	if (command.newGameSettings.has_value()) {
		packet.hasNewGameSettings = 1;
		packet.playerStartX = static_cast<int16_t>(command.newGameSettings->playerStart.x);
		packet.playerStartY = static_cast<int16_t>(command.newGameSettings->playerStart.y);
		packet.playerHitPoints = static_cast<int16_t>(command.newGameSettings->playerHitPoints);
	}
	if (command.slotId.has_value()) {
		packet.hasSlotId = 1;
		packet.slotId = *command.slotId;
	}
	if (command.mode.has_value()) {
		packet.hasMode = 1;
		packet.mode = static_cast<uint8_t>(*command.mode);
	}
	return packet;
}

std::optional<SessionCommand> SessionCommandCodec::fromPacket(const SessionCommandPacket &packet) const
{
	if (!IsValidCommandType(packet.commandType)
	    || !IsBoolean(packet.hasNewGameSettings)
	    || !IsBoolean(packet.hasSlotId)
	    || !IsBoolean(packet.hasMode)
	    || (packet.hasMode != 0U && !IsValidMode(packet.mode))
	    || !HasOnlyExpectedPayload(packet))
		return std::nullopt;

	SessionCommand command {
		.type = static_cast<SessionCommandType>(packet.commandType),
	};
	if (packet.hasNewGameSettings != 0U) {
		command.newGameSettings = NewGameSettings {
			.playerStart = { packet.playerStartX, packet.playerStartY },
			.playerHitPoints = packet.playerHitPoints,
		};
	}
	if (packet.hasSlotId != 0U)
		command.slotId = packet.slotId;
	if (packet.hasMode != 0U)
		command.mode = static_cast<GameSessionMode>(packet.mode);

	return command;
}

SessionCommandBytes SessionCommandCodec::encode(const SessionCommandPacket &packet) const
{
	SessionCommandBytes bytes;
	bytes.reserve(PacketSize);
	WriteU8(bytes, packet.commandType);
	WriteU8(bytes, packet.hasNewGameSettings);
	WriteU16(bytes, static_cast<uint16_t>(packet.playerStartX));
	WriteU16(bytes, static_cast<uint16_t>(packet.playerStartY));
	WriteU16(bytes, static_cast<uint16_t>(packet.playerHitPoints));
	WriteU8(bytes, packet.hasSlotId);
	WriteU32(bytes, packet.slotId);
	WriteU8(bytes, packet.hasMode);
	WriteU8(bytes, packet.mode);
	WriteU16(bytes, 0); // reserved
	WriteU32(bytes, 0); // reserved
	WriteU32(bytes, 0); // reserved
	return bytes;
}

std::optional<SessionCommandPacket> SessionCommandCodec::decode(const SessionCommandBytes &bytes) const
{
	if (bytes.size() != PacketSize)
		return std::nullopt;

	std::size_t offset = 0;
	SessionCommandPacket packet;
	packet.commandType = ReadU8(bytes, offset);
	packet.hasNewGameSettings = ReadU8(bytes, offset);
	packet.playerStartX = static_cast<int16_t>(ReadU16(bytes, offset));
	packet.playerStartY = static_cast<int16_t>(ReadU16(bytes, offset));
	packet.playerHitPoints = static_cast<int16_t>(ReadU16(bytes, offset));
	packet.hasSlotId = ReadU8(bytes, offset);
	packet.slotId = ReadU32(bytes, offset);
	packet.hasMode = ReadU8(bytes, offset);
	packet.mode = ReadU8(bytes, offset);
	ReadU16(bytes, offset); // reserved
	ReadU32(bytes, offset); // reserved
	ReadU32(bytes, offset); // reserved

	if (!fromPacket(packet).has_value())
		return std::nullopt;
	return packet;
}

} // namespace dev
