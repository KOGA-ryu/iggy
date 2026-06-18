#include "runtime/RuntimePolicyGameplayFrameReporter.hpp"

#include "runtime/RuntimeInventoryEventCountProjection.hpp"
#include "runtime/RuntimePlayerInputInteractionEffectApplyFrameReporter.hpp"

namespace iggy::runtime {

bool RuntimePolicyGameplayFrameReport::hasEvents() const
{
	return !events.empty();
}

namespace {

RuntimePlayerInputInteractionEffectApplyFrameResult InteractionResultFrom(
	const RuntimePlayerInputInteractionStateApplyFrameResult &stateResult)
{
	RuntimePlayerInputInteractionEffectApplyFrameResult result;
	result.playerInput = stateResult.playerInput;
	result.application = stateResult.application;
	result.session = stateResult.session;
	result.queue = stateResult.queue;
	result.interactionTargets = stateResult.interaction.targets;
	result.events = stateResult.events;
	return result;
}

void ProjectInventoryEvents(RuntimePolicyGameplayFrameReport &report)
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

void ProjectNpcMovement(RuntimePolicyGameplayFrameReport &report, const RuntimePolicyGameplayFrameResult &result)
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

bool NpcMovementNeedsRefresh(const RuntimePolicyGameplayFrameReport &report)
{
	return report.npcMovementNeedsOccupancyRebuild
		|| report.npcMovementNeedsAiMapQueryRefresh
		|| report.npcMovementNeedsInteractionRefresh
		|| report.npcMovementNeedsRenderRefresh
		|| report.npcMovementNeedsVisibilityRefresh;
}

} // namespace

RuntimePolicyGameplayFrameReport RuntimePolicyGameplayFrameReporter::report(
	const RuntimePolicyGameplayFrameResult &result) const
{
	RuntimePolicyGameplayFrameReport report;
	report.interaction = RuntimePlayerInputInteractionEffectApplyFrameReporter {}.report(
		InteractionResultFrom(result.frame.interaction));
	report.pickup = result.frame.pickup;
	report.inventoryEvents = result.inventoryEvents;
	report.acceptedCommandCount = report.interaction.acceptedCommandCount;
	report.blockedIntentCount = report.interaction.blockedIntentCount;
	report.rejectedIntentCount = report.interaction.rejectedIntentCount;
	report.interactionEventCount = report.interaction.interactionEventCount;
	report.pickedUpCount = result.frame.pickup.pickedUpCount;
	report.pickupNotReadyCount = result.frame.pickup.notReadyCount;
	report.pickupFailedCount = result.frame.pickup.failedCount;
	report.interactionChanged = report.interaction.mutated;
	report.inventoryChanged = result.frame.pickup.changed;
	ProjectInventoryEvents(report);
	ProjectNpcMovement(report, result);

	if (report.acceptedCommandCount > 0)
		report.events.push_back(RuntimePolicyGameplayFrameEvent::PlayerCommandAccepted);
	if (report.blockedIntentCount > 0)
		report.events.push_back(RuntimePolicyGameplayFrameEvent::PlayerIntentBlocked);
	if (report.rejectedIntentCount > 0)
		report.events.push_back(RuntimePolicyGameplayFrameEvent::PlayerIntentRejected);
	if (report.interaction.playerInput.queuedFrameCount > 0)
		report.events.push_back(RuntimePolicyGameplayFrameEvent::CommandFrameQueued);
	if (report.interaction.playerInput.tickResultCount > 0)
		report.events.push_back(RuntimePolicyGameplayFrameEvent::CommandRunnerRan);
	if (report.interactionChanged)
		report.events.push_back(RuntimePolicyGameplayFrameEvent::InteractionChanged);
	if (report.interactionEventCount > 0)
		report.events.push_back(RuntimePolicyGameplayFrameEvent::InteractionEventRecorded);
	if (report.inventoryChanged)
		report.events.push_back(RuntimePolicyGameplayFrameEvent::InventoryChanged);
	if (report.pickedUpCount > 0)
		report.events.push_back(RuntimePolicyGameplayFrameEvent::ItemPickedUp);
	if (report.pickupNotReadyCount > 0)
		report.events.push_back(RuntimePolicyGameplayFrameEvent::PickupNotReady);
	if (report.pickupFailedCount > 0)
		report.events.push_back(RuntimePolicyGameplayFrameEvent::PickupFailed);
	if (report.inventoryEventCount > 0)
		report.events.push_back(RuntimePolicyGameplayFrameEvent::InventoryEventRecorded);
	if (report.npcMovementChanged)
		report.events.push_back(RuntimePolicyGameplayFrameEvent::NpcMovementChanged);
	if (report.npcBlockedMovementCount > 0)
		report.events.push_back(RuntimePolicyGameplayFrameEvent::NpcMovementBlocked);
	if (report.npcRejectedMovementCount > 0)
		report.events.push_back(RuntimePolicyGameplayFrameEvent::NpcMovementRejected);
	if (report.npcMissingActorMovementCount > 0)
		report.events.push_back(RuntimePolicyGameplayFrameEvent::NpcMovementActorMissing);
	if (NpcMovementNeedsRefresh(report))
		report.events.push_back(RuntimePolicyGameplayFrameEvent::NpcMovementRefreshNeeded);

	return report;
}

} // namespace iggy::runtime
