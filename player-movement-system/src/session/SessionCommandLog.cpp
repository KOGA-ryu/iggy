#include "SessionCommandLog.hpp"

namespace dev {

void SessionCommandLog::record(const SessionCommand &command)
{
	commands_.push_back(command);
}

void SessionCommandLog::clear()
{
	commands_.clear();
}

const std::vector<SessionCommand> &SessionCommandLog::commands() const
{
	return commands_;
}

bool SessionCommandLog::empty() const
{
	return commands_.empty();
}

} // namespace dev
