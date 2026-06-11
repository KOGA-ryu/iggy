#include "RuntimeSessionCommandIntake.hpp"

namespace dev {

std::vector<SessionCommandResult> RuntimeSessionCommandIntake::dispatch(
    std::vector<SessionCommand> commands,
    const SessionCommandDispatcher &dispatcher) const
{
	std::vector<SessionCommandResult> results;
	results.reserve(commands.size());
	for (const SessionCommand &command : commands)
		results.push_back(dispatcher.dispatch(command));
	return results;
}

} // namespace dev
