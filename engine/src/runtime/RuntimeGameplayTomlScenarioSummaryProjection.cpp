#include "runtime/RuntimeGameplayTomlScenarioSummaryProjection.hpp"

namespace iggy::runtime {

RuntimeGameplayTomlScenarioRunSummaryProjection
projectRuntimeGameplayTomlScenarioRunSummary(
	const std::filesystem::path &sourcePath,
	const RuntimeGameplayProfileScenarioRunResult &run,
	const std::vector<std::string> &finalRows)
{
	RuntimeGameplayTomlScenarioRunSummaryProjection projection;
	projection.sourcePath = sourcePath;
	projection.frameCount = run.frameCount;
	projection.acceptedCommandCount =
		run.scenario.runner.acceptedCommandCount;
	projection.pickedUpCount = run.scenario.runner.pickedUpCount;
	projection.interactionChanged = run.scenario.runner.interactionChanged;
	projection.npcMovedCount = run.npcMovedCount;
	projection.npcBlockedMovementCount = run.npcBlockedMovementCount;
	projection.finalRows = finalRows;
	return projection;
}

} // namespace iggy::runtime
