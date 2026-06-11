#pragma once

#include <optional>
#include <vector>

#include "session/SessionCommandLogBytes.hpp"
#include "session/SessionCommandPacket.hpp"

namespace dev {

class SessionCommandLogFrameCodec {
public:
	[[nodiscard]] SessionCommandLogBytes encode(const std::vector<SessionCommandBytes> &packets) const;
	[[nodiscard]] std::optional<std::vector<SessionCommandBytes>> decode(const SessionCommandLogBytes &bytes) const;
};

} // namespace dev
