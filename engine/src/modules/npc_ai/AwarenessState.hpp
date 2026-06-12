#pragma once

#include "core/math/Vec2.hpp"
#include "modules/line_of_sight/LineOfSight.hpp"

namespace iggy::npc_ai {

enum class AwarenessEventType {
	None,
	PlayerSeen,
	PlayerLost,
	AlertExpired,
};

struct AwarenessState {
	bool playerVisible = false;
	bool alerted = false;
	int alertTicksRemaining = 0;
	line_of_sight::TileCoord lastSeenTile;
	Vec2 lastSeenPosition;
};

struct AwarenessEvent {
	AwarenessEventType type = AwarenessEventType::None;
	bool playerVisible = false;
	bool alerted = false;
	line_of_sight::TileCoord playerTile;
	line_of_sight::TileCoord lastSeenTile;
};

} // namespace iggy::npc_ai
