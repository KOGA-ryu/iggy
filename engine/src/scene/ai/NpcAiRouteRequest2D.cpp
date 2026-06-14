#include "scene/ai/NpcAiRouteRequest2D.hpp"

#include <cmath>

namespace {

float NonNegative(float value)
{
	return value < 0.0F ? 0.0F : value;
}

float Distance(iggy::Vec2 a, iggy::Vec2 b)
{
	const float dx = a.x - b.x;
	const float dy = a.y - b.y;
	return std::sqrt(dx * dx + dy * dy);
}

} // namespace

namespace iggy {

bool NpcAiRouteRequest2DResult::hasRouteRequest() const
{
	return requestsRoute;
}

NpcAiRouteRequest2DResult NpcAiRouteRequestBuilder2D::build(
	const NpcAiDecision2DResult &decision,
	const NpcAiRouteRequest2DConfig &config) const
{
	NpcAiRouteRequest2DResult result;
	result.decision = decision;
	result.intentType = decision.intent.type;
	result.startPosition = decision.score.currentStateValidation.state.position;
	result.targetPosition = decision.target.selectedPosition;

	if (!decision.hasDecision()) {
		result.status = NpcAiRouteRequest2DStatus::NoDecision;
		return result;
	}

	if (decision.intent.type == NpcAiBehaviorIntent2DType::HoldPosition) {
		result.status = NpcAiRouteRequest2DStatus::HoldPosition;
		return result;
	}

	if (Distance(result.startPosition, result.targetPosition) <= NonNegative(config.arrivalTolerance)) {
		result.status = NpcAiRouteRequest2DStatus::NoMovementNeeded;
		return result;
	}

	result.status = NpcAiRouteRequest2DStatus::Requested;
	result.requestsRoute = true;
	return result;
}

} // namespace iggy
