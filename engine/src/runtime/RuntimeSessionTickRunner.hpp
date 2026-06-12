#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Vec2.hpp"
#include "modules/npc_ai/NpcAgentBatch.hpp"
#include "modules/npc_ai/NpcAgentController.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "runtime/RuntimeSessionTick.hpp"

namespace iggy::runtime {

struct RuntimeSessionTickRunInput {
	RuntimeSessionState initialSession;
	Vec2 playerPosition;
	npc_ai::NpcAgentTickConfig npcConfig;
	std::size_t tickCount = 0;
};

struct RuntimeSessionTickRunResult {
	RuntimeSessionState finalSession;
	std::vector<std::vector<npc_ai::NpcAgentTickReportEntry>> reportsByTick;
};

class RuntimeSessionTickRunner {
public:
	[[nodiscard]] RuntimeSessionTickRunResult run(const RuntimeSessionTickRunInput &input) const;
};

} // namespace iggy::runtime
