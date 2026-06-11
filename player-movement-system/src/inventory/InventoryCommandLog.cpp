#include "InventoryCommandLog.hpp"

namespace dev {

void InventoryCommandLog::record(const InventoryCommand &command)
{
	commands_.push_back(command);
}

void InventoryCommandLog::clear()
{
	commands_.clear();
}

const std::vector<InventoryCommand> &InventoryCommandLog::commands() const
{
	return commands_;
}

bool InventoryCommandLog::empty() const
{
	return commands_.empty();
}

} // namespace dev
