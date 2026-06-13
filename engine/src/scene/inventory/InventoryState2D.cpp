#include "scene/inventory/InventoryState2D.hpp"

namespace {

bool HasEarlierMatchingId(const std::vector<iggy::InventoryItemStack2D> &stacks, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (stacks[index].itemId == stacks[currentIndex].itemId)
			return true;
	}
	return false;
}

} // namespace

namespace iggy {

const InventoryItemStack2D *InventoryState2D::find(const ResourceId &itemId) const
{
	for (const InventoryItemStack2D &stack : stacks) {
		if (stack.itemId == itemId)
			return &stack;
	}
	return nullptr;
}

bool InventoryState2D::contains(const ResourceId &itemId) const
{
	return find(itemId) != nullptr;
}

InventoryState2DBuildResult InventoryState2DBuilder::build(const std::vector<InventoryItemStack2D> &stacks) const
{
	InventoryState2DBuildResult result;
	for (std::size_t index = 0; index < stacks.size(); ++index) {
		const InventoryItemStack2D &stack = stacks[index];
		if (stack.itemId.empty()) {
			result.issues.push_back({
				InventoryState2DIssueCode::EmptyItemId,
				index,
				stack,
			});
		}
		if (stack.count == 0) {
			result.issues.push_back({
				InventoryState2DIssueCode::ZeroCount,
				index,
				stack,
			});
		}
		if (HasEarlierMatchingId(stacks, index)) {
			result.issues.push_back({
				InventoryState2DIssueCode::DuplicateItemId,
				index,
				stack,
			});
		}
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.inventory.stacks = stacks;
	return result;
}

} // namespace iggy
