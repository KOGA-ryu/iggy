#include "scene/ai/NpcAiMovementProposal2D.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

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

std::size_t FirstWaypointIndex(const std::vector<iggy::Vec2> &waypoints, iggy::Vec2 start, float tolerance)
{
	if (waypoints.empty())
		return 0;
	if (Distance(waypoints.front(), start) <= tolerance && waypoints.size() > 1)
		return 1;
	return 0;
}

} // namespace

namespace iggy {

bool NpcAiMovementProposal2DResult::hasMovementProposal() const
{
	return requestsMovement;
}

NpcAiMovementProposal2DResult NpcAiMovementProposalBuilder2D::build(
	const NpcAiPathReport2DResult &path,
	const NpcAiMovementProposal2DConfig &config) const
{
	NpcAiMovementProposal2DResult result;
	result.path = path;
	result.npcId = path.navigation.route.decision.score.currentStateValidation.state.npcId;
	result.startPosition = path.navigation.route.startPosition;
	result.finalTargetPosition = path.navigation.route.targetPosition;
	result.proposedPosition = result.finalTargetPosition;
	result.intentType = path.navigation.route.intentType;

	if (result.intentType == NpcAiBehaviorIntent2DType::HoldPosition) {
		result.status = NpcAiMovementProposal2DStatus::HoldPosition;
		return result;
	}

	if (!path.hasPath()) {
		result.status = NpcAiMovementProposal2DStatus::NoPath;
		return result;
	}

	const float arrivalTolerance = NonNegative(config.arrivalTolerance);
	if (Distance(result.startPosition, result.finalTargetPosition) <= arrivalTolerance) {
		result.status = NpcAiMovementProposal2DStatus::AlreadyAtTarget;
		return result;
	}

	if (!path.path.waypoints.empty()) {
		const std::size_t firstIndex = FirstWaypointIndex(path.path.waypoints, result.startPosition, arrivalTolerance);
		const std::size_t requestedIndex = firstIndex + config.waypointLookahead - (config.waypointLookahead > 0 ? 1 : 0);
		result.selectedWaypointIndex = std::min(requestedIndex, path.path.waypoints.size() - 1);
		result.proposedPosition = path.path.waypoints[result.selectedWaypointIndex];
	}

	result.status = NpcAiMovementProposal2DStatus::Proposed;
	result.requestsMovement = true;
	return result;
}

} // namespace iggy
