#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "modules/npc_ai/NpcAgentBatch.hpp"
#include "modules/npc_ai/NpcAgentTickConfig.hpp"
#include "scene/level/LevelTileMap.hpp"

namespace iggy {

struct LevelNpcUpdateResult {
	std::vector<npc_ai::NpcAgentEntry> npcAgents;
	std::vector<npc_ai::NpcAgentTickReportEntry> reports;
};

class LevelNpcUpdateStep {
public:
	[[nodiscard]] LevelNpcUpdateResult update(const LevelTileMap &map, const std::vector<npc_ai::NpcAgentEntry> &npcAgents, Vec2 playerPosition, const npc_ai::NpcAgentTickConfig &config) const;
};

} // namespace iggy
