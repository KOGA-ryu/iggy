#pragma once

#include "targeting/Target.hpp"

namespace dev {

enum class DestinationActionType {
	None,
	Attack,
	Pickup,
	Talk,
	Interact,
};

struct DestinationAction {
	DestinationActionType type = DestinationActionType::None;
	Target target;
	int rangeTiles = 0;
};

} // namespace dev

