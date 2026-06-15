#include "runtime/RuntimeNpcAiMovementPlannedFrameRunner.hpp"

namespace iggy::runtime {

bool RuntimeNpcAiMovementPlannedFrameRunnerResult::hasFrames() const
{
	return frameCount > 0;
}

bool RuntimeNpcAiMovementPlannedFrameRunnerResult::changed() const
{
	return changedFrameCount > 0;
}

RuntimeNpcAiMovementPlannedFrameRunnerResult RuntimeNpcAiMovementPlannedFrameRunner::run(
	const RuntimeNpcAiMovementPlannedFrameRunnerInput &input) const
{
	RuntimeNpcAiMovementPlannedFrameRunnerResult result;
	result.initialState = input.initialState;
	result.frames = input.frames;
	result.state = input.initialState;
	result.frameCount = input.frames.size();

	RuntimeNpcAiMovementPlannedFrameStep step;
	for (const RuntimeNpcAiMovementPlannedFrameRunnerFrame &frame : input.frames) {
		RuntimeNpcAiMovementPlannedFrameResult frameResult = step.run({
			result.state,
			frame.subjects,
			frame.pools,
			frame.aiMap,
			frame.controlConfig,
			frame.movementMap,
			frame.movementConfig,
		});
		result.state = frameResult.state;
		result.controlPlannedRequestCount += frameResult.controlPlannedRequestCount;
		result.controlAppliedCount += frameResult.controlAppliedCount;
		result.controlFailedCount += frameResult.controlFailedCount;
		result.controlMapChangedSelectionCount += frameResult.controlMapChangedSelectionCount;
		result.movementPlannedRequestCount += frameResult.movementPlannedRequestCount;
		result.movementPreReservationRequestCount += frameResult.movementPreReservationRequestCount;
		result.movementReservationAcceptedCount += frameResult.movementReservationAcceptedCount;
		result.movementReservationRejectedCount += frameResult.movementReservationRejectedCount;
		result.movedCount += frameResult.movedCount;
		result.blockedMovementCount += frameResult.blockedCount;
		result.rejectedMovementCount += frameResult.rejectedCount;
		result.missingActorMovementCount += frameResult.missingActorCount;
		if (frameResult.changedControls) {
			++result.controlChangedFrameCount;
		}
		if (frameResult.changedActors) {
			++result.actorChangedFrameCount;
		}
		if (frameResult.changedState()) {
			++result.changedFrameCount;
		}
		result.needsOccupancyRebuild =
			result.needsOccupancyRebuild || frameResult.movement.movement.report.needsOccupancyRebuild;
		result.needsAiMapQueryRefresh =
			result.needsAiMapQueryRefresh || frameResult.movement.movement.report.needsAiMapQueryRefresh;
		result.needsInteractionRefresh =
			result.needsInteractionRefresh || frameResult.movement.movement.report.needsInteractionRefresh;
		result.needsRenderRefresh =
			result.needsRenderRefresh || frameResult.movement.movement.report.needsRenderRefresh;
		result.needsVisibilityRefresh =
			result.needsVisibilityRefresh || frameResult.movement.movement.report.needsVisibilityRefresh;
		result.frameResults.push_back(frameResult);
	}

	return result;
}

} // namespace iggy::runtime
