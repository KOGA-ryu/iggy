#include "RuntimeInventoryScriptBatchRunner.hpp"

#include "app/RuntimeInventoryScriptIntake.hpp"

namespace dev {

std::vector<InventoryScriptRunResult> RuntimeInventoryScriptBatchRunner::run(
    std::vector<std::filesystem::path> paths,
    Player *player,
    InventoryEventSink *eventSink) const
{
	std::vector<InventoryScriptRunResult> results;
	results.reserve(paths.size());

	RuntimeInventoryScriptIntake intake;
	for (const std::filesystem::path &path : paths)
		results.push_back(intake.run(path, player, eventSink));

	return results;
}

} // namespace dev
