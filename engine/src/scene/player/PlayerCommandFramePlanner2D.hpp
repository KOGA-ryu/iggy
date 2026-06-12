#pragma once

#include <cstddef>
#include <vector>

#include "runtime/GameplayCommandFrame2DValidator.hpp"
#include "scene/player/PlayerAgentState.hpp"
#include "scene/player/PlayerCommandPlan2D.hpp"

namespace iggy {

struct PlayerCommandFramePlan2DRejectedPlan {
	std::size_t originalCommandIndex = 0;
	std::size_t planIndex = 0;
	PlayerCommandPlan2D plan;
};

struct PlayerCommandFramePlan2DResult {
	runtime::GameplayCommandFrame2DValidationResult validation;
	std::vector<PlayerCommandPlan2D> plans;
	std::vector<PlayerCommandFramePlan2DRejectedPlan> rejectedPlans;
};

class PlayerCommandFramePlanner2D {
public:
	[[nodiscard]] PlayerCommandFramePlan2DResult plan(const PlayerAgentState &player, const runtime::GameplayCommandFrame2D &frame) const;
};

} // namespace iggy
