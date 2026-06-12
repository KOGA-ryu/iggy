#pragma once

#include "modules/line_of_sight/LineOfSight.hpp"

namespace iggy::npc_ai {

enum class NpcIntentType {
	Idle,
	InvestigateLastSeen,
	PursueVisibleTarget,
	ReturnToPost,
};

struct NpcIntent {
	NpcIntentType type = NpcIntentType::Idle;
	line_of_sight::TileCoord targetTile;
};

} // namespace iggy::npc_ai
