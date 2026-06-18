#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "modules/npc_ai/NpcAgentTickConfig.hpp"
#include "runtime/RuntimeCommandQueue.hpp"
#include "runtime/RuntimeGameplayState.hpp"
#include "runtime/RuntimePlayerCommandExecutionStep.hpp"
#include "runtime/RuntimePolicyGameplayFrameReport.hpp"
#include "runtime/RuntimePolicyGameplayFrameStep.hpp"
#include "scene/interaction/InteractionReach2D.hpp"
#include "scene/inventory/InventoryEvent2D.hpp"
#include "scene/inventory/ItemDefinition2D.hpp"
#include "scene/player/PlayerInputIntent2D.hpp"
#include "scene/player/PlayerInputIntentGate2D.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

struct RuntimePolicyGameplayFrameRunnerFrame {
	PlayerInputContext2D playerInputContext;
	ResourceId actorId;
	std::vector<PlayerInputIntent2D> playerIntents;
	RuntimeCommandQueueConfig commandQueueConfig;
	Vec2 fallbackPlayerPosition;
	RuntimePlayerCommandExecutionConfig playerCommandConfig;
	npc_ai::NpcAgentTickConfig npcConfig;
	InteractionReach2DConfig interactionReach;
	RuntimePolicyPickupConfig policyPickupConfig;
	std::vector<NpcActorMovementFrameApply2DRequest> npcMovementRequests;
};

struct RuntimePolicyGameplayFrameRunnerInput {
	RuntimeGameplayState initialState;
	std::vector<RuntimePolicyGameplayFrameRunnerFrame> frames;
	ItemDefinition2DCatalog itemDefinitions;
};

struct RuntimePolicyGameplayFrameRunnerTick {
	RuntimePolicyGameplayFrameResult frame;
	RuntimePolicyGameplayFrameReport report;
};

struct RuntimePolicyGameplayFrameRunnerResult {
	RuntimeGameplayState finalState;
	std::vector<RuntimePolicyGameplayFrameRunnerTick> ticks;
	InventoryEventRecorder2D inventoryEvents;
	std::size_t npcMovedCount = 0;
	std::size_t npcBlockedMovementCount = 0;
	std::size_t npcRejectedMovementCount = 0;
	std::size_t npcMissingActorMovementCount = 0;
	std::size_t npcMovementDirtyTileCount = 0;
	bool npcMovementNeedsOccupancyRebuild = false;
	bool npcMovementNeedsAiMapQueryRefresh = false;
	bool npcMovementNeedsInteractionRefresh = false;
	bool npcMovementNeedsRenderRefresh = false;
	bool npcMovementNeedsVisibilityRefresh = false;
};

class RuntimePolicyGameplayFrameRunner {
public:
	[[nodiscard]] RuntimePolicyGameplayFrameRunnerResult run(const RuntimePolicyGameplayFrameRunnerInput &input) const;

	[[nodiscard]] RuntimePolicyGameplayFrameRunnerResult run(
		const RuntimePolicyGameplayFrameRunnerInput &input,
		const physics2d::CollisionWorld2D &explicitWorld) const;
};

} // namespace iggy::runtime
