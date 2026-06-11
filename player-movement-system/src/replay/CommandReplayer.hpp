#pragma once

#include "commands/CommandDispatcher.hpp"
#include "replay/CommandLog.hpp"

namespace dev {

class CommandReplayer {
public:
	explicit CommandReplayer(CommandDispatcher &dispatcher);

	void replay(const CommandLog &log) const;

private:
	CommandDispatcher &dispatcher_;
};

} // namespace dev

