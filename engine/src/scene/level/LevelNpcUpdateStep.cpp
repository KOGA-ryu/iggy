#include "scene/level/LevelNpcUpdateStep.hpp"

#include "modules/npc_ai/NpcAgentBatchUpdater.hpp"

namespace iggy {

LevelNpcUpdateResult LevelNpcUpdateStep::update(const LevelTileMap &map, const std::vector<npc_ai::NpcAgentEntry> &npcAgents, Vec2 playerPosition, const npc_ai::NpcAgentTickConfig &config) const
{
	const npc_ai::NpcAgentBatchUpdateResult batchResult = npc_ai::NpcAgentBatchUpdater {}.update(map, npcAgents, playerPosition, config);
	return { batchResult.agents, batchResult.reports };
}

} // namespace iggy
