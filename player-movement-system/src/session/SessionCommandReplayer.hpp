#pragma once

#include <vector>

#include "session/SessionCommandDispatcher.hpp"
#include "session/SessionCommandLog.hpp"

namespace dev {

class SessionCommandReplayer {
public:
	explicit SessionCommandReplayer(SessionCommandDispatcher &dispatcher);

	[[nodiscard]] std::vector<SessionCommandResult> replay(const SessionCommandLog &log) const;

private:
	SessionCommandDispatcher &dispatcher_;
};

} // namespace dev
