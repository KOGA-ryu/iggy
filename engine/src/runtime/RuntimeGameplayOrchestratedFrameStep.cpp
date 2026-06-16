#include "runtime/RuntimeGameplayOrchestratedFrameStep.hpp"

namespace iggy::runtime {
namespace {

RuntimeGameplayFrameInput PlayerFrameInputWithoutPreparedNpcMovement(
	const RuntimeGameplayOrchestratedFrameInput &input)
{
	RuntimeGameplayFrameInput playerInput = input.playerFrame;
	playerInput.npcMovementRequests.clear();
	return playerInput;
}

RuntimeNpcAiMovementRefreshFrameInput NpcFrameInputFrom(
	const RuntimeGameplayOrchestratedFrameInput &input,
	const RuntimeGameplayState &state)
{
	return {
		{
			state,
			input.subjects,
			input.pools,
			input.aiMap,
			input.controlConfig,
			input.controlOverrides,
			input.movementMap,
			input.movementConfig,
		},
		input.previousOccupancy,
		input.interactionTargets,
		input.refreshAiMap,
		input.refreshConfig,
	};
}

void ProjectCounts(RuntimeGameplayOrchestratedFrameResult &result)
{
	result.inventoryEvents = result.playerFrame.inventoryEvents;
	result.acceptedCommandCount = result.playerFrame.report.acceptedCommandCount;
	result.blockedIntentCount = result.playerFrame.report.blockedIntentCount;
	result.rejectedIntentCount = result.playerFrame.report.rejectedIntentCount;
	result.pickedUpCount = result.playerFrame.report.pickedUpCount;
	result.inventoryEventCount = result.playerFrame.inventoryEvents.events.size();
	result.playerFrameRan = !result.playerFrame.report.events.empty();
	result.interactionChanged = result.playerFrame.report.interactionMutated;
	result.inventoryChanged = result.playerFrame.report.inventoryChanged;

	result.npcControlPlannedRequestCount = result.npcFrame.controlPlannedRequestCount;
	result.npcControlAppliedCount = result.npcFrame.controlAppliedCount;
	result.npcControlFailedCount = result.npcFrame.controlFailedCount;
	result.npcMovementPlannedRequestCount = result.npcFrame.movementPlannedRequestCount;
	result.npcMovedCount = result.npcFrame.movedCount;
	result.npcBlockedMovementCount = result.npcFrame.blockedMovementCount;
	result.npcRejectedMovementCount = result.npcFrame.rejectedMovementCount;
	result.npcMissingActorMovementCount = result.npcFrame.missingActorMovementCount;
	result.npcRefreshDirtyTileCount = result.npcFrame.dirtyTileCount;
	result.npcControlsChanged = result.npcFrame.changedControls;
	result.npcActorsChanged = result.npcFrame.changedActors;
	result.npcOccupancyRefreshed = result.npcFrame.occupancyRefreshed;
	result.npcInteractionRefreshed = result.npcFrame.interactionRefreshed;
	result.npcAiMapRefreshed = result.npcFrame.aiMapRefreshed;
	result.npcRenderRefreshed = result.npcFrame.renderRefreshed;
	result.npcVisibilityRefreshed = result.npcFrame.visibilityRefreshed;
}

RuntimeGameplayOrchestratedFrameResult ResultFrom(
	const RuntimeGameplayOrchestratedFrameInput &input,
	const RuntimeGameplayFrameResult &playerFrame)
{
	RuntimeGameplayOrchestratedFrameResult result;
	result.input = input;
	result.playerFrame = playerFrame;
	result.npcFrame = RuntimeNpcAiMovementRefreshFrameStep {}.run(
		NpcFrameInputFrom(input, playerFrame.state));
	result.state = result.npcFrame.state;
	ProjectCounts(result);
	result.status = (result.acceptedCommandCount > 0
						|| result.blockedIntentCount > 0
						|| result.rejectedIntentCount > 0
						|| result.pickedUpCount > 0
						|| result.inventoryEventCount > 0
						|| result.interactionChanged
						|| result.inventoryChanged
						|| result.changedNpcState()
						|| result.refreshedNpcData())
		? RuntimeGameplayOrchestratedFrameStatus::Ran
		: RuntimeGameplayOrchestratedFrameStatus::NoChanges;
	return result;
}

} // namespace

bool RuntimeGameplayOrchestratedFrameResult::changedNpcState() const
{
	return npcControlsChanged || npcActorsChanged;
}

bool RuntimeGameplayOrchestratedFrameResult::refreshedNpcData() const
{
	return npcOccupancyRefreshed
		|| npcInteractionRefreshed
		|| npcAiMapRefreshed
		|| npcRenderRefreshed
		|| npcVisibilityRefreshed;
}

bool RuntimeGameplayOrchestratedFrameResult::changedGameplayState() const
{
	return acceptedCommandCount > 0
		|| interactionChanged
		|| inventoryChanged
		|| changedNpcState();
}

RuntimeGameplayOrchestratedFrameResult RuntimeGameplayOrchestratedFrameStep::run(
	const RuntimeGameplayOrchestratedFrameInput &input) const
{
	return ResultFrom(
		input,
		RuntimeGameplayFrameStep {}.run(PlayerFrameInputWithoutPreparedNpcMovement(input)));
}

RuntimeGameplayOrchestratedFrameResult RuntimeGameplayOrchestratedFrameStep::run(
	const RuntimeGameplayOrchestratedFrameInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	return ResultFrom(
		input,
		RuntimeGameplayFrameStep {}.run(
			PlayerFrameInputWithoutPreparedNpcMovement(input),
			explicitWorld));
}

} // namespace iggy::runtime
