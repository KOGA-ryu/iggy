#include "scene/interaction/InteractionPlan2D.hpp"

#include <utility>

namespace {

iggy::InteractionPlan2DStatus PlanStatusForReach(iggy::InteractionReach2DStatus status)
{
	switch (status) {
	case iggy::InteractionReach2DStatus::Reachable:
		return iggy::InteractionPlan2DStatus::Ready;
	case iggy::InteractionReach2DStatus::MissingTargetId:
		return iggy::InteractionPlan2DStatus::MissingTargetId;
	case iggy::InteractionReach2DStatus::TargetNotFound:
		return iggy::InteractionPlan2DStatus::TargetNotFound;
	case iggy::InteractionReach2DStatus::TargetDisabled:
		return iggy::InteractionPlan2DStatus::TargetDisabled;
	case iggy::InteractionReach2DStatus::OutOfRange:
		return iggy::InteractionPlan2DStatus::OutOfRange;
	}
	return iggy::InteractionPlan2DStatus::TargetNotFound;
}

} // namespace

namespace iggy {

bool InteractionPlan2DResult::ready() const
{
	return status == InteractionPlan2DStatus::Ready;
}

InteractionPlan2DResult InteractionPlan2D::plan(
	const InteractionTarget2DRegistry &registry,
	ResourceId targetId,
	Vec2 actorPosition,
	const InteractionReach2DConfig &reachConfig) const
{
	InteractionPlan2DResult result;
	result.targetId = targetId;
	result.actorPosition = actorPosition;
	result.query = InteractionTargetQuery2D {}.find(registry, std::move(targetId));
	result.reach = InteractionReach2D {}.evaluate(actorPosition, result.query, reachConfig);
	result.status = PlanStatusForReach(result.reach.status);
	return result;
}

} // namespace iggy
