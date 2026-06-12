#pragma once

#include <vector>

#include "core/resource/ResourceId.hpp"
#include "modules/npc_ai/NpcAgentState.hpp"
#include "modules/npc_ai/NpcTickReport.hpp"

namespace iggy::npc_ai {

struct NpcAgentEntry {
	ResourceId id;
	NpcAgentState state;
};

struct NpcAgentTickReportEntry {
	ResourceId id;
	NpcTickReport report;
};

struct NpcAgentBatchUpdateResult {
	std::vector<NpcAgentEntry> agents;
	std::vector<NpcAgentTickReportEntry> reports;
};

} // namespace iggy::npc_ai
