#pragma once

#include "scene/level/TileCoord.hpp"

namespace iggy::npc_ai {

enum class NpcIntentType {
	Idle,
	InvestigateLastSeen,
	PursueVisibleTarget,
	ReturnToPost,
};

struct NpcIntent {
	NpcIntentType type = NpcIntentType::Idle;
	TileCoord targetTile;
};

} // namespace iggy::npc_ai
