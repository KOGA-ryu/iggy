#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayOrchestratedFrameReport.hpp"
#include "runtime/RuntimeGameplayOrchestratedFrameRunner.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayOrchestratedFrameRunnerEvent {
	FrameChangedObserved,
	InventoryEventObserved,
	NpcControlChanged,
	NpcActorMoved,
	NpcActorBlocked,
	NpcActorRejected,
	NpcActorMissing,
	RefreshOccupancy,
	RefreshInteraction,
	RefreshAiMap,
	RefreshRender,
	RefreshVisibility,
	RunnerChanged,
	RunnerUnchanged,
};

struct RuntimeGameplayOrchestratedFrameRunnerReport {
	RuntimeGameplayOrchestratedFrameRunnerResult result;
	std::vector<RuntimeGameplayOrchestratedFrameReport> frames;
	InventoryEventRecorder2D inventoryEvents;
	std::vector<RuntimeGameplayOrchestratedFrameRunnerEvent> events;
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

	[[nodiscard]] bool changed() const;
	[[nodiscard]] bool refreshedNpcData() const;
	[[nodiscard]] bool hasEvents() const;
};

class RuntimeGameplayOrchestratedFrameRunnerReporter {
public:
	[[nodiscard]] RuntimeGameplayOrchestratedFrameRunnerReport report(
		const RuntimeGameplayOrchestratedFrameRunnerResult &result) const;
};

} // namespace iggy::runtime
