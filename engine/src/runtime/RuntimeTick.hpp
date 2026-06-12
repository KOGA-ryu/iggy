#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "modules/npc_ai/NpcAgentController.hpp"
#include "modules/npc_ai/NpcAgentBatch.hpp"
#include "scene/level/LevelRuntimeState.hpp"

namespace iggy::runtime {

struct RuntimeTickInput {
	LevelRuntimeState state;
	Vec2 playerPosition;
	npc_ai::NpcAgentTickConfig npcConfig;
};

struct RuntimeTickResult {
	LevelRuntimeState state;
	std::vector<npc_ai::NpcAgentTickReportEntry> npcReports;
};

class RuntimeTick {
public:
	[[nodiscard]] RuntimeTickResult run(const RuntimeTickInput &input) const;
};

} // namespace iggy::runtime
