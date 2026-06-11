#pragma once

#include "targeting/Target.hpp"

namespace dev {

enum class InteractionIntentType {
	Move,
	Attack,
	Pickup,
	Talk,
	Interact,
};

struct InteractionIntent {
	InteractionIntentType type = InteractionIntentType::Move;
	Target target;
};

} // namespace dev

