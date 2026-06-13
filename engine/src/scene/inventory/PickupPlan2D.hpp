#pragma once

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/inventory/LevelItemDrop2D.hpp"

namespace iggy {

enum class PickupPlan2DStatus {
	Ready,
	MissingDropId,
	DropNotFound,
	DropDisabled,
	OutOfRange,
};

struct PickupPlan2DConfig {
	float extraReach = 0.0F;
};

struct PickupPlan2DResult {
	PickupPlan2DStatus status = PickupPlan2DStatus::DropNotFound;
	ResourceId dropId;
	Vec2 actorPosition;
	LevelItemDrop2D drop;
	float distance = 0.0F;
	float allowedDistance = 0.0F;

	[[nodiscard]] bool ready() const;
};

class PickupPlan2D {
public:
	[[nodiscard]] PickupPlan2DResult plan(
		const LevelItemDrop2DRegistry &drops,
		ResourceId dropId,
		Vec2 actorPosition,
		const PickupPlan2DConfig &config = {}) const;
};

} // namespace iggy
