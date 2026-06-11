#include "CommandReplayer.hpp"

namespace dev {

CommandReplayer::CommandReplayer(CommandDispatcher &dispatcher)
    : dispatcher_(dispatcher)
{
}

CommandReplayReport CommandReplayer::replay(const CommandLog &log) const
{
	CommandReplayReport report;
	report.results.reserve(log.commands().size());
	for (const MovementCommand &command : log.commands()) {
		report.results.push_back(dispatcher_.dispatch(command));
	}
	return report;
}

} // namespace dev
