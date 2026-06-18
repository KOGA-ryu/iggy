#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "modules/npc_ai/NpcAgentBatch.hpp"
#include "modules/npc_ai/NpcAgentTickConfig.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "runtime/RuntimeTick.hpp"

namespace iggy::runtime {

struct RuntimeSessionTickInput {
	RuntimeSessionState session;
	Vec2 playerPosition;
	npc_ai::NpcAgentTickConfig npcConfig;
};

struct RuntimeSessionTickResult {
	RuntimeSessionState session;
	std::vector<npc_ai::NpcAgentTickReportEntry> npcReports;
};

class RuntimeSessionTick {
public:
	[[nodiscard]] RuntimeSessionTickResult run(const RuntimeSessionTickInput &input) const;
};

} // namespace iggy::runtime
