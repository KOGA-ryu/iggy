#include "SessionCommandSource.hpp"

namespace dev {

void QueuedSessionCommandSource::enqueue(const SessionCommand &command)
{
	commands_.push_back(command);
}

void QueuedSessionCommandSource::clear()
{
	commands_.clear();
}

std::vector<SessionCommand> QueuedSessionCommandSource::drain()
{
	std::vector<SessionCommand> drained;
	drained.swap(commands_);
	return drained;
}

bool QueuedSessionCommandSource::empty() const
{
	return commands_.empty();
}

std::size_t QueuedSessionCommandSource::size() const
{
	return commands_.size();
}

} // namespace dev
