#pragma once

#include <optional>

#include "core/math/Vec2.hpp"

namespace iggy::navigation {

enum class NavigationRequestStatus {
	None,
	Accepted,
	MissingDestination,
	DestinationOutOfBounds,
	DestinationBlocked,
};

struct NavigationRequest {
	NavigationRequestStatus status = NavigationRequestStatus::None;
	std::optional<Vec2> destination;
	int destinationTileX = -1;
	int destinationTileY = -1;

	[[nodiscard]] bool accepted() const;
};

struct NavigationGridValidationInput {
	bool requestsNavigation = false;
	bool hasDestination = false;
	Vec2 destination;
};

} // namespace iggy::navigation
