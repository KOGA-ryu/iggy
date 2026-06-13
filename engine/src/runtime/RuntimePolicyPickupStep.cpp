#include "runtime/RuntimePolicyPickupStep.hpp"

namespace iggy::runtime {
namespace {

void AppendEvents(InventoryEventRecorder2D &destination, const InventoryEventRecorder2D &source)
{
	for (const InventoryEvent2D &event : source.events)
		recordInventoryEvent(destination, event);
}

} // namespace

RuntimePolicyPickupResult RuntimePolicyPickupStep::pickup(
	const RuntimeSessionState &session,
	const RuntimeInventoryState &inventory,
	const ItemDefinition2DCatalog &catalog,
	ResourceId dropId,
	const RuntimePolicyPickupConfig &config) const
{
	RuntimePolicyPickupResult result;
	result.dropId = dropId;
	result.inventory = inventory;

	if (!session.hasPlayer) {
		result.status = RuntimePolicyPickupStatus::MissingPlayer;
		return result;
	}

	result.plan = PickupPlan2D {}.plan(inventory.drops, dropId, session.player.position, config.plan);
	if (!result.plan.ready()) {
		result.status = RuntimePolicyPickupStatus::PickupNotReady;
		recordInventoryEvent(result.events, pickupNotReadyInventoryEvent(dropId));
		return result;
	}

	result.transfer = PickupPolicyTransfer2D {}.transfer(
		result.plan,
		inventory.inventory,
		inventory.drops,
		catalog,
		config.transfer);
	result.inventory.inventory = result.transfer.inventory;
	result.inventory.drops = result.transfer.drops;
	AppendEvents(result.events, result.transfer.events);

	if (result.transfer.status != PickupPolicyTransfer2DStatus::Transferred) {
		result.status = RuntimePolicyPickupStatus::TransferFailed;
		return result;
	}

	result.status = RuntimePolicyPickupStatus::PickedUp;
	result.changed = true;
	return result;
}

} // namespace iggy::runtime
