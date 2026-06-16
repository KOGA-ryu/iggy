#include "runtime/RuntimeNpcAiMovementRefreshFrameRunner.hpp"

#include "runtime/RuntimeNpcOrchestrationAggregates.hpp"

#include "scene/npc/NpcActorOccupancy2D.hpp"

namespace iggy::runtime {

bool RuntimeNpcAiMovementRefreshFrameRunnerResult::hasFrames() const
{
	return frameCount > 0;
}

bool RuntimeNpcAiMovementRefreshFrameRunnerResult::changed() const
{
	return changedFrameCount > 0;
}

bool RuntimeNpcAiMovementRefreshFrameRunnerResult::refreshedAny() const
{
	return occupancyRefreshCount > 0
		|| interactionRefreshCount > 0
		|| aiMapRefreshCount > 0
		|| renderRefreshCount > 0
		|| visibilityRefreshCount > 0;
}

RuntimeNpcAiMovementRefreshFrameRunnerResult RuntimeNpcAiMovementRefreshFrameRunner::run(
	const RuntimeNpcAiMovementRefreshFrameRunnerInput &input) const
{
	RuntimeNpcAiMovementRefreshFrameRunnerResult result;
	result.initialState = input.initialState;
	result.frames = input.frames;
	result.state = input.initialState;
	result.frameCount = input.frames.size();

	RuntimeNpcAiMovementRefreshFrameStep step;
	RuntimeNpcControlAggregate control;
	RuntimeNpcMovementAggregate movement;
	RuntimeNpcRefreshAggregate refresh;
	for (const RuntimeNpcAiMovementRefreshFrameRunnerFrame &frame : input.frames) {
		const NpcActorOccupancy2D previousOccupancy =
			NpcActorOccupancyProjector2D {}.project(result.state.npcActors);
		RuntimeNpcAiMovementRefreshFrameResult frameResult = step.run({
			{
				result.state,
				frame.subjects,
				frame.pools,
				frame.aiMap,
				frame.controlConfig,
				frame.movementMap,
				frame.movementConfig,
			},
			previousOccupancy,
			frame.interactionTargets,
			frame.refreshAiMap,
			frame.refreshConfig,
		});
		result.state = frameResult.state;
		foldRuntimeNpcControlAggregate(control, frameResult);
		foldRuntimeNpcMovementAggregate(movement, frameResult);
		foldRuntimeNpcRefreshAggregate(refresh, frameResult);
		if (frameResult.changedControls) {
			++result.controlChangedFrameCount;
		}
		if (frameResult.changedActors) {
			++result.actorChangedFrameCount;
		}
		if (frameResult.changedState()) {
			++result.changedFrameCount;
		}
		result.frameResults.push_back(frameResult);
	}

	result.controlPlannedRequestCount = control.plannedRequestCount;
	result.controlAppliedCount = control.appliedCount;
	result.controlFailedCount = control.failedCount;
	result.movementPlannedRequestCount = movement.plannedRequestCount;
	result.movedCount = movement.movedCount;
	result.blockedMovementCount = movement.blockedMovementCount;
	result.rejectedMovementCount = movement.rejectedMovementCount;
	result.missingActorMovementCount = movement.missingActorMovementCount;
	result.dirtyTileCount = refresh.dirtyTileCount;
	result.occupancyRefreshCount = refresh.occupancyRefreshCount;
	result.interactionRefreshCount = refresh.interactionRefreshCount;
	result.aiMapRefreshCount = refresh.aiMapRefreshCount;
	result.renderRefreshCount = refresh.renderRefreshCount;
	result.visibilityRefreshCount = refresh.visibilityRefreshCount;

	return result;
}

} // namespace iggy::runtime
