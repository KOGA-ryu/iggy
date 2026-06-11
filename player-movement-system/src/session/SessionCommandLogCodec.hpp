#pragma once

#include <optional>

#include "session/SessionCommandCodec.hpp"
#include "session/SessionCommandLogBytes.hpp"
#include "session/SessionCommandLog.hpp"
#include "session/SessionCommandLogFrameCodec.hpp"

namespace dev {

class SessionCommandLogCodec {
public:
	[[nodiscard]] SessionCommandLogBytes encode(const SessionCommandLog &log) const;
	[[nodiscard]] std::optional<SessionCommandLog> decode(const SessionCommandLogBytes &bytes) const;

private:
	SessionCommandCodec commandCodec_;
	SessionCommandLogFrameCodec frameCodec_;
};

} // namespace dev
