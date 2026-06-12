#pragma once

#include "core/math/Vec2.hpp"
#include "modules/npc_ai/AwarenessState.hpp"
#include "scene/level/TileCoord.hpp"
#include "servers/navigation/NavigationPathFollower.hpp"

namespace iggy::npc_ai {

struct NpcAgentState {
	Vec2 position;
	TileCoord homeTile;
	AwarenessState awareness;
	navigation::NavigationPathFollowState followState;
};

} // namespace iggy::npc_ai
