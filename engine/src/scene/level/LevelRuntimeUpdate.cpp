#include "scene/level/LevelRuntimeUpdate.hpp"

#include "scene/level/LevelNpcUpdateStep.hpp"

namespace iggy {

LevelRuntimeUpdateResult LevelRuntimeUpdate::updateNpcAgents(const LevelRuntimeState &state, Vec2 playerPosition, const npc_ai::NpcAgentTickConfig &config) const
{
	const LevelNpcUpdateResult npcUpdate = LevelNpcUpdateStep {}.update(state.map, state.npcAgents, playerPosition, config);

	LevelRuntimeState nextState;
	nextState.map = state.map;
	nextState.npcAgents = npcUpdate.npcAgents;
	return { nextState, npcUpdate.reports };
}

} // namespace iggy
