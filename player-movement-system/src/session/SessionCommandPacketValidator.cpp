#include "SessionCommandPacketValidator.hpp"

#include "session/SessionCommand.hpp"

namespace dev {

bool SessionCommandPacketValidator::isValid(const SessionCommandPacket &packet) const
{
	return isValidCommandType(packet.commandType)
	    && isBoolean(packet.hasNewGameSettings)
	    && isBoolean(packet.hasSlotId)
	    && isBoolean(packet.hasMode)
	    && (packet.hasMode == 0U || isValidMode(packet.mode))
	    && hasOnlyExpectedPayload(packet);
}

bool SessionCommandPacketValidator::isValidCommandType(uint8_t value) const
{
	return value <= static_cast<uint8_t>(SessionCommandType::SetMode);
}

bool SessionCommandPacketValidator::isValidMode(uint8_t value) const
{
	return value <= static_cast<uint8_t>(GameSessionMode::Inventory);
}

bool SessionCommandPacketValidator::isBoolean(uint8_t value) const
{
	return value <= 1U;
}

bool SessionCommandPacketValidator::hasOnlyExpectedPayload(const SessionCommandPacket &packet) const
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

} // namespace dev
