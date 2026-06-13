#include "runtime/RuntimePickupStep.hpp"

namespace iggy::runtime {
namespace {

void AppendEvents(InventoryEventRecorder2D &destination, const InventoryEventRecorder2D &source)
{
	for (const InventoryEvent2D &event : source.events)
		recordInventoryEvent(destination, event);
}

} // namespace

RuntimePickupResult RuntimePickupStep::pickup(
	const RuntimeSessionState &session,
	const RuntimeInventoryState &inventory,
	ResourceId dropId,
	const RuntimePickupConfig &config) const
{
	RuntimePickupResult result;
	result.inventory = inventory;
	result.dropId = dropId;

	if (!session.hasPlayer) {
		result.status = RuntimePickupStatus::MissingPlayer;
		return result;
	}

	result.plan = PickupPlan2D {}.plan(inventory.drops, dropId, session.player.position, config.plan);
	if (!result.plan.ready()) {
		result.status = RuntimePickupStatus::PickupNotReady;
		recordInventoryEvent(result.events, pickupNotReadyInventoryEvent(dropId));
		return result;
	}

	result.transfer = PickupTransfer2D {}.transfer(
		inventory.inventory,
		inventory.drops,
		result.plan,
		config.transfer);
	result.inventory.inventory = result.transfer.inventory;
	result.inventory.drops = result.transfer.drops;
	AppendEvents(result.events, result.transfer.events);

	if (result.transfer.status != PickupTransfer2DStatus::Transferred) {
		result.status = RuntimePickupStatus::TransferFailed;
		return result;
	}

	result.status = RuntimePickupStatus::PickedUp;
	result.changed = true;
	return result;
}

} // namespace iggy::runtime
