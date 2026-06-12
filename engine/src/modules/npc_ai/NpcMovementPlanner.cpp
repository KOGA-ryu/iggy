#include "modules/npc_ai/NpcMovementPlanner.hpp"

namespace iggy::npc_ai {

namespace {

bool IsValidTile(TileCoord tile)
{
	return tile.x >= 0 && tile.y >= 0;
}

NpcMovementPlan MoveToTile(TileCoord tile)
{
	if (!IsValidTile(tile))
		return {};
	return { NpcMovementPlanType::MoveTo, tileCenter(tile) };
}

} // namespace

NpcMovementPlan NpcMovementPlanner::plan(const NpcIntent &intent, TileCoord homeTile) const
{
	switch (intent.type) {
	case NpcIntentType::Idle:
		return {};
	case NpcIntentType::PursueVisibleTarget:
	case NpcIntentType::InvestigateLastSeen:
		return MoveToTile(intent.targetTile);
	case NpcIntentType::ReturnToPost:
		return MoveToTile(homeTile);
	}

	return {};
}

} // namespace iggy::npc_ai
