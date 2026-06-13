#include "scene/inventory/LevelItemDrop2D.hpp"

namespace {

bool HasEarlierMatchingId(const std::vector<iggy::LevelItemDrop2D> &drops, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (drops[index].id == drops[currentIndex].id)
			return true;
	}
	return false;
}

} // namespace

namespace iggy {

const LevelItemDrop2D *LevelItemDrop2DRegistry::find(const ResourceId &id) const
{
	for (const LevelItemDrop2D &drop : drops) {
		if (drop.id == id)
			return &drop;
	}
	return nullptr;
}

bool LevelItemDrop2DRegistry::contains(const ResourceId &id) const
{
	return find(id) != nullptr;
}

LevelItemDrop2DRegistryBuildResult LevelItemDrop2DRegistryBuilder::build(const std::vector<LevelItemDrop2D> &drops) const
{
	LevelItemDrop2DRegistryBuildResult result;
	for (std::size_t index = 0; index < drops.size(); ++index) {
		const LevelItemDrop2D &drop = drops[index];
		if (drop.id.empty()) {
			result.issues.push_back({
				LevelItemDrop2DIssueCode::EmptyId,
				index,
				drop,
			});
		}
		if (HasEarlierMatchingId(drops, index)) {
			result.issues.push_back({
				LevelItemDrop2DIssueCode::DuplicateId,
				index,
				drop,
			});
		}
		if (drop.itemId.empty()) {
			result.issues.push_back({
				LevelItemDrop2DIssueCode::EmptyItemId,
				index,
				drop,
			});
		}
		if (drop.count == 0) {
			result.issues.push_back({
				LevelItemDrop2DIssueCode::ZeroCount,
				index,
				drop,
			});
		}
		if (drop.pickupRadius < 0.0F) {
			result.issues.push_back({
				LevelItemDrop2DIssueCode::NegativePickupRadius,
				index,
				drop,
			});
		}
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.registry.drops = drops;
	return result;
}

} // namespace iggy
