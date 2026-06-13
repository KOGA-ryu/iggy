#include "scene/inventory/InventoryAddItem2D.hpp"

namespace iggy {

InventoryAddItem2DResult InventoryAddItem2D::add(
	const InventoryState2D &inventory,
	ResourceId itemId,
	std::uint32_t count) const
{
	InventoryAddItem2DResult result;
	result.inventory = inventory;
	result.itemId = itemId;
	result.count = count;

	if (itemId.empty()) {
		result.status = InventoryAddItem2DStatus::InvalidItemId;
		return result;
	}
	if (count == 0) {
		result.status = InventoryAddItem2DStatus::InvalidCount;
		return result;
	}

	for (InventoryItemStack2D &stack : result.inventory.stacks) {
		if (stack.itemId == itemId) {
			stack.count += count;
			result.status = InventoryAddItem2DStatus::Added;
			result.changed = true;
			recordInventoryEvent(result.events, itemAddedInventoryEvent(itemId, count));
			return result;
		}
	}

	result.inventory.stacks.push_back({ itemId, count });
	result.status = InventoryAddItem2DStatus::Added;
	result.changed = true;
	recordInventoryEvent(result.events, itemAddedInventoryEvent(itemId, count));
	return result;
}

} // namespace iggy
