#include "runtime/RuntimeNpcAiMovementPlannedFrameRunner.hpp"

#include "runtime/RuntimeNpcOrchestrationAggregates.hpp"

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
	RuntimeNpcControlAggregate control;
	RuntimeNpcMovementAggregate movement;
	for (const RuntimeNpcAiMovementPlannedFrameRunnerFrame &frame : input.frames) {
		RuntimeNpcAiMovementPlannedFrameResult frameResult = step.run({
			result.state,
			frame.subjects,
			frame.pools,
			frame.aiMap,
			frame.controlConfig,
			{},
			frame.movementMap,
			frame.movementConfig,
		});
		result.state = frameResult.state;
		foldRuntimeNpcControlAggregate(control, frameResult);
		foldRuntimeNpcMovementAggregate(movement, frameResult);
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
	result.controlMapChangedSelectionCount = control.mapChangedSelectionCount;
	result.movementPlannedRequestCount = movement.plannedRequestCount;
	result.movementPreReservationRequestCount = movement.preReservationRequestCount;
	result.movementReservationAcceptedCount = movement.reservationAcceptedCount;
	result.movementReservationRejectedCount = movement.reservationRejectedCount;
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
