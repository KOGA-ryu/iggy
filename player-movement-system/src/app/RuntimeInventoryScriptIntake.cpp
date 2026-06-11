#include "RuntimeInventoryScriptIntake.hpp"

#include "inventory/InventoryCommandDispatcher.hpp"

namespace dev {

InventoryScriptRunResult RuntimeInventoryScriptIntake::run(
    const std::filesystem::path &path,
    Player *player,
    InventoryEventSink *eventSink) const
{
	if (player == nullptr)
		return { .status = InventoryScriptRunStatus::NoActivePlayer };

	InventoryCommandDispatcher dispatcher { *player, eventSink };
	InventoryScriptRunner runner { dispatcher };
	return runner.run(path);
}

} // namespace dev
