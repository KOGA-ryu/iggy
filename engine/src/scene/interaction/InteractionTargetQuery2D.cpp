#include "scene/interaction/InteractionTargetQuery2D.hpp"

#include <utility>

namespace iggy {

bool InteractionTargetQuery2DResult::hasTarget() const
{
	return status == InteractionTargetQuery2DStatus::Found || status == InteractionTargetQuery2DStatus::Disabled;
}

InteractionTargetQuery2DResult InteractionTargetQuery2D::find(const InteractionTarget2DRegistry &registry, ResourceId targetId) const
{
	InteractionTargetQuery2DResult result;
	result.targetId = std::move(targetId);

	if (result.targetId.empty()) {
		result.status = InteractionTargetQuery2DStatus::MissingTargetId;
		return result;
	}

	const InteractionTarget2D *target = registry.find(result.targetId);
	if (target == nullptr) {
		result.status = InteractionTargetQuery2DStatus::NotFound;
		return result;
	}

	result.target = *target;
	result.status = target->enabled ? InteractionTargetQuery2DStatus::Found : InteractionTargetQuery2DStatus::Disabled;
	return result;
}

} // namespace iggy
