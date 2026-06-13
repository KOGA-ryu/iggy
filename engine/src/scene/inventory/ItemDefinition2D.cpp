#include "scene/inventory/ItemDefinition2D.hpp"

namespace {

bool HasEarlierMatchingId(const std::vector<iggy::ItemDefinition2D> &entries, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (entries[index].itemId == entries[currentIndex].itemId)
			return true;
	}
	return false;
}

} // namespace

namespace iggy {

const ItemDefinition2D *ItemDefinition2DCatalog::find(const ResourceId &itemId) const
{
	for (const ItemDefinition2D &entry : entries) {
		if (entry.itemId == itemId)
			return &entry;
	}
	return nullptr;
}

bool ItemDefinition2DCatalog::contains(const ResourceId &itemId) const
{
	return find(itemId) != nullptr;
}

ItemDefinition2DCatalogBuildResult ItemDefinition2DCatalogBuilder::build(
	const std::vector<ItemDefinition2D> &entries) const
{
	ItemDefinition2DCatalogBuildResult result;
	for (std::size_t index = 0; index < entries.size(); ++index) {
		const ItemDefinition2D &entry = entries[index];
		if (entry.itemId.empty()) {
			result.issues.push_back({
				ItemDefinition2DIssueCode::EmptyItemId,
				index,
				entry,
			});
		}
		if (HasEarlierMatchingId(entries, index)) {
			result.issues.push_back({
				ItemDefinition2DIssueCode::DuplicateItemId,
				index,
				entry,
			});
		}
		if (entry.displayName.empty()) {
			result.issues.push_back({
				ItemDefinition2DIssueCode::EmptyDisplayName,
				index,
				entry,
			});
		}
		if (entry.maxStackCount == 0) {
			result.issues.push_back({
				ItemDefinition2DIssueCode::ZeroMaxStackCount,
				index,
				entry,
			});
		}
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.catalog.entries = entries;
	return result;
}

} // namespace iggy
