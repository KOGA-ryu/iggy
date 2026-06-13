#include "runtime/RuntimeGameplayFrameReporter.hpp"

namespace iggy::runtime {

bool RuntimeGameplayFrameReport::hasEvents() const
{
	return !events.empty();
}

namespace {

void CountInventoryEvents(RuntimeGameplayFrameReport &report)
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
	CountInventoryEvents(report);

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

	return report;
}

} // namespace iggy::runtime
