#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "modules/npc_ai/AwarenessState.hpp"
#include "modules/npc_ai/NpcAgentController.hpp"
#include "modules/npc_ai/NpcIntent.hpp"
#include "modules/npc_ai/NpcMovementPlan.hpp"
#include "modules/npc_ai/NpcNavigationController.hpp"
#include "modules/npc_ai/NpcTickEvent.hpp"

namespace iggy::npc_ai {

struct NpcTickReport {
	Vec2 previousPosition;
	Vec2 nextPosition;
	AwarenessEvent awarenessEvent;
	NpcIntent intent;
	NpcMovementPlan movementPlan;
	NpcNavigationResult navigation;
	std::vector<NpcTickEvent> events;
};

} // namespace iggy::npc_ai
