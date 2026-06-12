#pragma once

#include "core/math/Vec2.hpp"
#include "modules/line_of_sight/LineOfSight.hpp"
#include "modules/npc_ai/AwarenessState.hpp"
#include "servers/navigation/NavigationPathFollower.hpp"

namespace iggy::npc_ai {

struct NpcAgentState {
	Vec2 position;
	line_of_sight::TileCoord homeTile;
	AwarenessState awareness;
	navigation::NavigationPathFollowState followState;
};

} // namespace iggy::npc_ai
