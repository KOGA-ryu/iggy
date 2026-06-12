#pragma once

#include "modules/npc_ai/NpcAgentController.hpp"
#include "modules/npc_ai/NpcAgentState.hpp"
#include "modules/npc_ai/NpcTickReport.hpp"

namespace iggy::npc_ai {

class NpcTickReporter {
public:
	[[nodiscard]] NpcTickReport report(const NpcAgentState &previousState, const NpcAgentTickResult &tickResult) const;
};

} // namespace iggy::npc_ai
