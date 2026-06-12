#pragma once

namespace iggy::npc_ai {

enum class NpcTickEventType {
	TargetSeen,
	TargetLost,
	AlertExpired,
	IntentSelected,
	NavigationAdvanced,
	NavigationArrived,
	NavigationFailed,
	NoMovement,
	PositionChanged,
};

struct NpcTickEvent {
	NpcTickEventType type;
};

} // namespace iggy::npc_ai
