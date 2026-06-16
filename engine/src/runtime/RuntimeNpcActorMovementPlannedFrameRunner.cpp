#include "runtime/RuntimeNpcActorMovementPlannedFrameRunner.hpp"

#include "runtime/RuntimeNpcOrchestrationAggregates.hpp"

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
	RuntimeNpcMovementAggregate movement;
	for (const RuntimeNpcActorMovementPlannedFrameRunnerFrame &frame : input.frames) {
		RuntimeNpcActorMovementPlannedFrameResult frameResult = step.run({
			result.state,
			frame.map,
			frame.config,
		});
		result.state = frameResult.state;
		foldRuntimeNpcMovementAggregate(movement, frameResult);
		if (frameResult.changed) {
			++result.changedFrameCount;
		}
		result.frameResults.push_back(frameResult);
	}

	result.plannedRequestCount = movement.plannedRequestCount;
	result.preReservationRequestCount = movement.preReservationRequestCount;
	result.reservationAcceptedCount = movement.reservationAcceptedCount;
	result.reservationRejectedCount = movement.reservationRejectedCount;
	result.movedCount = movement.movedCount;
	result.blockedMovementCount = movement.blockedMovementCount;
	result.rejectedMovementCount = movement.rejectedMovementCount;
	result.missingActorMovementCount = movement.missingActorMovementCount;
	result.needsOccupancyRebuild = movement.needsOccupancyRefresh;
	result.needsAiMapQueryRefresh = movement.needsAiMapRefresh;
	result.needsInteractionRefresh = movement.needsInteractionRefresh;
	result.needsRenderRefresh = movement.needsRenderRefresh;
	result.needsVisibilityRefresh = movement.needsVisibilityRefresh;

	return result;
}

} // namespace iggy::runtime
