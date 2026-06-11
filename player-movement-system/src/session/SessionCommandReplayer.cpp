#include "SessionCommandReplayer.hpp"

namespace dev {

SessionCommandReplayer::SessionCommandReplayer(SessionCommandDispatcher &dispatcher)
    : dispatcher_(dispatcher)
{
}

std::vector<SessionCommandResult> SessionCommandReplayer::replay(const SessionCommandLog &log) const
{
	std::vector<SessionCommandResult> results;
	results.reserve(log.commands().size());
	for (const SessionCommand &command : log.commands()) {
		results.push_back(dispatcher_.dispatch(command));
	}
	return results;
}

} // namespace dev
