#include "runtime/RuntimeGameplayOrchestratedFrameRunner.hpp"

namespace iggy::runtime {
namespace {

RuntimeGameplayOrchestratedFrameInput FrameInputFrom(
	const RuntimeGameplayState &state,
	const RuntimeGameplayOrchestratedFrameRunnerFrame &frame)
{
	RuntimeGameplayOrchestratedFrameInput input;
	input.playerFrame = frame.playerFrame;
	input.playerFrame.state = state;
	input.subjects = frame.subjects;
	input.pools = frame.pools;
	input.aiMap = frame.aiMap;
	input.controlConfig = frame.controlConfig;
	input.movementMap = frame.movementMap;
	input.movementConfig = frame.movementConfig;
	input.previousOccupancy = frame.previousOccupancy;
	input.interactionTargets = frame.interactionTargets;
	input.refreshAiMap = frame.refreshAiMap;
	input.refreshConfig = frame.refreshConfig;
	return input;
}

void AppendInventoryEvents(
	InventoryEventRecorder2D &events,
	const InventoryEventRecorder2D &frameEvents)
{
	for (const InventoryEvent2D &event : frameEvents.events) {
		recordInventoryEvent(events, event);
	}
}

void AppendFrame(
	RuntimeGameplayOrchestratedFrameRunnerResult &result,
	RuntimeGameplayOrchestratedFrameResult frameResult)
{
	AppendInventoryEvents(result.inventoryEvents, frameResult.inventoryEvents);
	result.acceptedCommandCount += frameResult.acceptedCommandCount;
	result.blockedIntentCount += frameResult.blockedIntentCount;
	result.rejectedIntentCount += frameResult.rejectedIntentCount;
	result.pickedUpCount += frameResult.pickedUpCount;
	result.inventoryEventCount += frameResult.inventoryEventCount;
	result.npcControlPlannedRequestCount += frameResult.npcControlPlannedRequestCount;
	result.npcControlAppliedCount += frameResult.npcControlAppliedCount;
	result.npcControlFailedCount += frameResult.npcControlFailedCount;
	result.npcMovementPlannedRequestCount += frameResult.npcMovementPlannedRequestCount;
	result.npcMovedCount += frameResult.npcMovedCount;
	result.npcBlockedMovementCount += frameResult.npcBlockedMovementCount;
	result.npcRejectedMovementCount += frameResult.npcRejectedMovementCount;
	result.npcMissingActorMovementCount += frameResult.npcMissingActorMovementCount;
	result.npcRefreshDirtyTileCount += frameResult.npcRefreshDirtyTileCount;
	result.interactionChanged = result.interactionChanged || frameResult.interactionChanged;
	result.inventoryChanged = result.inventoryChanged || frameResult.inventoryChanged;
	result.npcControlsChanged = result.npcControlsChanged || frameResult.npcControlsChanged;
	result.npcActorsChanged = result.npcActorsChanged || frameResult.npcActorsChanged;
	result.npcOccupancyRefreshed = result.npcOccupancyRefreshed || frameResult.npcOccupancyRefreshed;
	result.npcInteractionRefreshed = result.npcInteractionRefreshed || frameResult.npcInteractionRefreshed;
	result.npcAiMapRefreshed = result.npcAiMapRefreshed || frameResult.npcAiMapRefreshed;
	result.npcRenderRefreshed = result.npcRenderRefreshed || frameResult.npcRenderRefreshed;
	result.npcVisibilityRefreshed = result.npcVisibilityRefreshed || frameResult.npcVisibilityRefreshed;
	if (frameResult.changedGameplayState() || frameResult.refreshedNpcData()) {
		++result.changedFrameCount;
	}
	result.state = frameResult.state;
	result.frameResults.push_back(frameResult);
}

} // namespace

bool RuntimeGameplayOrchestratedFrameRunnerResult::hasFrames() const
{
	return frameCount > 0;
}

bool RuntimeGameplayOrchestratedFrameRunnerResult::changed() const
{
	return changedFrameCount > 0;
}

bool RuntimeGameplayOrchestratedFrameRunnerResult::refreshedNpcData() const
{
	return npcOccupancyRefreshed
		|| npcInteractionRefreshed
		|| npcAiMapRefreshed
		|| npcRenderRefreshed
		|| npcVisibilityRefreshed;
}

RuntimeGameplayOrchestratedFrameRunnerResult RuntimeGameplayOrchestratedFrameRunner::run(
	const RuntimeGameplayOrchestratedFrameRunnerInput &input) const
{
	RuntimeGameplayOrchestratedFrameRunnerResult result;
	result.initialState = input.initialState;
	result.frames = input.frames;
	result.state = input.initialState;
	result.frameCount = input.frames.size();

	RuntimeGameplayOrchestratedFrameStep step;
	for (const RuntimeGameplayOrchestratedFrameRunnerFrame &frame : input.frames) {
		AppendFrame(result, step.run(FrameInputFrom(result.state, frame)));
	}

	return result;
}

} // namespace iggy::runtime
