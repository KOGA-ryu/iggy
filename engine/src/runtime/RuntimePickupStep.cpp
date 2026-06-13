#include "runtime/RuntimePickupStep.hpp"

namespace iggy::runtime {

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
		return result;
	}

	result.transfer = PickupTransfer2D {}.transfer(
		inventory.inventory,
		inventory.drops,
		result.plan,
		config.transfer);
	result.inventory.inventory = result.transfer.inventory;
	result.inventory.drops = result.transfer.drops;

	if (result.transfer.status != PickupTransfer2DStatus::Transferred) {
		result.status = RuntimePickupStatus::TransferFailed;
		return result;
	}

	result.status = RuntimePickupStatus::PickedUp;
	result.changed = true;
	return result;
}

} // namespace iggy::runtime
