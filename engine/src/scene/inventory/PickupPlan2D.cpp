#include "scene/inventory/PickupPlan2D.hpp"

#include <algorithm>

namespace iggy {

bool PickupPlan2DResult::ready() const
{
	return status == PickupPlan2DStatus::Ready;
}

PickupPlan2DResult PickupPlan2D::plan(
	const LevelItemDrop2DRegistry &drops,
	ResourceId dropId,
	Vec2 actorPosition,
	const PickupPlan2DConfig &config) const
{
	PickupPlan2DResult result;
	result.dropId = dropId;
	result.actorPosition = actorPosition;

	if (dropId.empty()) {
		result.status = PickupPlan2DStatus::MissingDropId;
		return result;
	}

	const LevelItemDrop2D *drop = drops.find(dropId);
	if (drop == nullptr) {
		result.status = PickupPlan2DStatus::DropNotFound;
		return result;
	}

	result.drop = *drop;
	if (!drop->enabled) {
		result.status = PickupPlan2DStatus::DropDisabled;
		return result;
	}

	result.distance = (drop->position - actorPosition).length();
	result.allowedDistance = std::max(0.0F, drop->pickupRadius) + std::max(0.0F, config.extraReach);
	result.status = result.distance <= result.allowedDistance
		? PickupPlan2DStatus::Ready
		: PickupPlan2DStatus::OutOfRange;
	return result;
}

} // namespace iggy
