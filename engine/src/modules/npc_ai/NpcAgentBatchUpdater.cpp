#include "modules/npc_ai/NpcAgentBatchUpdater.hpp"

#include "modules/npc_ai/NpcTickReporter.hpp"

namespace iggy::npc_ai {

NpcAgentBatchUpdateResult NpcAgentBatchUpdater::update(const LevelTileMap &map, const std::vector<NpcAgentEntry> &agents, Vec2 playerPosition, const NpcAgentTickConfig &config) const
{
	NpcAgentBatchUpdateResult result;
	result.agents.reserve(agents.size());
	result.reports.reserve(agents.size());

	for (const NpcAgentEntry &agent : agents) {
		const NpcAgentTickResult tick = NpcAgentController {}.tick(map, agent.state, playerPosition, config);
		result.agents.push_back({ agent.id, tick.state });
		result.reports.push_back({ agent.id, NpcTickReporter {}.report(agent.state, tick) });
	}

	return result;
}

} // namespace iggy::npc_ai
