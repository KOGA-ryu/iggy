#include "runtime/RuntimeNpcAiProfileMovementPlannedFrameStep.hpp"

namespace iggy::runtime {

bool RuntimeNpcAiProfileMovementPlannedFrameResult::ranAnyRequests() const
{
	return status == RuntimeNpcAiProfileMovementPlannedFrameStatus::Ran;
}

bool RuntimeNpcAiProfileMovementPlannedFrameResult::changedState() const
{
	return changedControls || changedActors;
}

RuntimeNpcAiProfileMovementPlannedFrameResult RuntimeNpcAiProfileMovementPlannedFrameStep::run(
	const RuntimeNpcAiProfileMovementPlannedFrameInput &input) const
{
	RuntimeNpcAiProfileMovementPlannedFrameResult result;
	result.input = input;
	result.control = RuntimeNpcAiProfileControlPlannedFrameStep {}.run({
		input.state,
		input.profileTraits,
		input.pools,
		input.aiMap,
		input.profileConfig,
		input.controlConfig,
	});
	result.movement = RuntimeNpcActorMovementPlannedFrameStep {}.run({
		result.control.state,
		input.movementMap,
		input.movementConfig,
	});

	result.state = result.movement.state;
	result.resolvedSubjectCount = result.control.resolvedSubjectCount;
	result.missingProfileCount = result.control.missingProfileCount;
	result.emptyActorProfileIdCount = result.control.emptyActorProfileIdCount;
	result.absentSkippedCount = result.control.absentSkippedCount;
	result.controlPlannedRequestCount = result.control.plannedRequestCount;
	result.controlAppliedCount = result.control.appliedControlCount;
	result.controlFailedCount = result.control.failedControlCount;
	result.controlPlanIssueCount = result.control.planIssueCount;
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
		? RuntimeNpcAiProfileMovementPlannedFrameStatus::Ran
		: RuntimeNpcAiProfileMovementPlannedFrameStatus::NoChanges;
	return result;
}

} // namespace iggy::runtime
