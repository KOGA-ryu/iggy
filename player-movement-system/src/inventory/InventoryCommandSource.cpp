#include "InventoryCommandSource.hpp"

namespace dev {

void QueuedInventoryCommandSource::enqueue(const InventoryCommand &command)
{
	commands_.push_back(command);
}

void QueuedInventoryCommandSource::clear()
{
	commands_.clear();
}

std::vector<InventoryCommand> QueuedInventoryCommandSource::drain()
{
	std::vector<InventoryCommand> drained;
	drained.swap(commands_);
	return drained;
}

bool QueuedInventoryCommandSource::empty() const
{
	return commands_.empty();
}

std::size_t QueuedInventoryCommandSource::size() const
{
	return commands_.size();
}

} // namespace dev
