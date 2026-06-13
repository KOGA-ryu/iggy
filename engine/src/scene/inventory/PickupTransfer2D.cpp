#include "scene/inventory/PickupTransfer2D.hpp"

namespace iggy {

PickupTransfer2DResult PickupTransfer2D::transfer(
	const InventoryState2D &inventory,
	const LevelItemDrop2DRegistry &drops,
	const PickupPlan2DResult &plan,
	const PickupTransfer2DConfig &config) const
{
	PickupTransfer2DResult result;
	result.inventory = inventory;
	result.drops = drops;
	result.plan = plan;

	if (!plan.ready()) {
		result.status = PickupTransfer2DStatus::PickupNotReady;
		return result;
	}

	result.add = InventoryAddItem2D {}.add(inventory, plan.drop.itemId, plan.drop.count);
	result.inventory = result.add.inventory;
	if (result.add.status != InventoryAddItem2DStatus::Added) {
		result.status = PickupTransfer2DStatus::InventoryAddFailed;
		return result;
	}

	result.consume = LevelItemDropConsume2D {}.consume(drops, plan.drop.id, config.consumeMode);
	result.drops = result.consume.registry;
	if (result.consume.status != LevelItemDropConsume2DStatus::Consumed) {
		result.status = PickupTransfer2DStatus::DropConsumeFailed;
		return result;
	}

	result.status = PickupTransfer2DStatus::Transferred;
	result.changed = true;
	return result;
}

} // namespace iggy
