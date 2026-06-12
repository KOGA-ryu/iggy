#include "servers/navigation/NavigationGridValidator.hpp"

#include <cmath>

namespace iggy::navigation {

namespace {

NavigationRequest Request(NavigationRequestStatus status, std::optional<Vec2> destination = {}, int tileX = -1, int tileY = -1)
{
	return { status, destination, tileX, tileY };
}

} // namespace

bool NavigationRequest::accepted() const
{
	return status == NavigationRequestStatus::Accepted;
}

NavigationRequest NavigationGridValidator::validate(const LevelTileMap &map, const npc_ai::NpcMovementPlan &plan) const
{
	if (plan.type == npc_ai::NpcMovementPlanType::None)
		return Request(NavigationRequestStatus::None);
	if (!plan.destination.has_value())
		return Request(NavigationRequestStatus::MissingDestination);

	const Vec2 destination = *plan.destination;
	const int tileX = static_cast<int>(std::floor(destination.x));
	const int tileY = static_cast<int>(std::floor(destination.y));
	const LevelTile *tile = map.tileAt(tileX, tileY);
	if (tile == nullptr)
		return Request(NavigationRequestStatus::DestinationOutOfBounds, destination, tileX, tileY);
	if (!tile->walkable)
		return Request(NavigationRequestStatus::DestinationBlocked, destination, tileX, tileY);
	return Request(NavigationRequestStatus::Accepted, destination, tileX, tileY);
}

} // namespace iggy::navigation
