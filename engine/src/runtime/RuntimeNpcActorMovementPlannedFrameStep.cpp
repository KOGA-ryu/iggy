#include "runtime/RuntimeNpcActorMovementPlannedFrameStep.hpp"

namespace iggy::runtime {

bool RuntimeNpcActorMovementPlannedFrameResult::ranMovementRequests() const
{
	return status == RuntimeNpcActorMovementPlannedFrameStatus::Ran;
}

bool RuntimeNpcActorMovementPlannedFrameResult::changedState() const
{
	return changed;
}

RuntimeNpcActorMovementPlannedFrameResult RuntimeNpcActorMovementPlannedFrameStep::run(
	const RuntimeNpcActorMovementPlannedFrameInput &input) const
{
	RuntimeNpcActorMovementPlannedFrameResult result;
	result.inputState = input.state;
	result.map = input.map;
	result.config = input.config;
	result.plan = RuntimeNpcActorMovementRequestPlanStep {}.plan({
		input.state,
		input.map,
		input.config,
	});
	result.movement = RuntimeNpcActorMovementFrameStep {}.run({
		input.state,
		result.plan.requests,
	});
	result.state = result.movement.state;
	result.plannedRequestCount = result.plan.requestCount;
	result.preReservationRequestCount = result.plan.preReservationRequestCount;
	result.reservationAcceptedCount = result.plan.reservationAcceptedCount;
	result.reservationRejectedCount = result.plan.reservationRejectedCount;
	result.movedCount = result.movement.apply.movedCount;
	result.blockedCount = result.movement.apply.blockedCount;
	result.rejectedCount = result.movement.apply.rejectedCount;
	result.missingActorCount = result.movement.apply.missingActorCount;
	result.changed = result.movement.changed;
	result.status = result.plannedRequestCount > 0
		? RuntimeNpcActorMovementPlannedFrameStatus::Ran
		: RuntimeNpcActorMovementPlannedFrameStatus::NoRequests;
	return result;
}

} // namespace iggy::runtime
