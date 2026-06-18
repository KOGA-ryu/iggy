#include "runtime/RuntimeGameplayScenarioReportProjection.hpp"

#include "runtime/RuntimeNpcOrchestrationAggregates.hpp"

namespace iggy::runtime {
namespace {

template <typename Report>
RuntimeGameplayScenarioNpcReportProjection ProjectNpcFacts(const Report &report)
{
	RuntimeNpcControlAggregate control;
	RuntimeNpcMovementAggregate movement;
	RuntimeNpcRefreshAggregate refresh;
	foldRuntimeNpcControlAggregate(control, report);
	foldRuntimeNpcMovementAggregate(movement, report);
	foldRuntimeNpcRefreshAggregate(refresh, report);

	RuntimeGameplayScenarioNpcReportProjection projection;
	projection.npcControlPlannedRequestCount = control.plannedRequestCount;
	projection.npcControlAppliedCount = control.appliedCount;
	projection.npcControlFailedCount = control.failedCount;
	projection.npcControlsChanged = control.controlsChanged;
	projection.npcMovementPlannedRequestCount = movement.plannedRequestCount;
	projection.npcMovedCount = movement.movedCount;
	projection.npcBlockedMovementCount = movement.blockedMovementCount;
	projection.npcRejectedMovementCount = movement.rejectedMovementCount;
	projection.npcMissingActorMovementCount = movement.missingActorMovementCount;
	projection.npcActorsChanged = movement.actorsChanged;
	projection.npcRefreshDirtyTileCount = refresh.dirtyTileCount;
	projection.npcOccupancyRefreshed = refresh.occupancyRefreshed;
	projection.npcInteractionRefreshed = refresh.interactionRefreshed;
	projection.npcAiMapRefreshed = refresh.aiMapRefreshed;
	projection.npcRenderRefreshed = refresh.renderRefreshed;
	projection.npcVisibilityRefreshed = refresh.visibilityRefreshed;
	return projection;
}

template <typename Target>
void ApplyNpcFacts(
	Target &target,
	const RuntimeGameplayScenarioNpcReportProjection &projection)
{
	target.npcControlPlannedRequestCount = projection.npcControlPlannedRequestCount;
	target.npcControlAppliedCount = projection.npcControlAppliedCount;
	target.npcControlFailedCount = projection.npcControlFailedCount;
	target.npcControlsChanged = projection.npcControlsChanged;
	target.npcMovementPlannedRequestCount = projection.npcMovementPlannedRequestCount;
	target.npcMovedCount = projection.npcMovedCount;
	target.npcBlockedMovementCount = projection.npcBlockedMovementCount;
	target.npcRejectedMovementCount = projection.npcRejectedMovementCount;
	target.npcMissingActorMovementCount = projection.npcMissingActorMovementCount;
	target.npcActorsChanged = projection.npcActorsChanged;
	target.npcRefreshDirtyTileCount = projection.npcRefreshDirtyTileCount;
	target.npcOccupancyRefreshed = projection.npcOccupancyRefreshed;
	target.npcInteractionRefreshed = projection.npcInteractionRefreshed;
	target.npcAiMapRefreshed = projection.npcAiMapRefreshed;
	target.npcRenderRefreshed = projection.npcRenderRefreshed;
	target.npcVisibilityRefreshed = projection.npcVisibilityRefreshed;
}

} // namespace

RuntimeGameplayScenarioNpcReportProjection projectRuntimeGameplayScenarioNpcReport(
	const RuntimeGameplayOrchestratedFrameReport &report)
{
	return ProjectNpcFacts(report);
}

RuntimeGameplayScenarioNpcReportProjection projectRuntimeGameplayScenarioNpcReport(
	const RuntimeGameplayOrchestratedFrameRunnerReport &report)
{
	return ProjectNpcFacts(report);
}

void applyRuntimeGameplayScenarioResultProjection(
	RuntimeGameplayScenarioResult &result,
	const RuntimeGameplayOrchestratedFrameRunnerReport &report)
{
	result.frameCount = report.frameCount;
	result.changedFrameCount = report.changedFrameCount;
	result.inventoryEventCount = report.inventoryEventCount;
	ApplyNpcFacts(result, projectRuntimeGameplayScenarioNpcReport(report));
}

RuntimeGameplayScenarioLedgerFrame projectRuntimeGameplayScenarioLedgerFrame(
	std::size_t frameIndex,
	const RuntimeGameplayOrchestratedFrameReport &report)
{
	RuntimeGameplayScenarioLedgerFrame frame;
	frame.frameIndex = frameIndex;
	frame.changed = report.changed() || report.refreshedNpcData();
	frame.inventoryEventCount = report.inventoryEventCount;
	ApplyNpcFacts(frame, projectRuntimeGameplayScenarioNpcReport(report));
	return frame;
}

void applyRuntimeGameplayScenarioLedgerSummaryProjection(
	RuntimeGameplayScenarioLedger &ledger,
	const RuntimeGameplayScenarioResult &result)
{
	ledger.frameCount = result.frameCount;
	ledger.changedFrameCount = result.changedFrameCount;
	ledger.inventoryEventCount = result.inventoryEventCount;
	ApplyNpcFacts(ledger, projectRuntimeGameplayScenarioNpcReport(result.report));
}

} // namespace iggy::runtime
