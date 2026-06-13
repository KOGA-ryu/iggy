#include "scene/interaction/InteractionReach2D.hpp"

#include <algorithm>

namespace iggy {

bool InteractionReach2DResult::reachable() const
{
	return status == InteractionReach2DStatus::Reachable;
}

InteractionReach2DResult InteractionReach2D::evaluate(
	Vec2 actorPosition,
	const InteractionTargetQuery2DResult &query,
	const InteractionReach2DConfig &config) const
{
	InteractionReach2DResult result;
	result.query = query;
	result.actorPosition = actorPosition;

	switch (query.status) {
	case InteractionTargetQuery2DStatus::MissingTargetId:
		result.status = InteractionReach2DStatus::MissingTargetId;
		return result;
	case InteractionTargetQuery2DStatus::NotFound:
		result.status = InteractionReach2DStatus::TargetNotFound;
		return result;
	case InteractionTargetQuery2DStatus::Disabled:
		result.status = InteractionReach2DStatus::TargetDisabled;
		return result;
	case InteractionTargetQuery2DStatus::Found:
		break;
	}

	result.distance = (query.target.position - actorPosition).length();
	result.allowedDistance = std::max(0.0F, query.target.radius) + std::max(0.0F, config.extraReach);
	result.status = result.distance <= result.allowedDistance
		? InteractionReach2DStatus::Reachable
		: InteractionReach2DStatus::OutOfRange;
	return result;
}

} // namespace iggy
