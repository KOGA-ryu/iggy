#pragma once

#include "core/resource/ResourceId.hpp"
#include "scene/inventory/LevelItemDrop2D.hpp"

namespace iggy {

enum class LevelItemDropConsume2DMode {
	Disable,
	Remove,
};

enum class LevelItemDropConsume2DStatus {
	Consumed,
	MissingDropId,
	DropNotFound,
	AlreadyDisabled,
};

struct LevelItemDropConsume2DResult {
	LevelItemDropConsume2DStatus status = LevelItemDropConsume2DStatus::MissingDropId;
	LevelItemDrop2DRegistry registry;
	ResourceId dropId;
	LevelItemDropConsume2DMode mode = LevelItemDropConsume2DMode::Disable;
	bool changed = false;
};

class LevelItemDropConsume2D {
public:
	[[nodiscard]] LevelItemDropConsume2DResult consume(
		const LevelItemDrop2DRegistry &registry,
		ResourceId dropId,
		LevelItemDropConsume2DMode mode = LevelItemDropConsume2DMode::Disable) const;
};

} // namespace iggy
