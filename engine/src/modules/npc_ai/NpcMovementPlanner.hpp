#pragma once

#include "modules/line_of_sight/LineOfSight.hpp"
#include "modules/npc_ai/NpcIntent.hpp"
#include "modules/npc_ai/NpcMovementPlan.hpp"

namespace iggy::npc_ai {

class NpcMovementPlanner {
public:
	[[nodiscard]] NpcMovementPlan plan(const NpcIntent &intent, line_of_sight::TileCoord homeTile) const;
};

} // namespace iggy::npc_ai
