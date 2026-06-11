#include "SessionCommandCodec.hpp"

#include "session/SessionCommandPacketByteCodec.hpp"
#include "session/SessionCommandPacketValidator.hpp"

namespace dev {

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
	if (!SessionCommandPacketValidator {}.isValid(packet))
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
	return SessionCommandPacketByteCodec {}.encode(packet);
}

std::optional<SessionCommandPacket> SessionCommandCodec::decode(const SessionCommandBytes &bytes) const
{
	return SessionCommandPacketByteCodec {}.decode(bytes);
}

} // namespace dev
