#include "runtime/RuntimeNpcAiMovementRefreshFrameStep.hpp"

namespace iggy::runtime {

bool RuntimeNpcAiMovementRefreshFrameResult::changedState() const
{
	return changedControls || changedActors;
}

bool RuntimeNpcAiMovementRefreshFrameResult::refreshedAny() const
{
	return occupancyRefreshed
		|| interactionRefreshed
		|| aiMapRefreshed
		|| renderRefreshed
		|| visibilityRefreshed;
}

RuntimeNpcAiMovementRefreshFrameResult RuntimeNpcAiMovementRefreshFrameStep::run(
	const RuntimeNpcAiMovementRefreshFrameInput &input) const
{
	RuntimeNpcAiMovementRefreshFrameResult result;
	result.input = input;
	result.planned = RuntimeNpcAiMovementPlannedFrameStep {}.run(input.planned);
	result.refresh = NpcActorMovementRefreshFrameProjector2D {}.project({
		result.planned.movement.movement.report,
		result.planned.state.npcActors,
		input.previousOccupancy,
		input.interactionTargets,
		input.refreshAiMap,
		input.refreshConfig,
	});
	result.state = result.planned.state;
	result.controlPlannedRequestCount = result.planned.controlPlannedRequestCount;
	result.controlAppliedCount = result.planned.controlAppliedCount;
	result.controlFailedCount = result.planned.controlFailedCount;
	result.movementPlannedRequestCount = result.planned.movementPlannedRequestCount;
	result.movedCount = result.planned.movedCount;
	result.blockedMovementCount = result.planned.blockedCount;
	result.rejectedMovementCount = result.planned.rejectedCount;
	result.missingActorMovementCount = result.planned.missingActorCount;
	result.dirtyTileCount = result.refresh.dirtyTileCount;
	result.changedControls = result.planned.changedControls;
	result.changedActors = result.planned.changedActors;
	result.occupancyRefreshed = result.refresh.occupancyRefreshed;
	result.interactionRefreshed = result.refresh.interactionRefreshed;
	result.aiMapRefreshed = result.refresh.aiMapRefreshed;
	result.renderRefreshed = result.refresh.renderRefreshed;
	result.visibilityRefreshed = result.refresh.visibilityRefreshed;
	result.status = (result.changedState() || result.refreshedAny())
		? RuntimeNpcAiMovementRefreshFrameStatus::Ran
		: RuntimeNpcAiMovementRefreshFrameStatus::NoChanges;
	return result;
}

} // namespace iggy::runtime
