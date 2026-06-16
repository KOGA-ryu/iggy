#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayOrchestratedFrameStep.hpp"

namespace iggy::runtime {

struct RuntimeGameplayOrchestratedFrameRunnerFrame {
	RuntimeGameplayFrameInput playerFrame;
	std::vector<NpcMapPlayControlFramePlanSubject> subjects;
	NpcMapPlayControlFramePlanPools pools;
	AiMap2D aiMap;
	NpcMapPlayControlFramePlanConfig controlConfig;
	std::vector<NpcActorControlState2D> controlOverrides;
	LevelTileMap movementMap;
	NpcActorMovementFramePlan2DConfig movementConfig;
	NpcActorOccupancy2D previousOccupancy;
	InteractionTarget2DRegistry interactionTargets;
	AiMap2D refreshAiMap;
	NpcActorMovementRefreshFrame2DConfig refreshConfig;
};

struct RuntimeGameplayOrchestratedFrameRunnerInput {
	RuntimeGameplayState initialState;
	std::vector<RuntimeGameplayOrchestratedFrameRunnerFrame> frames;
};

struct RuntimeGameplayOrchestratedFrameRunnerResult {
	RuntimeGameplayState initialState;
	std::vector<RuntimeGameplayOrchestratedFrameRunnerFrame> frames;
	RuntimeGameplayState state;
	std::vector<RuntimeGameplayOrchestratedFrameResult> frameResults;
	InventoryEventRecorder2D inventoryEvents;
	std::size_t frameCount = 0;
	std::size_t changedFrameCount = 0;
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
	bool interactionChanged = false;
	bool inventoryChanged = false;
	bool npcControlsChanged = false;
	bool npcActorsChanged = false;
	bool npcOccupancyRefreshed = false;
	bool npcInteractionRefreshed = false;
	bool npcAiMapRefreshed = false;
	bool npcRenderRefreshed = false;
	bool npcVisibilityRefreshed = false;

	[[nodiscard]] bool hasFrames() const;
	[[nodiscard]] bool changed() const;
	[[nodiscard]] bool refreshedNpcData() const;
};

class RuntimeGameplayOrchestratedFrameRunner {
public:
	[[nodiscard]] RuntimeGameplayOrchestratedFrameRunnerResult run(
		const RuntimeGameplayOrchestratedFrameRunnerInput &input) const;
};

} // namespace iggy::runtime
