#pragma once

#include <vector>

#include "modules/npc_ai/NpcAgentBatch.hpp"
#include "scene/level/LevelTileMap.hpp"

namespace iggy {

struct LevelRuntimeState {
	LevelTileMap map;
	std::vector<npc_ai::NpcAgentEntry> npcAgents;
};

} // namespace iggy
