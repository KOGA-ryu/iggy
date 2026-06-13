#include "scene/inventory/PickupTransfer2D.hpp"

namespace iggy {
namespace {

void AppendEvents(InventoryEventRecorder2D &destination, const InventoryEventRecorder2D &source)
{
	for (const InventoryEvent2D &event : source.events)
		recordInventoryEvent(destination, event);
}

} // namespace

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
		recordInventoryEvent(result.events, inventoryAddFailedEvent(plan.drop.itemId, plan.drop.count));
		return result;
	}
	AppendEvents(result.events, result.add.events);

	result.consume = LevelItemDropConsume2D {}.consume(drops, plan.drop.id, config.consumeMode);
	result.drops = result.consume.registry;
	if (result.consume.status != LevelItemDropConsume2DStatus::Consumed) {
		result.status = PickupTransfer2DStatus::DropConsumeFailed;
		return result;
	}
	AppendEvents(result.events, result.consume.events);

	result.status = PickupTransfer2DStatus::Transferred;
	result.changed = true;
	recordInventoryEvent(result.events, itemPickedUpInventoryEvent(plan.drop.id, plan.drop.itemId, plan.drop.count));
	return result;
}

} // namespace iggy
