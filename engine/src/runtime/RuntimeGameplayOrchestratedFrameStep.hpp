#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayFrameStep.hpp"
#include "runtime/RuntimeNpcAiMovementRefreshFrameStep.hpp"
#include "scene/ai/AiMap2D.hpp"
#include "scene/ai/NpcMapPlayControlFramePlan.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "scene/npc/NpcActorMovementFramePlan2D.hpp"
#include "scene/npc/NpcActorMovementRefreshFrame2D.hpp"
#include "scene/npc/NpcActorOccupancy2D.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayOrchestratedFrameStatus {
	Ran,
	NoChanges,
};

struct RuntimeGameplayOrchestratedFrameInput {
	RuntimeGameplayFrameInput playerFrame;
	std::vector<NpcMapPlayControlFramePlanSubject> subjects;
	NpcMapPlayControlFramePlanPools pools;
	AiMap2D aiMap;
	NpcMapPlayControlFramePlanConfig controlConfig;
	LevelTileMap movementMap;
	NpcActorMovementFramePlan2DConfig movementConfig;
	NpcActorOccupancy2D previousOccupancy;
	InteractionTarget2DRegistry interactionTargets;
	AiMap2D refreshAiMap;
	NpcActorMovementRefreshFrame2DConfig refreshConfig;
};

struct RuntimeGameplayOrchestratedFrameResult {
	RuntimeGameplayOrchestratedFrameStatus status =
		RuntimeGameplayOrchestratedFrameStatus::NoChanges;
	RuntimeGameplayOrchestratedFrameInput input;
	RuntimeGameplayFrameResult playerFrame;
	RuntimeNpcAiMovementRefreshFrameResult npcFrame;
	RuntimeGameplayState state;
	InventoryEventRecorder2D inventoryEvents;
	std::size_t acceptedCommandCount = 0;
	std::size_t blockedIntentCount = 0;
	std::size_t rejectedIntentCount = 0;
	std::size_t pickedUpCount = 0;
	std::size_t inventoryEventCount = 0;
	std::size_t npcControlPlannedRequestCount = 0;
	std::size_t npcControlAppliedCount = 0;
	std::size_t npcControlFailedCount = 0;
	std::size_t npcMovementPlannedRequestCount = 0;
	std::size_t npcMovedCount = 0;
	std::size_t npcBlockedMovementCount = 0;
	std::size_t npcRejectedMovementCount = 0;
	std::size_t npcMissingActorMovementCount = 0;
	std::size_t npcRefreshDirtyTileCount = 0;
	bool playerFrameRan = false;
	bool interactionChanged = false;
	bool inventoryChanged = false;
	bool npcControlsChanged = false;
	bool npcActorsChanged = false;
	bool npcOccupancyRefreshed = false;
	bool npcInteractionRefreshed = false;
	bool npcAiMapRefreshed = false;
	bool npcRenderRefreshed = false;
	bool npcVisibilityRefreshed = false;

	[[nodiscard]] bool changedNpcState() const;
	[[nodiscard]] bool refreshedNpcData() const;
	[[nodiscard]] bool changedGameplayState() const;
};

class RuntimeGameplayOrchestratedFrameStep {
public:
	[[nodiscard]] RuntimeGameplayOrchestratedFrameResult run(
		const RuntimeGameplayOrchestratedFrameInput &input) const;

	[[nodiscard]] RuntimeGameplayOrchestratedFrameResult run(
		const RuntimeGameplayOrchestratedFrameInput &input,
		const physics2d::CollisionWorld2D &explicitWorld) const;
};

} // namespace iggy::runtime
