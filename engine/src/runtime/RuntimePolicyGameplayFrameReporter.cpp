#include "runtime/RuntimePolicyGameplayFrameReporter.hpp"

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

void CountInventoryEvents(RuntimePolicyGameplayFrameReport &report)
{
	report.inventoryEventCount = report.inventoryEvents.events.size();
	for (const InventoryEvent2D &event : report.inventoryEvents.events) {
		if (event.type == InventoryEvent2DType::ItemAdded) {
			++report.itemAddedEventCount;
		} else if (event.type == InventoryEvent2DType::ItemPickedUp) {
			++report.itemPickedUpEventCount;
		} else if (event.type == InventoryEvent2DType::DropConsumed) {
			++report.dropConsumedEventCount;
		} else if (event.type == InventoryEvent2DType::PickupNotReady) {
			++report.pickupNotReadyEventCount;
		} else if (event.type == InventoryEvent2DType::InventoryAddFailed) {
			++report.inventoryAddFailedEventCount;
		}
	}
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
	CountInventoryEvents(report);

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

	return report;
}

} // namespace iggy::runtime
