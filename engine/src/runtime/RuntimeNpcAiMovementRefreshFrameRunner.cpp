#include "runtime/RuntimeNpcAiMovementRefreshFrameRunner.hpp"

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
		result.controlPlannedRequestCount += frameResult.controlPlannedRequestCount;
		result.controlAppliedCount += frameResult.controlAppliedCount;
		result.controlFailedCount += frameResult.controlFailedCount;
		result.movementPlannedRequestCount += frameResult.movementPlannedRequestCount;
		result.movedCount += frameResult.movedCount;
		result.blockedMovementCount += frameResult.blockedMovementCount;
		result.rejectedMovementCount += frameResult.rejectedMovementCount;
		result.missingActorMovementCount += frameResult.missingActorMovementCount;
		result.dirtyTileCount += frameResult.dirtyTileCount;
		if (frameResult.changedControls) {
			++result.controlChangedFrameCount;
		}
		if (frameResult.changedActors) {
			++result.actorChangedFrameCount;
		}
		if (frameResult.changedState()) {
			++result.changedFrameCount;
		}
		if (frameResult.occupancyRefreshed) {
			++result.occupancyRefreshCount;
		}
		if (frameResult.interactionRefreshed) {
			++result.interactionRefreshCount;
		}
		if (frameResult.aiMapRefreshed) {
			++result.aiMapRefreshCount;
		}
		if (frameResult.renderRefreshed) {
			++result.renderRefreshCount;
		}
		if (frameResult.visibilityRefreshed) {
			++result.visibilityRefreshCount;
		}
		result.frameResults.push_back(frameResult);
	}

	return result;
}

} // namespace iggy::runtime
