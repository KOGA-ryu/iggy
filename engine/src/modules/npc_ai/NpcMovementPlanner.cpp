#include "modules/npc_ai/NpcMovementPlanner.hpp"

namespace iggy::npc_ai {

namespace {

bool IsValidTile(line_of_sight::TileCoord tile)
{
	return tile.x >= 0 && tile.y >= 0;
}

Vec2 TileCenter(line_of_sight::TileCoord tile)
{
	return { static_cast<float>(tile.x) + 0.5F, static_cast<float>(tile.y) + 0.5F };
}

NpcMovementPlan MoveToTile(line_of_sight::TileCoord tile)
{
	if (!IsValidTile(tile))
		return {};
	return { NpcMovementPlanType::MoveTo, TileCenter(tile) };
}

} // namespace

NpcMovementPlan NpcMovementPlanner::plan(const NpcIntent &intent, line_of_sight::TileCoord homeTile) const
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
