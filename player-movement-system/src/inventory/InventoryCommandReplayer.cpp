#include "InventoryCommandReplayer.hpp"

namespace dev {

InventoryCommandReplayer::InventoryCommandReplayer(InventoryCommandDispatcher &dispatcher)
    : dispatcher_(dispatcher)
{
}

std::vector<InventoryCommandResult> InventoryCommandReplayer::replay(const InventoryCommandLog &log) const
{
	std::vector<InventoryCommandResult> results;
	results.reserve(log.commands().size());
	for (const InventoryCommand &command : log.commands())
		results.push_back(dispatcher_.dispatch(command));
	return results;
}

} // namespace dev
