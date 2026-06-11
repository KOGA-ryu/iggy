#pragma once

#include "commands/CommandDispatcher.hpp"
#include "replay/CommandLog.hpp"
#include "replay/CommandReplayReport.hpp"

namespace dev {

class CommandReplayer {
public:
	explicit CommandReplayer(CommandDispatcher &dispatcher);

	[[nodiscard]] CommandReplayReport replay(const CommandLog &log) const;

private:
	CommandDispatcher &dispatcher_;
};

} // namespace dev
