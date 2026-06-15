#include "runtime/RuntimeGameplayFrameRunner.hpp"

#include <utility>

#include "runtime/RuntimeGameplayFrameReporter.hpp"

namespace iggy::runtime {
namespace {

RuntimeGameplayFrameInput FrameInputFrom(
	const RuntimeGameplayState &state,
	const RuntimeGameplayFrameRunnerFrame &frame)
{
	RuntimeGameplayFrameInput input;
	input.state = state;
	input.playerInputContext = frame.playerInputContext;
	input.actorId = frame.actorId;
	input.playerIntents = frame.playerIntents;
	input.commandQueueConfig = frame.commandQueueConfig;
	input.fallbackPlayerPosition = frame.fallbackPlayerPosition;
	input.playerCommandConfig = frame.playerCommandConfig;
	input.npcConfig = frame.npcConfig;
	input.interactionReach = frame.interactionReach;
	input.pickup = frame.pickup;
	input.npcMovementRequests = frame.npcMovementRequests;
	return input;
}

void AppendInventoryEvents(InventoryEventRecorder2D &events, const InventoryEventRecorder2D &frameEvents)
{
	for (const InventoryEvent2D &event : frameEvents.events) {
		recordInventoryEvent(events, event);
	}
}

void AppendTick(RuntimeGameplayFrameRunnerResult &result, RuntimeGameplayFrameResult frameResult)
{
	RuntimeGameplayFrameRunnerTick tick;
	tick.frame = frameResult;
	tick.report = RuntimeGameplayFrameReporter {}.report(frameResult);
	AppendInventoryEvents(result.inventoryEvents, frameResult.inventoryEvents);
	result.npcMovedCount += tick.report.npcMovedCount;
	result.npcBlockedMovementCount += tick.report.npcBlockedMovementCount;
	result.npcRejectedMovementCount += tick.report.npcRejectedMovementCount;
	result.npcMissingActorMovementCount += tick.report.npcMissingActorMovementCount;
	result.npcMovementDirtyTileCount += tick.report.npcMovementDirtyTileCount;
	result.npcMovementNeedsOccupancyRebuild =
		result.npcMovementNeedsOccupancyRebuild || tick.report.npcMovementNeedsOccupancyRebuild;
	result.npcMovementNeedsAiMapQueryRefresh =
		result.npcMovementNeedsAiMapQueryRefresh || tick.report.npcMovementNeedsAiMapQueryRefresh;
	result.npcMovementNeedsInteractionRefresh =
		result.npcMovementNeedsInteractionRefresh || tick.report.npcMovementNeedsInteractionRefresh;
	result.npcMovementNeedsRenderRefresh =
		result.npcMovementNeedsRenderRefresh || tick.report.npcMovementNeedsRenderRefresh;
	result.npcMovementNeedsVisibilityRefresh =
		result.npcMovementNeedsVisibilityRefresh || tick.report.npcMovementNeedsVisibilityRefresh;
	result.finalState = frameResult.state;
	result.ticks.push_back(std::move(tick));
}

} // namespace

RuntimeGameplayFrameRunnerResult RuntimeGameplayFrameRunner::run(const RuntimeGameplayFrameRunnerInput &input) const
{
	RuntimeGameplayFrameRunnerResult result;
	result.finalState = input.initialState;

	for (const RuntimeGameplayFrameRunnerFrame &frame : input.frames) {
		RuntimeGameplayFrameResult frameResult = RuntimeGameplayFrameStep {}.run(FrameInputFrom(result.finalState, frame));
		AppendTick(result, frameResult);
	}

	return result;
}

RuntimeGameplayFrameRunnerResult RuntimeGameplayFrameRunner::run(
	const RuntimeGameplayFrameRunnerInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	RuntimeGameplayFrameRunnerResult result;
	result.finalState = input.initialState;

	for (const RuntimeGameplayFrameRunnerFrame &frame : input.frames) {
		RuntimeGameplayFrameResult frameResult =
			RuntimeGameplayFrameStep {}.run(FrameInputFrom(result.finalState, frame), explicitWorld);
		AppendTick(result, frameResult);
	}

	return result;
}

} // namespace iggy::runtime
