#pragma once

#include <optional>

#include "session/SessionCommand.hpp"
#include "session/SessionCommandPacket.hpp"

namespace dev {

class SessionCommandCodec {
public:
	[[nodiscard]] SessionCommandPacket toPacket(const SessionCommand &command) const;
	[[nodiscard]] std::optional<SessionCommand> fromPacket(const SessionCommandPacket &packet) const;

	[[nodiscard]] SessionCommandBytes encode(const SessionCommandPacket &packet) const;
	[[nodiscard]] std::optional<SessionCommandPacket> decode(const SessionCommandBytes &bytes) const;
};

} // namespace dev
