#pragma once

#include "runtime/GameplayCommand2D.hpp"
#include "scene/player/PlayerAgentState.hpp"
#include "scene/player/PlayerCommandPlan2D.hpp"

namespace iggy {

class PlayerCommandPlanner2D {
public:
	[[nodiscard]] PlayerCommandPlan2D plan(const PlayerAgentState &player, const runtime::GameplayCommand2D &command) const;
};

} // namespace iggy
