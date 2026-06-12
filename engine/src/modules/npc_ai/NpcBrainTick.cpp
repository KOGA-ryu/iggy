#include "modules/npc_ai/NpcBrainTick.hpp"

#include "modules/npc_ai/NpcMovementPlanner.hpp"

namespace iggy::npc_ai {

NpcBrainTickResult NpcBrainTick::tick(const LevelTileMap &map, const NpcBrainTickInput &input) const
{
	AwarenessState awareness = input.awarenessState;
	const AwarenessEvent awarenessEvent = AwarenessSensor { input.awarenessConfig }.observe(map, input.npcPosition, input.playerPosition, awareness);
	const NpcIntent intent = NpcIntentSelector { input.intentConfig }.select(awareness);
	const NpcMovementPlan movementPlan = NpcMovementPlanner {}.plan(intent, input.homeTile);
	const NpcNavigationResult navigation = NpcNavigationController {}.step(map, input.npcPosition, movementPlan, input.followState, input.maxDistance);

	return {
		awarenessEvent,
		awareness,
		intent,
		movementPlan,
		navigation,
		navigation.nextPosition,
		navigation.followState,
	};
}

} // namespace iggy::npc_ai
