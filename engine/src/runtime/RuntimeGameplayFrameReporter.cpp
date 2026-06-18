#include "runtime/RuntimeGameplayFrameReporter.hpp"

#include "runtime/RuntimeInventoryEventCountProjection.hpp"

namespace iggy::runtime {

bool RuntimeGameplayFrameReport::hasEvents() const
{
	return !events.empty();
}

namespace {

void ProjectInventoryEvents(RuntimeGameplayFrameReport &report)
{
	const RuntimeInventoryEventCountProjection projection =
		projectRuntimeInventoryEventCounts(report.inventoryEvents);
	report.inventoryEventCount = projection.inventoryEventCount;
	report.itemAddedEventCount = projection.itemAddedEventCount;
	report.itemPickedUpEventCount = projection.itemPickedUpEventCount;
	report.dropConsumedEventCount = projection.dropConsumedEventCount;
	report.pickupNotReadyEventCount = projection.pickupNotReadyEventCount;
	report.inventoryAddFailedEventCount =
		projection.inventoryAddFailedEventCount;
}

void ProjectNpcMovement(RuntimeGameplayFrameReport &report, const RuntimeGameplayFrameResult &result)
{
	report.npcMovement = result.npcMovement.report;
	report.npcMovedCount = report.npcMovement.movedCount;
	report.npcBlockedMovementCount = report.npcMovement.blockedCount;
	report.npcRejectedMovementCount = report.npcMovement.rejectedCount;
	report.npcMissingActorMovementCount = report.npcMovement.missingActorCount;
	report.npcMovementDirtyTileCount = report.npcMovement.dirtyTiles.size();
	report.npcMovementChanged = report.npcMovement.changed();
	report.npcMovementNeedsOccupancyRebuild = report.npcMovement.needsOccupancyRebuild;
	report.npcMovementNeedsAiMapQueryRefresh = report.npcMovement.needsAiMapQueryRefresh;
	report.npcMovementNeedsInteractionRefresh = report.npcMovement.needsInteractionRefresh;
	report.npcMovementNeedsRenderRefresh = report.npcMovement.needsRenderRefresh;
	report.npcMovementNeedsVisibilityRefresh = report.npcMovement.needsVisibilityRefresh;
}

bool NpcMovementNeedsRefresh(const RuntimeGameplayFrameReport &report)
{
	return report.npcMovementNeedsOccupancyRebuild
		|| report.npcMovementNeedsAiMapQueryRefresh
		|| report.npcMovementNeedsInteractionRefresh
		|| report.npcMovementNeedsRenderRefresh
		|| report.npcMovementNeedsVisibilityRefresh;
}

} // namespace

RuntimeGameplayFrameReport RuntimeGameplayFrameReporter::report(const RuntimeGameplayFrameResult &result) const
{
	RuntimeGameplayFrameReport report;
	report.input = result.report;
	report.inventoryEvents = result.inventoryEvents;
	report.acceptedCommandCount = result.report.acceptedCommandCount;
	report.blockedIntentCount = result.report.blockedIntentCount;
	report.rejectedIntentCount = result.report.rejectedIntentCount;
	report.interactionEventCount = result.report.interactionEventCount;
	report.pickedUpCount = result.report.pickedUpCount;
	report.pickupNotReadyCount = result.report.pickupNotReadyCount;
	report.pickupFailedCount = result.report.pickupFailedCount;
	report.interactionChanged = result.report.interactionMutated;
	report.inventoryChanged = result.report.inventoryChanged;
	ProjectInventoryEvents(report);
	ProjectNpcMovement(report, result);

	if (report.acceptedCommandCount > 0)
		report.events.push_back(RuntimeGameplayFrameEvent::PlayerCommandAccepted);
	if (report.blockedIntentCount > 0)
		report.events.push_back(RuntimeGameplayFrameEvent::PlayerIntentBlocked);
	if (report.rejectedIntentCount > 0)
		report.events.push_back(RuntimeGameplayFrameEvent::PlayerIntentRejected);
	if (result.report.interaction.playerInput.queuedFrameCount > 0)
		report.events.push_back(RuntimeGameplayFrameEvent::CommandFrameQueued);
	if (result.report.interaction.playerInput.tickResultCount > 0)
		report.events.push_back(RuntimeGameplayFrameEvent::CommandRunnerRan);
	if (report.interactionChanged)
		report.events.push_back(RuntimeGameplayFrameEvent::InteractionChanged);
	if (report.interactionEventCount > 0)
		report.events.push_back(RuntimeGameplayFrameEvent::InteractionEventRecorded);
	if (report.inventoryChanged)
		report.events.push_back(RuntimeGameplayFrameEvent::InventoryChanged);
	if (report.pickedUpCount > 0)
		report.events.push_back(RuntimeGameplayFrameEvent::ItemPickedUp);
	if (report.pickupNotReadyCount > 0)
		report.events.push_back(RuntimeGameplayFrameEvent::PickupNotReady);
	if (report.pickupFailedCount > 0)
		report.events.push_back(RuntimeGameplayFrameEvent::PickupFailed);
	if (report.inventoryEventCount > 0)
		report.events.push_back(RuntimeGameplayFrameEvent::InventoryEventRecorded);
	if (report.npcMovementChanged)
		report.events.push_back(RuntimeGameplayFrameEvent::NpcMovementChanged);
	if (report.npcBlockedMovementCount > 0)
		report.events.push_back(RuntimeGameplayFrameEvent::NpcMovementBlocked);
	if (report.npcRejectedMovementCount > 0)
		report.events.push_back(RuntimeGameplayFrameEvent::NpcMovementRejected);
	if (report.npcMissingActorMovementCount > 0)
		report.events.push_back(RuntimeGameplayFrameEvent::NpcMovementActorMissing);
	if (NpcMovementNeedsRefresh(report))
		report.events.push_back(RuntimeGameplayFrameEvent::NpcMovementRefreshNeeded);

	return report;
}

} // namespace iggy::runtime
