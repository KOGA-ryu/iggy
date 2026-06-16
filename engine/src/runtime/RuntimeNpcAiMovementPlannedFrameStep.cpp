#include "runtime/RuntimeNpcAiMovementPlannedFrameStep.hpp"

namespace iggy::runtime {
namespace {

RuntimeGameplayState ApplyControlOverrides(
	RuntimeGameplayState state,
	const std::vector<NpcActorControlState2D> &overrides)
{
	for (const NpcActorControlState2D &overrideControl : overrides) {
		bool replaced = false;
		for (NpcActorControlState2D &control : state.npcControls.entries) {
			if (control.npcId == overrideControl.npcId) {
				control = overrideControl;
				replaced = true;
				break;
			}
		}
		if (!replaced)
			state.npcControls.entries.push_back(overrideControl);
	}
	return state;
}

} // namespace

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
	result.controlOverrides = input.controlOverrides;
	result.movementMap = input.movementMap;
	result.movementConfig = input.movementConfig;

	result.control = RuntimeNpcAiControlPlannedFrameStep {}.run({
		input.state,
		input.subjects,
		input.pools,
		input.aiMap,
		input.controlConfig,
	});

	const RuntimeGameplayState movementInputState =
		ApplyControlOverrides(result.control.state, input.controlOverrides);

	result.movement = RuntimeNpcActorMovementPlannedFrameStep {}.run({
		movementInputState,
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
	result.changedControls = result.control.changedControls || !input.controlOverrides.empty();
	result.changedActors = result.movement.changed;
	result.status = (result.controlPlannedRequestCount > 0 || result.movementPlannedRequestCount > 0)
		? RuntimeNpcAiMovementPlannedFrameStatus::Ran
		: RuntimeNpcAiMovementPlannedFrameStatus::NoChanges;
	return result;
}

} // namespace iggy::runtime
