#include "runtime/RuntimeNpcOrchestrationAggregates.hpp"

namespace iggy::runtime {

void foldRuntimeNpcMovementAggregate(
	RuntimeNpcMovementAggregate &aggregate,
	const RuntimeNpcActorMovementPlannedFrameResult &result)
{
	aggregate.plannedRequestCount += result.plannedRequestCount;
	aggregate.preReservationRequestCount += result.preReservationRequestCount;
	aggregate.reservationAcceptedCount += result.reservationAcceptedCount;
	aggregate.reservationRejectedCount += result.reservationRejectedCount;
	aggregate.movedCount += result.movedCount;
	aggregate.blockedMovementCount += result.blockedCount;
	aggregate.rejectedMovementCount += result.rejectedCount;
	aggregate.missingActorMovementCount += result.missingActorCount;
	aggregate.dirtyTileCount += result.movement.report.dirtyTiles.size();
	aggregate.actorsChanged = aggregate.actorsChanged || result.changed;
	aggregate.needsOccupancyRefresh =
		aggregate.needsOccupancyRefresh || result.movement.report.needsOccupancyRebuild;
	aggregate.needsAiMapRefresh =
		aggregate.needsAiMapRefresh || result.movement.report.needsAiMapQueryRefresh;
	aggregate.needsInteractionRefresh =
		aggregate.needsInteractionRefresh || result.movement.report.needsInteractionRefresh;
	aggregate.needsRenderRefresh =
		aggregate.needsRenderRefresh || result.movement.report.needsRenderRefresh;
	aggregate.needsVisibilityRefresh =
		aggregate.needsVisibilityRefresh || result.movement.report.needsVisibilityRefresh;
}

void foldRuntimeNpcMovementAggregate(
	RuntimeNpcMovementAggregate &aggregate,
	const RuntimeNpcAiMovementPlannedFrameResult &result)
{
	aggregate.plannedRequestCount += result.movementPlannedRequestCount;
	aggregate.preReservationRequestCount += result.movementPreReservationRequestCount;
	aggregate.reservationAcceptedCount += result.movementReservationAcceptedCount;
	aggregate.reservationRejectedCount += result.movementReservationRejectedCount;
	aggregate.movedCount += result.movedCount;
	aggregate.blockedMovementCount += result.blockedCount;
	aggregate.rejectedMovementCount += result.rejectedCount;
	aggregate.missingActorMovementCount += result.missingActorCount;
	aggregate.dirtyTileCount += result.movement.movement.report.dirtyTiles.size();
	aggregate.actorsChanged = aggregate.actorsChanged || result.changedActors;
	aggregate.needsOccupancyRefresh =
		aggregate.needsOccupancyRefresh || result.movement.movement.report.needsOccupancyRebuild;
	aggregate.needsAiMapRefresh =
		aggregate.needsAiMapRefresh || result.movement.movement.report.needsAiMapQueryRefresh;
	aggregate.needsInteractionRefresh =
		aggregate.needsInteractionRefresh || result.movement.movement.report.needsInteractionRefresh;
	aggregate.needsRenderRefresh =
		aggregate.needsRenderRefresh || result.movement.movement.report.needsRenderRefresh;
	aggregate.needsVisibilityRefresh =
		aggregate.needsVisibilityRefresh || result.movement.movement.report.needsVisibilityRefresh;
}

void foldRuntimeNpcMovementAggregate(
	RuntimeNpcMovementAggregate &aggregate,
	const RuntimeNpcAiMovementRefreshFrameResult &result)
{
	aggregate.plannedRequestCount += result.movementPlannedRequestCount;
	aggregate.movedCount += result.movedCount;
	aggregate.blockedMovementCount += result.blockedMovementCount;
	aggregate.rejectedMovementCount += result.rejectedMovementCount;
	aggregate.missingActorMovementCount += result.missingActorMovementCount;
	aggregate.dirtyTileCount += result.dirtyTileCount;
	aggregate.actorsChanged = aggregate.actorsChanged || result.changedActors;
}

void foldRuntimeNpcMovementAggregate(
	RuntimeNpcMovementAggregate &aggregate,
	const RuntimeGameplayOrchestratedFrameResult &result)
{
	aggregate.plannedRequestCount += result.npcMovementPlannedRequestCount;
	aggregate.movedCount += result.npcMovedCount;
	aggregate.blockedMovementCount += result.npcBlockedMovementCount;
	aggregate.rejectedMovementCount += result.npcRejectedMovementCount;
	aggregate.missingActorMovementCount += result.npcMissingActorMovementCount;
	aggregate.dirtyTileCount += result.npcRefreshDirtyTileCount;
	aggregate.actorsChanged = aggregate.actorsChanged || result.npcActorsChanged;
	aggregate.needsOccupancyRefresh =
		aggregate.needsOccupancyRefresh || result.npcOccupancyRefreshed;
	aggregate.needsAiMapRefresh =
		aggregate.needsAiMapRefresh || result.npcAiMapRefreshed;
	aggregate.needsInteractionRefresh =
		aggregate.needsInteractionRefresh || result.npcInteractionRefreshed;
	aggregate.needsRenderRefresh =
		aggregate.needsRenderRefresh || result.npcRenderRefreshed;
	aggregate.needsVisibilityRefresh =
		aggregate.needsVisibilityRefresh || result.npcVisibilityRefreshed;
}

void foldRuntimeNpcMovementAggregate(
	RuntimeNpcMovementAggregate &aggregate,
	const RuntimeGameplayOrchestratedFrameReport &report)
{
	aggregate.plannedRequestCount += report.npcMovementPlannedRequestCount;
	aggregate.movedCount += report.npcMovedCount;
	aggregate.blockedMovementCount += report.npcBlockedMovementCount;
	aggregate.rejectedMovementCount += report.npcRejectedMovementCount;
	aggregate.missingActorMovementCount += report.npcMissingActorMovementCount;
	aggregate.dirtyTileCount += report.npcRefreshDirtyTileCount;
	aggregate.actorsChanged = aggregate.actorsChanged || report.npcActorsChanged;
	aggregate.needsOccupancyRefresh =
		aggregate.needsOccupancyRefresh || report.npcOccupancyRefreshed;
	aggregate.needsAiMapRefresh =
		aggregate.needsAiMapRefresh || report.npcAiMapRefreshed;
	aggregate.needsInteractionRefresh =
		aggregate.needsInteractionRefresh || report.npcInteractionRefreshed;
	aggregate.needsRenderRefresh =
		aggregate.needsRenderRefresh || report.npcRenderRefreshed;
	aggregate.needsVisibilityRefresh =
		aggregate.needsVisibilityRefresh || report.npcVisibilityRefreshed;
}

void foldRuntimeNpcMovementAggregate(
	RuntimeNpcMovementAggregate &aggregate,
	const RuntimeGameplayOrchestratedFrameRunnerReport &report)
{
	aggregate.plannedRequestCount += report.npcMovementPlannedRequestCount;
	aggregate.movedCount += report.npcMovedCount;
	aggregate.blockedMovementCount += report.npcBlockedMovementCount;
	aggregate.rejectedMovementCount += report.npcRejectedMovementCount;
	aggregate.missingActorMovementCount += report.npcMissingActorMovementCount;
	aggregate.dirtyTileCount += report.npcRefreshDirtyTileCount;
	aggregate.actorsChanged = aggregate.actorsChanged || report.npcActorsChanged;
	aggregate.needsOccupancyRefresh =
		aggregate.needsOccupancyRefresh || report.npcOccupancyRefreshed;
	aggregate.needsAiMapRefresh =
		aggregate.needsAiMapRefresh || report.npcAiMapRefreshed;
	aggregate.needsInteractionRefresh =
		aggregate.needsInteractionRefresh || report.npcInteractionRefreshed;
	aggregate.needsRenderRefresh =
		aggregate.needsRenderRefresh || report.npcRenderRefreshed;
	aggregate.needsVisibilityRefresh =
		aggregate.needsVisibilityRefresh || report.npcVisibilityRefreshed;
}

void foldRuntimeNpcControlAggregate(
	RuntimeNpcControlAggregate &aggregate,
	const RuntimeNpcAiControlPlannedFrameResult &result)
{
	aggregate.plannedRequestCount += result.plannedRequestCount;
	aggregate.appliedCount += result.appliedControlCount;
	aggregate.failedCount += result.failedControlCount;
	aggregate.planIssueCount += result.planIssueCount;
	aggregate.mapChangedSelectionCount += result.mapChangedSelectionCount;
	aggregate.controlsChanged = aggregate.controlsChanged || result.changedControls;
}

void foldRuntimeNpcControlAggregate(
	RuntimeNpcControlAggregate &aggregate,
	const RuntimeNpcAiMovementPlannedFrameResult &result)
{
	aggregate.plannedRequestCount += result.controlPlannedRequestCount;
	aggregate.appliedCount += result.controlAppliedCount;
	aggregate.failedCount += result.controlFailedCount;
	aggregate.mapChangedSelectionCount += result.controlMapChangedSelectionCount;
	aggregate.controlsChanged = aggregate.controlsChanged || result.changedControls;
}

void foldRuntimeNpcControlAggregate(
	RuntimeNpcControlAggregate &aggregate,
	const RuntimeNpcAiMovementRefreshFrameResult &result)
{
	aggregate.plannedRequestCount += result.controlPlannedRequestCount;
	aggregate.appliedCount += result.controlAppliedCount;
	aggregate.failedCount += result.controlFailedCount;
	aggregate.controlsChanged = aggregate.controlsChanged || result.changedControls;
}

void foldRuntimeNpcControlAggregate(
	RuntimeNpcControlAggregate &aggregate,
	const RuntimeGameplayOrchestratedFrameResult &result)
{
	aggregate.plannedRequestCount += result.npcControlPlannedRequestCount;
	aggregate.appliedCount += result.npcControlAppliedCount;
	aggregate.failedCount += result.npcControlFailedCount;
	aggregate.controlsChanged = aggregate.controlsChanged || result.npcControlsChanged;
}

void foldRuntimeNpcControlAggregate(
	RuntimeNpcControlAggregate &aggregate,
	const RuntimeGameplayOrchestratedFrameReport &report)
{
	aggregate.plannedRequestCount += report.npcControlPlannedRequestCount;
	aggregate.appliedCount += report.npcControlAppliedCount;
	aggregate.failedCount += report.npcControlFailedCount;
	aggregate.controlsChanged = aggregate.controlsChanged || report.npcControlsChanged;
}

void foldRuntimeNpcControlAggregate(
	RuntimeNpcControlAggregate &aggregate,
	const RuntimeGameplayOrchestratedFrameRunnerReport &report)
{
	aggregate.plannedRequestCount += report.npcControlPlannedRequestCount;
	aggregate.appliedCount += report.npcControlAppliedCount;
	aggregate.failedCount += report.npcControlFailedCount;
	aggregate.controlsChanged = aggregate.controlsChanged || report.npcControlsChanged;
}

void foldRuntimeNpcRefreshAggregate(
	RuntimeNpcRefreshAggregate &aggregate,
	const RuntimeNpcAiMovementRefreshFrameResult &result)
{
	aggregate.dirtyTileCount += result.dirtyTileCount;
	if (result.occupancyRefreshed) {
		++aggregate.occupancyRefreshCount;
	}
	if (result.interactionRefreshed) {
		++aggregate.interactionRefreshCount;
	}
	if (result.aiMapRefreshed) {
		++aggregate.aiMapRefreshCount;
	}
	if (result.renderRefreshed) {
		++aggregate.renderRefreshCount;
	}
	if (result.visibilityRefreshed) {
		++aggregate.visibilityRefreshCount;
	}
	aggregate.occupancyRefreshed = aggregate.occupancyRefreshed || result.occupancyRefreshed;
	aggregate.interactionRefreshed = aggregate.interactionRefreshed || result.interactionRefreshed;
	aggregate.aiMapRefreshed = aggregate.aiMapRefreshed || result.aiMapRefreshed;
	aggregate.renderRefreshed = aggregate.renderRefreshed || result.renderRefreshed;
	aggregate.visibilityRefreshed = aggregate.visibilityRefreshed || result.visibilityRefreshed;
}

void foldRuntimeNpcRefreshAggregate(
	RuntimeNpcRefreshAggregate &aggregate,
	const RuntimeGameplayOrchestratedFrameResult &result)
{
	aggregate.dirtyTileCount += result.npcRefreshDirtyTileCount;
	if (result.npcOccupancyRefreshed) {
		++aggregate.occupancyRefreshCount;
	}
	if (result.npcInteractionRefreshed) {
		++aggregate.interactionRefreshCount;
	}
	if (result.npcAiMapRefreshed) {
		++aggregate.aiMapRefreshCount;
	}
	if (result.npcRenderRefreshed) {
		++aggregate.renderRefreshCount;
	}
	if (result.npcVisibilityRefreshed) {
		++aggregate.visibilityRefreshCount;
	}
	aggregate.occupancyRefreshed = aggregate.occupancyRefreshed || result.npcOccupancyRefreshed;
	aggregate.interactionRefreshed = aggregate.interactionRefreshed || result.npcInteractionRefreshed;
	aggregate.aiMapRefreshed = aggregate.aiMapRefreshed || result.npcAiMapRefreshed;
	aggregate.renderRefreshed = aggregate.renderRefreshed || result.npcRenderRefreshed;
	aggregate.visibilityRefreshed = aggregate.visibilityRefreshed || result.npcVisibilityRefreshed;
}

void foldRuntimeNpcRefreshAggregate(
	RuntimeNpcRefreshAggregate &aggregate,
	const RuntimeGameplayOrchestratedFrameReport &report)
{
	aggregate.dirtyTileCount += report.npcRefreshDirtyTileCount;
	if (report.npcOccupancyRefreshed) {
		++aggregate.occupancyRefreshCount;
	}
	if (report.npcInteractionRefreshed) {
		++aggregate.interactionRefreshCount;
	}
	if (report.npcAiMapRefreshed) {
		++aggregate.aiMapRefreshCount;
	}
	if (report.npcRenderRefreshed) {
		++aggregate.renderRefreshCount;
	}
	if (report.npcVisibilityRefreshed) {
		++aggregate.visibilityRefreshCount;
	}
	aggregate.occupancyRefreshed = aggregate.occupancyRefreshed || report.npcOccupancyRefreshed;
	aggregate.interactionRefreshed = aggregate.interactionRefreshed || report.npcInteractionRefreshed;
	aggregate.aiMapRefreshed = aggregate.aiMapRefreshed || report.npcAiMapRefreshed;
	aggregate.renderRefreshed = aggregate.renderRefreshed || report.npcRenderRefreshed;
	aggregate.visibilityRefreshed = aggregate.visibilityRefreshed || report.npcVisibilityRefreshed;
}

void foldRuntimeNpcRefreshAggregate(
	RuntimeNpcRefreshAggregate &aggregate,
	const RuntimeGameplayOrchestratedFrameRunnerReport &report)
{
	aggregate.dirtyTileCount += report.npcRefreshDirtyTileCount;
	if (report.npcOccupancyRefreshed) {
		++aggregate.occupancyRefreshCount;
	}
	if (report.npcInteractionRefreshed) {
		++aggregate.interactionRefreshCount;
	}
	if (report.npcAiMapRefreshed) {
		++aggregate.aiMapRefreshCount;
	}
	if (report.npcRenderRefreshed) {
		++aggregate.renderRefreshCount;
	}
	if (report.npcVisibilityRefreshed) {
		++aggregate.visibilityRefreshCount;
	}
	aggregate.occupancyRefreshed = aggregate.occupancyRefreshed || report.npcOccupancyRefreshed;
	aggregate.interactionRefreshed = aggregate.interactionRefreshed || report.npcInteractionRefreshed;
	aggregate.aiMapRefreshed = aggregate.aiMapRefreshed || report.npcAiMapRefreshed;
	aggregate.renderRefreshed = aggregate.renderRefreshed || report.npcRenderRefreshed;
	aggregate.visibilityRefreshed = aggregate.visibilityRefreshed || report.npcVisibilityRefreshed;
}

} // namespace iggy::runtime
