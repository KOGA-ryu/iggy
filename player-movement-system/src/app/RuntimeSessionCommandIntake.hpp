#pragma once

#include <vector>

#include "session/SessionCommand.hpp"
#include "session/SessionCommandDispatcher.hpp"

namespace dev {

class RuntimeSessionCommandIntake {
public:
	[[nodiscard]] std::vector<SessionCommandResult> dispatch(
	    std::vector<SessionCommand> commands,
	    const SessionCommandDispatcher &dispatcher) const;
};

} // namespace dev
