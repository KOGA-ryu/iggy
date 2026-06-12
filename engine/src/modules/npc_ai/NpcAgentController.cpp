#include "modules/npc_ai/NpcAgentController.hpp"

namespace iggy::npc_ai {

NpcAgentTickResult NpcAgentController::tick(const LevelTileMap &map, const NpcAgentState &state, Vec2 playerPosition, const NpcAgentTickConfig &config) const
{
	NpcBrainTickInput input;
	input.npcPosition = state.position;
	input.playerPosition = playerPosition;
	input.homeTile = state.homeTile;
	input.awarenessState = state.awareness;
	input.followState = state.followState;
	input.maxDistance = config.maxDistance;
	input.awarenessConfig = config.awareness;
	input.intentConfig = config.intent;

	const NpcBrainTickResult brain = NpcBrainTick {}.tick(map, input);
	NpcAgentState nextState;
	nextState.position = brain.nextPosition;
	nextState.homeTile = state.homeTile;
	nextState.awareness = brain.awarenessState;
	nextState.followState = brain.followState;

	return { nextState, brain };
}

} // namespace iggy::npc_ai
