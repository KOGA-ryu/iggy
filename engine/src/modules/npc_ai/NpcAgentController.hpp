#pragma once

#include "core/math/Vec2.hpp"
#include "modules/npc_ai/AwarenessSensor.hpp"
#include "modules/npc_ai/NpcAgentState.hpp"
#include "modules/npc_ai/NpcBrainTick.hpp"
#include "modules/npc_ai/NpcIntentSelector.hpp"
#include "scene/level/LevelTileMap.hpp"

namespace iggy::npc_ai {

struct NpcAgentTickConfig {
	float maxDistance = 0.0F;
	AwarenessSensorConfig awareness;
	NpcIntentSelectorConfig intent;
};

struct NpcAgentTickResult {
	NpcAgentState state;
	NpcBrainTickResult brain;
};

class NpcAgentController {
public:
	[[nodiscard]] NpcAgentTickResult tick(const LevelTileMap &map, const NpcAgentState &state, Vec2 playerPosition, const NpcAgentTickConfig &config) const;
};

} // namespace iggy::npc_ai
