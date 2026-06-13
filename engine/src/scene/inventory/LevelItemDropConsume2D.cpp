#include "scene/inventory/LevelItemDropConsume2D.hpp"

namespace iggy {

LevelItemDropConsume2DResult LevelItemDropConsume2D::consume(
	const LevelItemDrop2DRegistry &registry,
	ResourceId dropId,
	LevelItemDropConsume2DMode mode) const
{
	LevelItemDropConsume2DResult result;
	result.registry = registry;
	result.dropId = dropId;
	result.mode = mode;

	if (dropId.empty()) {
		result.status = LevelItemDropConsume2DStatus::MissingDropId;
		return result;
	}

	for (std::size_t index = 0; index < result.registry.drops.size(); ++index) {
		LevelItemDrop2D &drop = result.registry.drops[index];
		if (drop.id != dropId)
			continue;

		if (!drop.enabled) {
			result.status = LevelItemDropConsume2DStatus::AlreadyDisabled;
			return result;
		}

		if (mode == LevelItemDropConsume2DMode::Remove) {
			result.registry.drops.erase(result.registry.drops.begin() + static_cast<std::ptrdiff_t>(index));
		} else {
			drop.enabled = false;
		}

		result.status = LevelItemDropConsume2DStatus::Consumed;
		result.changed = true;
		return result;
	}

	result.status = LevelItemDropConsume2DStatus::DropNotFound;
	return result;
}

} // namespace iggy
