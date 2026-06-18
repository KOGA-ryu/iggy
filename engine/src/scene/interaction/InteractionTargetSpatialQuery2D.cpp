#include "scene/interaction/InteractionTargetSpatialQuery2D.hpp"

#include <algorithm>

namespace iggy {

bool InteractionTargetSpatialQuery2DResult::hasTarget() const
{
	return status == InteractionTargetSpatialQuery2DStatus::Found;
}

InteractionTargetSpatialQuery2DResult InteractionTargetSpatialQuery2D::find(
	const InteractionTarget2DRegistry &registry,
	Vec2 point,
	const InteractionTargetSpatialQuery2DConfig &config) const
{
	InteractionTargetSpatialQuery2DResult result;
	bool found = false;
	float bestDistance = 0.0F;

	const std::vector<InteractionTarget2D> &targets = registry.targets();
	for (std::size_t index = 0; index < targets.size(); ++index) {
		const InteractionTarget2D &target = targets[index];
		if (!target.enabled)
			continue;

		const float allowedDistance =
			std::max(0.0F, target.radius) + std::max(0.0F, config.extraRadius);
		const float distance = (target.position - point).length();
		if (distance > allowedDistance)
			continue;
		if (found && distance >= bestDistance)
			continue;

		found = true;
		bestDistance = distance;
		result.status = InteractionTargetSpatialQuery2DStatus::Found;
		result.targetId = target.id;
		result.target = target;
		result.targetIndex = index;
		result.distance = distance;
		result.allowedDistance = allowedDistance;
	}

	return result;
}

InteractionTargetSpatialQuery2DResult InteractionTargetSpatialQuery2D::findTileCenter(
	const InteractionTarget2DRegistry &registry,
	TileCoord tile,
	const InteractionTargetSpatialQuery2DConfig &config) const
{
	return find(registry, tileCenter(tile), config);
}

} // namespace iggy
