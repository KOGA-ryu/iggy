#pragma once

#include <optional>

#include "session/SessionCommandPacket.hpp"

namespace dev {

class SessionCommandPacketByteCodec {
public:
	[[nodiscard]] SessionCommandBytes encode(const SessionCommandPacket &packet) const;
	[[nodiscard]] std::optional<SessionCommandPacket> decode(const SessionCommandBytes &bytes) const;
};

} // namespace dev
