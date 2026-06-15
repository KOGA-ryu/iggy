#include "runtime/RuntimeGameplayScenarioRunner.hpp"

namespace iggy::runtime {
namespace {

std::vector<RuntimeGameplayOrchestratedFrameRunnerFrame> FramesFromScenario(
	const std::vector<RuntimeGameplayScenarioFrame> &frames)
{
	std::vector<RuntimeGameplayOrchestratedFrameRunnerFrame> runnerFrames;
	runnerFrames.reserve(frames.size());
	for (const RuntimeGameplayScenarioFrame &frame : frames) {
		runnerFrames.push_back(frame.frame);
	}
	return runnerFrames;
}

void CopyReportCounts(RuntimeGameplayScenarioResult &result)
{
	result.frameCount = result.report.frameCount;
	result.changedFrameCount = result.report.changedFrameCount;
	result.inventoryEventCount = result.report.inventoryEventCount;
	result.npcControlPlannedRequestCount = result.report.npcControlPlannedRequestCount;
	result.npcControlAppliedCount = result.report.npcControlAppliedCount;
	result.npcControlFailedCount = result.report.npcControlFailedCount;
	result.npcMovementPlannedRequestCount = result.report.npcMovementPlannedRequestCount;
	result.npcMovedCount = result.report.npcMovedCount;
	result.npcBlockedMovementCount = result.report.npcBlockedMovementCount;
	result.npcRejectedMovementCount = result.report.npcRejectedMovementCount;
	result.npcMissingActorMovementCount = result.report.npcMissingActorMovementCount;
	result.npcRefreshDirtyTileCount = result.report.npcRefreshDirtyTileCount;
	result.npcControlsChanged = result.report.npcControlsChanged;
	result.npcActorsChanged = result.report.npcActorsChanged;
	result.npcOccupancyRefreshed = result.report.npcOccupancyRefreshed;
	result.npcInteractionRefreshed = result.report.npcInteractionRefreshed;
	result.npcAiMapRefreshed = result.report.npcAiMapRefreshed;
	result.npcRenderRefreshed = result.report.npcRenderRefreshed;
	result.npcVisibilityRefreshed = result.report.npcVisibilityRefreshed;
}

} // namespace

bool RuntimeGameplayScenarioResult::hasFrames() const
{
	return frameCount > 0;
}

bool RuntimeGameplayScenarioResult::changed() const
{
	return changedFrameCount > 0;
}

bool RuntimeGameplayScenarioResult::refreshedNpcData() const
{
	return npcOccupancyRefreshed
		|| npcInteractionRefreshed
		|| npcAiMapRefreshed
		|| npcRenderRefreshed
		|| npcVisibilityRefreshed;
}

RuntimeGameplayScenarioResult RuntimeGameplayScenarioRunner::run(const RuntimeGameplayScenario &scenario) const
{
	RuntimeGameplayScenarioResult result;
	result.scenario = scenario;

	RuntimeGameplayOrchestratedFrameRunnerInput runnerInput;
	runnerInput.initialState = scenario.initialState;
	runnerInput.frames = FramesFromScenario(scenario.frames);

	result.runner = RuntimeGameplayOrchestratedFrameRunner {}.run(runnerInput);
	result.report = RuntimeGameplayOrchestratedFrameRunnerReporter {}.report(result.runner);
	result.state = result.runner.state;
	CopyReportCounts(result);
	return result;
}

} // namespace iggy::runtime
