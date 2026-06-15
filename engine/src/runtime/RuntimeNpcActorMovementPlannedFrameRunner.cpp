#include "runtime/RuntimeNpcActorMovementPlannedFrameRunner.hpp"

namespace iggy::runtime {

bool RuntimeNpcActorMovementPlannedFrameRunnerResult::hasFrames() const
{
	return frameCount > 0;
}

bool RuntimeNpcActorMovementPlannedFrameRunnerResult::changed() const
{
	return changedFrameCount > 0;
}

RuntimeNpcActorMovementPlannedFrameRunnerResult RuntimeNpcActorMovementPlannedFrameRunner::run(
	const RuntimeNpcActorMovementPlannedFrameRunnerInput &input) const
{
	RuntimeNpcActorMovementPlannedFrameRunnerResult result;
	result.initialState = input.initialState;
	result.frames = input.frames;
	result.state = input.initialState;
	result.frameCount = input.frames.size();

	RuntimeNpcActorMovementPlannedFrameStep step;
	for (const RuntimeNpcActorMovementPlannedFrameRunnerFrame &frame : input.frames) {
		RuntimeNpcActorMovementPlannedFrameResult frameResult = step.run({
			result.state,
			frame.map,
			frame.config,
		});
		result.state = frameResult.state;
		result.plannedRequestCount += frameResult.plannedRequestCount;
		result.preReservationRequestCount += frameResult.preReservationRequestCount;
		result.reservationAcceptedCount += frameResult.reservationAcceptedCount;
		result.reservationRejectedCount += frameResult.reservationRejectedCount;
		result.movedCount += frameResult.movedCount;
		result.blockedMovementCount += frameResult.blockedCount;
		result.rejectedMovementCount += frameResult.rejectedCount;
		result.missingActorMovementCount += frameResult.missingActorCount;
		if (frameResult.changed) {
			++result.changedFrameCount;
		}
		result.needsOccupancyRebuild =
			result.needsOccupancyRebuild || frameResult.movement.report.needsOccupancyRebuild;
		result.needsAiMapQueryRefresh =
			result.needsAiMapQueryRefresh || frameResult.movement.report.needsAiMapQueryRefresh;
		result.needsInteractionRefresh =
			result.needsInteractionRefresh || frameResult.movement.report.needsInteractionRefresh;
		result.needsRenderRefresh =
			result.needsRenderRefresh || frameResult.movement.report.needsRenderRefresh;
		result.needsVisibilityRefresh =
			result.needsVisibilityRefresh || frameResult.movement.report.needsVisibilityRefresh;
		result.frameResults.push_back(frameResult);
	}

	return result;
}

} // namespace iggy::runtime
