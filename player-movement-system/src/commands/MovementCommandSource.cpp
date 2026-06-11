#include "MovementCommandSource.hpp"

namespace dev {

void QueuedMovementCommandSource::enqueue(const MovementCommand &command)
{
	commands_.push_back(command);
}

void QueuedMovementCommandSource::clear()
{
	commands_.clear();
}

std::vector<MovementCommand> QueuedMovementCommandSource::drain()
{
	std::vector<MovementCommand> drained;
	drained.swap(commands_);
	return drained;
}

bool QueuedMovementCommandSource::empty() const
{
	return commands_.empty();
}

std::size_t QueuedMovementCommandSource::size() const
{
	return commands_.size();
}

} // namespace dev
