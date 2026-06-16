#include "runtime/RuntimeGameplayScenarioRunner.hpp"

#include "runtime/RuntimeNpcOrchestrationAggregates.hpp"

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
	RuntimeNpcControlAggregate control;
	RuntimeNpcMovementAggregate movement;
	RuntimeNpcRefreshAggregate refresh;
	foldRuntimeNpcControlAggregate(control, result.report);
	foldRuntimeNpcMovementAggregate(movement, result.report);
	foldRuntimeNpcRefreshAggregate(refresh, result.report);

	result.frameCount = result.report.frameCount;
	result.changedFrameCount = result.report.changedFrameCount;
	result.inventoryEventCount = result.report.inventoryEventCount;
	result.npcControlPlannedRequestCount = control.plannedRequestCount;
	result.npcControlAppliedCount = control.appliedCount;
	result.npcControlFailedCount = control.failedCount;
	result.npcMovementPlannedRequestCount = movement.plannedRequestCount;
	result.npcMovedCount = movement.movedCount;
	result.npcBlockedMovementCount = movement.blockedMovementCount;
	result.npcRejectedMovementCount = movement.rejectedMovementCount;
	result.npcMissingActorMovementCount = movement.missingActorMovementCount;
	result.npcRefreshDirtyTileCount = refresh.dirtyTileCount;
	result.npcControlsChanged = control.controlsChanged;
	result.npcActorsChanged = movement.actorsChanged;
	result.npcOccupancyRefreshed = refresh.occupancyRefreshed;
	result.npcInteractionRefreshed = refresh.interactionRefreshed;
	result.npcAiMapRefreshed = refresh.aiMapRefreshed;
	result.npcRenderRefreshed = refresh.renderRefreshed;
	result.npcVisibilityRefreshed = refresh.visibilityRefreshed;
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
