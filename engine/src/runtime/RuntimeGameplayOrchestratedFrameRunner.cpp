#include "runtime/RuntimeGameplayOrchestratedFrameRunner.hpp"

#include "runtime/RuntimeNpcOrchestrationAggregates.hpp"

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
	RuntimeNpcControlAggregate &control,
	RuntimeNpcMovementAggregate &movement,
	RuntimeNpcRefreshAggregate &refresh,
	RuntimeGameplayOrchestratedFrameResult frameResult)
{
	AppendInventoryEvents(result.inventoryEvents, frameResult.inventoryEvents);
	result.acceptedCommandCount += frameResult.acceptedCommandCount;
	result.blockedIntentCount += frameResult.blockedIntentCount;
	result.rejectedIntentCount += frameResult.rejectedIntentCount;
	result.pickedUpCount += frameResult.pickedUpCount;
	result.inventoryEventCount += frameResult.inventoryEventCount;
	foldRuntimeNpcControlAggregate(control, frameResult);
	foldRuntimeNpcMovementAggregate(movement, frameResult);
	foldRuntimeNpcRefreshAggregate(refresh, frameResult);
	result.interactionChanged = result.interactionChanged || frameResult.interactionChanged;
	result.inventoryChanged = result.inventoryChanged || frameResult.inventoryChanged;
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
	RuntimeNpcControlAggregate control;
	RuntimeNpcMovementAggregate movement;
	RuntimeNpcRefreshAggregate refresh;
	for (const RuntimeGameplayOrchestratedFrameRunnerFrame &frame : input.frames) {
		AppendFrame(result, control, movement, refresh, step.run(FrameInputFrom(result.state, frame)));
	}

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

	return result;
}

} // namespace iggy::runtime
