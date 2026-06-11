#pragma once

#include <optional>

#include "world/Point.hpp"

namespace dev {

enum class PlayerIntentType {
	None,
	MoveTo,
	MoveDirection,
	StopMoving,
};

struct PlayerIntent {
	PlayerIntentType type = PlayerIntentType::None;
	std::optional<Point> destination;
};

} // namespace dev

