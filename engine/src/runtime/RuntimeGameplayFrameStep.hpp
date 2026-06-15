#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "modules/npc_ai/NpcAgentController.hpp"
#include "runtime/RuntimeCommandQueue.hpp"
#include "runtime/RuntimeGameplayState.hpp"
#include "runtime/RuntimeNpcActorMovementFrameStep.hpp"
#include "runtime/RuntimePickupStep.hpp"
#include "runtime/RuntimePlayerCommandExecutionStep.hpp"
#include "runtime/RuntimePlayerInputInteractionPickupFrameReport.hpp"
#include "runtime/RuntimePlayerInputInteractionPickupFrameReporter.hpp"
#include "runtime/RuntimePlayerInputInteractionPickupFrameStep.hpp"
#include "scene/inventory/InventoryEvent2D.hpp"
#include "scene/interaction/InteractionReach2D.hpp"
#include "scene/player/PlayerInputIntent2D.hpp"
#include "scene/player/PlayerInputIntentGate2D.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

struct RuntimeGameplayFrameInput {
	RuntimeGameplayState state;
	PlayerInputContext2D playerInputContext;
	ResourceId actorId;
	std::vector<PlayerInputIntent2D> playerIntents;
	RuntimeCommandQueueConfig commandQueueConfig;
	Vec2 fallbackPlayerPosition;
	RuntimePlayerCommandExecutionConfig playerCommandConfig;
	npc_ai::NpcAgentTickConfig npcConfig;
	InteractionReach2DConfig interactionReach;
	RuntimePickupConfig pickup;
	std::vector<NpcActorMovementFrameApply2DRequest> npcMovementRequests;
};

struct RuntimeGameplayFrameResult {
	RuntimeGameplayState state;
	RuntimePlayerInputInteractionPickupFrameResult frame;
	RuntimePlayerInputInteractionPickupFrameReport report;
	InventoryEventRecorder2D inventoryEvents;
	RuntimeNpcActorMovementFrameResult npcMovement;
};

class RuntimeGameplayFrameStep {
public:
	[[nodiscard]] RuntimeGameplayFrameResult run(const RuntimeGameplayFrameInput &input) const;

	[[nodiscard]] RuntimeGameplayFrameResult run(
		const RuntimeGameplayFrameInput &input,
		const physics2d::CollisionWorld2D &explicitWorld) const;
};

} // namespace iggy::runtime
