#include "runtime/RuntimeGameplayOrchestratedFrameRunnerReport.hpp"

#include "runtime/RuntimeNpcOrchestrationAggregates.hpp"

namespace iggy::runtime {
namespace {

void AppendInventoryEvents(
	InventoryEventRecorder2D &events,
	const InventoryEventRecorder2D &frameEvents)
{
	for (const InventoryEvent2D &event : frameEvents.events) {
		recordInventoryEvent(events, event);
	}
}

void AccumulateFrame(
	RuntimeGameplayOrchestratedFrameRunnerReport &report,
	RuntimeNpcControlAggregate &control,
	RuntimeNpcMovementAggregate &movement,
	RuntimeNpcRefreshAggregate &refresh,
	const RuntimeGameplayOrchestratedFrameReport &frame)
{
	AppendInventoryEvents(report.inventoryEvents, frame.inventoryEvents);
	report.acceptedCommandCount += frame.acceptedCommandCount;
	report.blockedIntentCount += frame.blockedIntentCount;
	report.rejectedIntentCount += frame.rejectedIntentCount;
	report.pickedUpCount += frame.pickedUpCount;
	report.inventoryEventCount += frame.inventoryEventCount;
	foldRuntimeNpcControlAggregate(control, frame);
	foldRuntimeNpcMovementAggregate(movement, frame);
	foldRuntimeNpcRefreshAggregate(refresh, frame);
	report.interactionChanged = report.interactionChanged || frame.interactionChanged;
	report.inventoryChanged = report.inventoryChanged || frame.inventoryChanged;
	if (frame.changed() || frame.refreshedNpcData()) {
		++report.changedFrameCount;
	}
}

void AppendEvents(RuntimeGameplayOrchestratedFrameRunnerReport &report)
{
	if (report.changedFrameCount > 0) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameRunnerEvent::FrameChangedObserved);
	}
	if (report.inventoryEventCount > 0) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameRunnerEvent::InventoryEventObserved);
	}
	if (report.npcControlsChanged) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameRunnerEvent::NpcControlChanged);
	}
	if (report.npcMovedCount > 0) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameRunnerEvent::NpcActorMoved);
	}
	if (report.npcBlockedMovementCount > 0) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameRunnerEvent::NpcActorBlocked);
	}
	if (report.npcRejectedMovementCount > 0) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameRunnerEvent::NpcActorRejected);
	}
	if (report.npcMissingActorMovementCount > 0) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameRunnerEvent::NpcActorMissing);
	}
	if (report.npcOccupancyRefreshed) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameRunnerEvent::RefreshOccupancy);
	}
	if (report.npcInteractionRefreshed) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameRunnerEvent::RefreshInteraction);
	}
	if (report.npcAiMapRefreshed) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameRunnerEvent::RefreshAiMap);
	}
	if (report.npcRenderRefreshed) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameRunnerEvent::RefreshRender);
	}
	if (report.npcVisibilityRefreshed) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameRunnerEvent::RefreshVisibility);
	}
	report.events.push_back(
		report.changed() || report.refreshedNpcData()
			? RuntimeGameplayOrchestratedFrameRunnerEvent::RunnerChanged
			: RuntimeGameplayOrchestratedFrameRunnerEvent::RunnerUnchanged);
}

} // namespace

bool RuntimeGameplayOrchestratedFrameRunnerReport::changed() const
{
	return changedFrameCount > 0;
}

bool RuntimeGameplayOrchestratedFrameRunnerReport::refreshedNpcData() const
{
	return npcOccupancyRefreshed
		|| npcInteractionRefreshed
		|| npcAiMapRefreshed
		|| npcRenderRefreshed
		|| npcVisibilityRefreshed;
}

bool RuntimeGameplayOrchestratedFrameRunnerReport::hasEvents() const
{
	return !events.empty();
}

RuntimeGameplayOrchestratedFrameRunnerReport RuntimeGameplayOrchestratedFrameRunnerReporter::report(
	const RuntimeGameplayOrchestratedFrameRunnerResult &result) const
{
	RuntimeGameplayOrchestratedFrameRunnerReport report;
	report.result = result;
	report.frameCount = result.frameCount;

	RuntimeGameplayOrchestratedFrameReporter frameReporter;
	RuntimeNpcControlAggregate control;
	RuntimeNpcMovementAggregate movement;
	RuntimeNpcRefreshAggregate refresh;
	for (const RuntimeGameplayOrchestratedFrameResult &frameResult : result.frameResults) {
		RuntimeGameplayOrchestratedFrameReport frame = frameReporter.report(frameResult);
		AccumulateFrame(report, control, movement, refresh, frame);
		report.frames.push_back(frame);
	}

	report.npcControlPlannedRequestCount = control.plannedRequestCount;
	report.npcControlAppliedCount = control.appliedCount;
	report.npcControlFailedCount = control.failedCount;
	report.npcMovementPlannedRequestCount = movement.plannedRequestCount;
	report.npcMovedCount = movement.movedCount;
	report.npcBlockedMovementCount = movement.blockedMovementCount;
	report.npcRejectedMovementCount = movement.rejectedMovementCount;
	report.npcMissingActorMovementCount = movement.missingActorMovementCount;
	report.npcRefreshDirtyTileCount = refresh.dirtyTileCount;
	report.npcControlsChanged = control.controlsChanged;
	report.npcActorsChanged = movement.actorsChanged;
	report.npcOccupancyRefreshed = refresh.occupancyRefreshed;
	report.npcInteractionRefreshed = refresh.interactionRefreshed;
	report.npcAiMapRefreshed = refresh.aiMapRefreshed;
	report.npcRenderRefreshed = refresh.renderRefreshed;
	report.npcVisibilityRefreshed = refresh.visibilityRefreshed;

	AppendEvents(report);
	return report;
}

} // namespace iggy::runtime
