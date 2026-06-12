#pragma once

#include "core/math/Vec2.hpp"
#include "modules/line_of_sight/LineOfSight.hpp"
#include "modules/npc_ai/AwarenessSensor.hpp"
#include "modules/npc_ai/AwarenessState.hpp"
#include "modules/npc_ai/NpcIntent.hpp"
#include "modules/npc_ai/NpcIntentSelector.hpp"
#include "modules/npc_ai/NpcMovementPlan.hpp"
#include "modules/npc_ai/NpcNavigationController.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "servers/navigation/NavigationPathFollower.hpp"

namespace iggy::npc_ai {

struct NpcBrainTickInput {
	Vec2 npcPosition;
	Vec2 playerPosition;
	line_of_sight::TileCoord homeTile;
	AwarenessState awarenessState;
	navigation::NavigationPathFollowState followState;
	float maxDistance = 0.0F;
	AwarenessSensorConfig awarenessConfig;
	NpcIntentSelectorConfig intentConfig;
};

struct NpcBrainTickResult {
	AwarenessEvent awarenessEvent;
	AwarenessState awarenessState;
	NpcIntent intent;
	NpcMovementPlan movementPlan;
	NpcNavigationResult navigation;
	Vec2 nextPosition;
	navigation::NavigationPathFollowState followState;
};

class NpcBrainTick {
public:
	[[nodiscard]] NpcBrainTickResult tick(const LevelTileMap &map, const NpcBrainTickInput &input) const;
};

} // namespace iggy::npc_ai
