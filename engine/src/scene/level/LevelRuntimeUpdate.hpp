#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "modules/npc_ai/NpcAgentBatch.hpp"
#include "modules/npc_ai/NpcAgentTickConfig.hpp"
#include "scene/level/LevelRuntimeState.hpp"

namespace iggy {

struct LevelRuntimeUpdateResult {
	LevelRuntimeState state;
	std::vector<npc_ai::NpcAgentTickReportEntry> npcReports;
};

class LevelRuntimeUpdate {
public:
	[[nodiscard]] LevelRuntimeUpdateResult updateNpcAgents(const LevelRuntimeState &state, Vec2 playerPosition, const npc_ai::NpcAgentTickConfig &config) const;
};

} // namespace iggy
