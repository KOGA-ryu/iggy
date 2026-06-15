#include "runtime/RuntimeNpcAiMovementPlannedFrameStep.hpp"

namespace iggy::runtime {

bool RuntimeNpcAiMovementPlannedFrameResult::ranAnyRequests() const
{
	return status == RuntimeNpcAiMovementPlannedFrameStatus::Ran;
}

bool RuntimeNpcAiMovementPlannedFrameResult::changedState() const
{
	return changedControls || changedActors;
}

RuntimeNpcAiMovementPlannedFrameResult RuntimeNpcAiMovementPlannedFrameStep::run(
	const RuntimeNpcAiMovementPlannedFrameInput &input) const
{
	RuntimeNpcAiMovementPlannedFrameResult result;
	result.inputState = input.state;
	result.subjects = input.subjects;
	result.pools = input.pools;
	result.aiMap = input.aiMap;
	result.controlConfig = input.controlConfig;
	result.movementMap = input.movementMap;
	result.movementConfig = input.movementConfig;

	result.control = RuntimeNpcAiControlPlannedFrameStep {}.run({
		input.state,
		input.subjects,
		input.pools,
		input.aiMap,
		input.controlConfig,
	});

	result.movement = RuntimeNpcActorMovementPlannedFrameStep {}.run({
		result.control.state,
		input.movementMap,
		input.movementConfig,
	});

	result.state = result.movement.state;
	result.controlPlannedRequestCount = result.control.plannedRequestCount;
	result.controlAppliedCount = result.control.appliedControlCount;
	result.controlFailedCount = result.control.failedControlCount;
	result.controlMapChangedSelectionCount = result.control.mapChangedSelectionCount;
	result.movementPlannedRequestCount = result.movement.plannedRequestCount;
	result.movementPreReservationRequestCount = result.movement.preReservationRequestCount;
	result.movementReservationAcceptedCount = result.movement.reservationAcceptedCount;
	result.movementReservationRejectedCount = result.movement.reservationRejectedCount;
	result.movedCount = result.movement.movedCount;
	result.blockedCount = result.movement.blockedCount;
	result.rejectedCount = result.movement.rejectedCount;
	result.missingActorCount = result.movement.missingActorCount;
	result.changedControls = result.control.changedControls;
	result.changedActors = result.movement.changed;
	result.status = (result.controlPlannedRequestCount > 0 || result.movementPlannedRequestCount > 0)
		? RuntimeNpcAiMovementPlannedFrameStatus::Ran
		: RuntimeNpcAiMovementPlannedFrameStatus::NoChanges;
	return result;
}

} // namespace iggy::runtime
