#pragma once

#include "modules/npc_ai/NpcIntent.hpp"
#include "modules/npc_ai/NpcMovementPlan.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy::npc_ai {

class NpcMovementPlanner {
public:
	[[nodiscard]] NpcMovementPlan plan(const NpcIntent &intent, TileCoord homeTile) const;
};

} // namespace iggy::npc_ai
