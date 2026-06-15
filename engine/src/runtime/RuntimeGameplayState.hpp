#pragma once

#include "runtime/RuntimeCommandQueue.hpp"
#include "runtime/RuntimeInteractionState.hpp"
#include "runtime/RuntimeInventoryState.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/npc/NpcActorControlState2D.hpp"
#include "scene/npc/NpcActorState2D.hpp"

namespace iggy::runtime {

struct RuntimeGameplayState {
	RuntimeSessionState session;
	RuntimeCommandQueueState commandQueue;
	RuntimeInteractionState interaction;
	RuntimeInventoryState inventory;
	NpcActorState2DRegistry npcActors;
	NpcActorControlState2DRegistry npcControls;
};

} // namespace iggy::runtime
