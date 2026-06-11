#include "CommandLog.hpp"

namespace dev {

void CommandLog::record(const MovementCommand &command)
{
	commands_.push_back(command);
}

void CommandLog::clear()
{
	commands_.clear();
}

const std::vector<MovementCommand> &CommandLog::commands() const
{
	return commands_;
}

bool CommandLog::empty() const
{
	return commands_.empty();
}

} // namespace dev

