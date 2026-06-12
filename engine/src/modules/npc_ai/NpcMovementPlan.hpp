#pragma once

#include <optional>

#include "core/math/Vec2.hpp"

namespace iggy::npc_ai {

enum class NpcMovementPlanType {
	None,
	MoveTo,
};

struct NpcMovementPlan {
	NpcMovementPlanType type = NpcMovementPlanType::None;
	std::optional<Vec2> destination;
};

} // namespace iggy::npc_ai
