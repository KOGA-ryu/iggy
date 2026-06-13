#include "scene/inventory/InventoryStackPolicy2D.hpp"

namespace iggy {

bool InventoryStackPolicy2DResult::canAdd() const
{
	return status == InventoryStackPolicy2DStatus::CanAdd;
}

InventoryStackPolicy2DResult InventoryStackPolicy2D::planAdd(
	const InventoryState2D &inventory,
	const ItemDefinition2DCatalog &catalog,
	ResourceId itemId,
	std::uint32_t count) const
{
	InventoryStackPolicy2DResult result;
	result.itemId = itemId;
	result.requestedCount = count;

	if (itemId.empty()) {
		result.status = InventoryStackPolicy2DStatus::MissingItemId;
		return result;
	}

	if (count == 0) {
		result.status = InventoryStackPolicy2DStatus::InvalidCount;
		return result;
	}

	const ItemDefinition2D *definition = catalog.find(itemId);
	if (definition == nullptr) {
		result.status = InventoryStackPolicy2DStatus::ItemDefinitionNotFound;
		return result;
	}

	result.definition = *definition;
	result.maxStackCount = definition->maxStackCount;

	if (const InventoryItemStack2D *stack = inventory.find(itemId)) {
		result.currentCount = stack->count;
	}

	result.resultingCount = result.currentCount + count;
	if (result.resultingCount > result.maxStackCount) {
		result.status = InventoryStackPolicy2DStatus::StackLimitExceeded;
		return result;
	}

	result.status = InventoryStackPolicy2DStatus::CanAdd;
	return result;
}

} // namespace iggy
