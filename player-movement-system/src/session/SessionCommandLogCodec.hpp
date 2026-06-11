#pragma once

#include <optional>

#include "session/SessionCommandCodec.hpp"
#include "session/SessionCommandLog.hpp"

namespace dev {

using SessionCommandLogBytes = std::vector<uint8_t>;

class SessionCommandLogCodec {
public:
	[[nodiscard]] SessionCommandLogBytes encode(const SessionCommandLog &log) const;
	[[nodiscard]] std::optional<SessionCommandLog> decode(const SessionCommandLogBytes &bytes) const;

private:
	SessionCommandCodec commandCodec_;
};

} // namespace dev
