#pragma once

#include "session/SessionCommandPacket.hpp"

namespace dev {

class SessionCommandPacketValidator {
public:
	[[nodiscard]] bool isValid(const SessionCommandPacket &packet) const;

private:
	[[nodiscard]] bool isValidCommandType(uint8_t value) const;
	[[nodiscard]] bool isValidMode(uint8_t value) const;
	[[nodiscard]] bool isBoolean(uint8_t value) const;
	[[nodiscard]] bool hasOnlyExpectedPayload(const SessionCommandPacket &packet) const;
};

} // namespace dev
