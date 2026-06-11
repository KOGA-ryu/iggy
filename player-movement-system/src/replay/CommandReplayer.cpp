#include "CommandReplayer.hpp"

namespace dev {

CommandReplayer::CommandReplayer(CommandDispatcher &dispatcher)
    : dispatcher_(dispatcher)
{
}

void CommandReplayer::replay(const CommandLog &log) const
{
	for (const MovementCommand &command : log.commands()) {
		dispatcher_.dispatch(command);
	}
}

} // namespace dev

