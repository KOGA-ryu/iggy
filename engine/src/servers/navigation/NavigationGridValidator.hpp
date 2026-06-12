#pragma once

#include "modules/npc_ai/NpcMovementPlan.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "servers/navigation/NavigationRequest.hpp"

namespace iggy::navigation {

class NavigationGridValidator {
public:
	[[nodiscard]] NavigationRequest validate(const LevelTileMap &map, const npc_ai::NpcMovementPlan &plan) const;
};

} // namespace iggy::navigation
