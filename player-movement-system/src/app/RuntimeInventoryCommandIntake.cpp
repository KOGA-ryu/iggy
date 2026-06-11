#include "RuntimeInventoryCommandIntake.hpp"

#include "inventory/InventoryCommandDispatcher.hpp"
#include "inventory/InventoryCommandEventEmitter.hpp"

namespace dev {

std::vector<InventoryCommandResult> RuntimeInventoryCommandIntake::dispatch(
    std::vector<InventoryCommand> commands,
    Player *player,
    InventoryEventSink *eventSink) const
{
	std::vector<InventoryCommandResult> results;
	results.reserve(commands.size());

	if (player == nullptr) {
		InventoryCommandEventEmitter events { eventSink };
		for (const InventoryCommand &command : commands) {
			InventoryCommandResult result { .type = InventoryCommandResultType::Rejected, .command = command };
			events.emit(result);
			results.push_back(result);
		}
		return results;
	}

	InventoryCommandDispatcher dispatcher { *player, eventSink };
	for (const InventoryCommand &command : commands)
		results.push_back(dispatcher.dispatch(command));

	return results;
}

} // namespace dev
