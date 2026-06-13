#include "scene/inventory/PickupPolicyTransfer2D.hpp"

namespace iggy {
namespace {

void AppendEvents(InventoryEventRecorder2D &destination, const InventoryEventRecorder2D &source)
{
	for (const InventoryEvent2D &event : source.events)
		recordInventoryEvent(destination, event);
}

} // namespace

PickupPolicyTransfer2DResult PickupPolicyTransfer2D::transfer(
	const PickupPlan2DResult &plan,
	const InventoryState2D &inventory,
	const LevelItemDrop2DRegistry &drops,
	const ItemDefinition2DCatalog &catalog,
	const PickupPolicyTransfer2DConfig &config) const
{
	PickupPolicyTransfer2DResult result;
	result.plan = plan;
	result.inventory = inventory;
	result.drops = drops;

	if (!plan.ready()) {
		result.status = PickupPolicyTransfer2DStatus::PickupNotReady;
		return result;
	}

	result.add = InventoryPolicyAddItem2D {}.add(inventory, catalog, plan.drop.itemId, plan.drop.count);
	result.inventory = result.add.inventory;
	AppendEvents(result.events, result.add.events);
	if (result.add.status != InventoryPolicyAddItem2DStatus::Added) {
		result.status = PickupPolicyTransfer2DStatus::InventoryAddFailed;
		return result;
	}

	result.consume = LevelItemDropConsume2D {}.consume(drops, plan.drop.id, config.consumeMode);
	result.drops = result.consume.registry;
	if (result.consume.status != LevelItemDropConsume2DStatus::Consumed) {
		result.status = PickupPolicyTransfer2DStatus::DropConsumeFailed;
		return result;
	}
	AppendEvents(result.events, result.consume.events);

	result.status = PickupPolicyTransfer2DStatus::Transferred;
	result.changed = true;
	recordInventoryEvent(result.events, itemPickedUpInventoryEvent(plan.drop.id, plan.drop.itemId, plan.drop.count));
	return result;
}

} // namespace iggy
