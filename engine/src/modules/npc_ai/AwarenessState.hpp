#pragma once

#include "core/math/Vec2.hpp"
#include "scene/level/TileCoord.hpp"

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
	TileCoord lastSeenTile;
	Vec2 lastSeenPosition;
};

struct AwarenessEvent {
	AwarenessEventType type = AwarenessEventType::None;
	bool playerVisible = false;
	bool alerted = false;
	TileCoord playerTile;
	TileCoord lastSeenTile;
};

} // namespace iggy::npc_ai
