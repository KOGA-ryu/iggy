#include "runtime/RuntimeGameplayOrchestratedFrameReport.hpp"

namespace iggy::runtime {
namespace {

bool PlayerFrameHasFacts(const RuntimeGameplayOrchestratedFrameReport &report)
{
	return report.acceptedCommandCount > 0
		|| report.blockedIntentCount > 0
		|| report.rejectedIntentCount > 0
		|| report.pickedUpCount > 0
		|| report.interactionChanged
		|| report.inventoryChanged;
}

bool NpcControlWasEvaluated(const RuntimeGameplayOrchestratedFrameReport &report)
{
	return report.npcControlPlannedRequestCount > 0
		|| report.npcControlAppliedCount > 0
		|| report.npcControlFailedCount > 0;
}

void AppendEvents(RuntimeGameplayOrchestratedFrameReport &report)
{
	if (PlayerFrameHasFacts(report)) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameEvent::PlayerFrameRan);
	}
	if (report.inventoryEventCount > 0) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameEvent::InventoryEventObserved);
	}
	if (report.npcControlsChanged) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameEvent::NpcControlChanged);
	} else if (NpcControlWasEvaluated(report)) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameEvent::NpcControlUnchanged);
	}
	if (report.npcMovedCount > 0) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameEvent::NpcActorMoved);
	}
	if (report.npcBlockedMovementCount > 0) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameEvent::NpcActorBlocked);
	}
	if (report.npcRejectedMovementCount > 0) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameEvent::NpcActorRejected);
	}
	if (report.npcMissingActorMovementCount > 0) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameEvent::NpcActorMissing);
	}
	if (report.npcOccupancyRefreshed) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameEvent::RefreshOccupancy);
	}
	if (report.npcInteractionRefreshed) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameEvent::RefreshInteraction);
	}
	if (report.npcAiMapRefreshed) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameEvent::RefreshAiMap);
	}
	if (report.npcRenderRefreshed) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameEvent::RefreshRender);
	}
	if (report.npcVisibilityRefreshed) {
		report.events.push_back(RuntimeGameplayOrchestratedFrameEvent::RefreshVisibility);
	}
	report.events.push_back(
		report.changed() || report.refreshedNpcData()
			? RuntimeGameplayOrchestratedFrameEvent::FrameChanged
			: RuntimeGameplayOrchestratedFrameEvent::FrameUnchanged);
}

} // namespace

bool RuntimeGameplayOrchestratedFrameReport::changed() const
{
	return playerChanged || npcControlsChanged || npcActorsChanged;
}

bool RuntimeGameplayOrchestratedFrameReport::refreshedNpcData() const
{
	return npcOccupancyRefreshed
		|| npcInteractionRefreshed
		|| npcAiMapRefreshed
		|| npcRenderRefreshed
		|| npcVisibilityRefreshed;
}

bool RuntimeGameplayOrchestratedFrameReport::hasEvents() const
{
	return !events.empty();
}

RuntimeGameplayOrchestratedFrameReport RuntimeGameplayOrchestratedFrameReporter::report(
	const RuntimeGameplayOrchestratedFrameResult &result) const
{
	RuntimeGameplayOrchestratedFrameReport report;
	report.result = result;
	report.player = RuntimeGameplayFrameReporter {}.report(result.playerFrame);
	report.inventoryEvents = result.inventoryEvents;
	report.acceptedCommandCount = result.acceptedCommandCount;
	report.blockedIntentCount = result.blockedIntentCount;
	report.rejectedIntentCount = result.rejectedIntentCount;
	report.pickedUpCount = result.pickedUpCount;
	report.inventoryEventCount = result.inventoryEventCount;
	report.npcControlPlannedRequestCount = result.npcControlPlannedRequestCount;
	report.npcControlAppliedCount = result.npcControlAppliedCount;
	report.npcControlFailedCount = result.npcControlFailedCount;
	report.npcMovementPlannedRequestCount = result.npcMovementPlannedRequestCount;
	report.npcMovedCount = result.npcMovedCount;
	report.npcBlockedMovementCount = result.npcBlockedMovementCount;
	report.npcRejectedMovementCount = result.npcRejectedMovementCount;
	report.npcMissingActorMovementCount = result.npcMissingActorMovementCount;
	report.npcRefreshDirtyTileCount = result.npcRefreshDirtyTileCount;
	report.interactionChanged = result.interactionChanged;
	report.inventoryChanged = result.inventoryChanged;
	report.playerChanged = result.acceptedCommandCount > 0
		|| result.interactionChanged
		|| result.inventoryChanged;
	report.npcControlsChanged = result.npcControlsChanged;
	report.npcActorsChanged = result.npcActorsChanged;
	report.npcOccupancyRefreshed = result.npcOccupancyRefreshed;
	report.npcInteractionRefreshed = result.npcInteractionRefreshed;
	report.npcAiMapRefreshed = result.npcAiMapRefreshed;
	report.npcRenderRefreshed = result.npcRenderRefreshed;
	report.npcVisibilityRefreshed = result.npcVisibilityRefreshed;
	AppendEvents(report);
	return report;
}

} // namespace iggy::runtime
