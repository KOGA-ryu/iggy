#pragma once

#include "modules/npc_ai/AwarenessSensor.hpp"
#include "modules/npc_ai/NpcIntentSelector.hpp"

namespace iggy::npc_ai {

struct NpcAgentTickConfig {
	float maxDistance = 0.0F;
	AwarenessSensorConfig awareness;
	NpcIntentSelectorConfig intent;
};

} // namespace iggy::npc_ai
