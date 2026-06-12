#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Vec2.hpp"
#include "modules/npc_ai/NpcAgentController.hpp"
#include "modules/npc_ai/NpcAgentBatch.hpp"
#include "runtime/RuntimeTick.hpp"
#include "scene/level/LevelRuntimeState.hpp"

namespace iggy::runtime {

struct RuntimeTickRunInput {
	LevelRuntimeState initialState;
	Vec2 playerPosition;
	npc_ai::NpcAgentTickConfig npcConfig;
	std::size_t tickCount = 0;
};

struct RuntimeTickRunResult {
	LevelRuntimeState finalState;
	std::vector<std::vector<npc_ai::NpcAgentTickReportEntry>> reportsByTick;
};

class RuntimeTickRunner {
public:
	[[nodiscard]] RuntimeTickRunResult run(const RuntimeTickRunInput &input) const;
};

} // namespace iggy::runtime
