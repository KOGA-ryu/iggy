#include "runtime/RuntimeGameplayOrchestratedFrameRunnerReport.hpp"

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
	const RuntimeGameplayOrchestratedFrameReport &frame)
{
	AppendInventoryEvents(report.inventoryEvents, frame.inventoryEvents);
	report.acceptedCommandCount += frame.acceptedCommandCount;
	report.blockedIntentCount += frame.blockedIntentCount;
	report.rejectedIntentCount += frame.rejectedIntentCount;
	report.pickedUpCount += frame.pickedUpCount;
	report.inventoryEventCount += frame.inventoryEventCount;
	report.npcControlPlannedRequestCount += frame.npcControlPlannedRequestCount;
	report.npcControlAppliedCount += frame.npcControlAppliedCount;
	report.npcControlFailedCount += frame.npcControlFailedCount;
	report.npcMovementPlannedRequestCount += frame.npcMovementPlannedRequestCount;
	report.npcMovedCount += frame.npcMovedCount;
	report.npcBlockedMovementCount += frame.npcBlockedMovementCount;
	report.npcRejectedMovementCount += frame.npcRejectedMovementCount;
	report.npcMissingActorMovementCount += frame.npcMissingActorMovementCount;
	report.npcRefreshDirtyTileCount += frame.npcRefreshDirtyTileCount;
	report.interactionChanged = report.interactionChanged || frame.interactionChanged;
	report.inventoryChanged = report.inventoryChanged || frame.inventoryChanged;
	report.npcControlsChanged = report.npcControlsChanged || frame.npcControlsChanged;
	report.npcActorsChanged = report.npcActorsChanged || frame.npcActorsChanged;
	report.npcOccupancyRefreshed = report.npcOccupancyRefreshed || frame.npcOccupancyRefreshed;
	report.npcInteractionRefreshed = report.npcInteractionRefreshed || frame.npcInteractionRefreshed;
	report.npcAiMapRefreshed = report.npcAiMapRefreshed || frame.npcAiMapRefreshed;
	report.npcRenderRefreshed = report.npcRenderRefreshed || frame.npcRenderRefreshed;
	report.npcVisibilityRefreshed = report.npcVisibilityRefreshed || frame.npcVisibilityRefreshed;
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
	for (const RuntimeGameplayOrchestratedFrameResult &frameResult : result.frameResults) {
		RuntimeGameplayOrchestratedFrameReport frame = frameReporter.report(frameResult);
		AccumulateFrame(report, frame);
		report.frames.push_back(frame);
	}

	AppendEvents(report);
	return report;
}

} // namespace iggy::runtime
