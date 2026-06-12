#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "modules/npc_ai/NpcAgentBatch.hpp"
#include "modules/npc_ai/NpcAgentController.hpp"
#include "scene/level/LevelTileMap.hpp"

namespace iggy::npc_ai {

class NpcAgentBatchUpdater {
public:
	[[nodiscard]] NpcAgentBatchUpdateResult update(const LevelTileMap &map, const std::vector<NpcAgentEntry> &agents, Vec2 playerPosition, const NpcAgentTickConfig &config) const;
};

} // namespace iggy::npc_ai
