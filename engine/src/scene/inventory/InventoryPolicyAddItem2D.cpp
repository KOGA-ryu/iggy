#include "scene/inventory/InventoryPolicyAddItem2D.hpp"

namespace iggy {

bool InventoryPolicyAddItem2DResult::added() const
{
	return status == InventoryPolicyAddItem2DStatus::Added;
}

InventoryPolicyAddItem2DResult InventoryPolicyAddItem2D::add(
	const InventoryState2D &inventory,
	const ItemDefinition2DCatalog &catalog,
	ResourceId itemId,
	std::uint32_t count) const
{
	InventoryPolicyAddItem2DResult result;
	result.inventory = inventory;
	result.plan = InventoryStackPolicy2D {}.planAdd(inventory, catalog, itemId, count);

	if (!result.plan.canAdd()) {
		result.status = InventoryPolicyAddItem2DStatus::PlanFailed;
		return result;
	}

	result.add = InventoryAddItem2D {}.add(inventory, itemId, count);
	result.inventory = result.add.inventory;
	result.events = result.add.events;
	if (result.add.status != InventoryAddItem2DStatus::Added) {
		result.status = InventoryPolicyAddItem2DStatus::AddFailed;
		result.changed = false;
		return result;
	}

	result.status = InventoryPolicyAddItem2DStatus::Added;
	result.changed = true;
	return result;
}

} // namespace iggy
