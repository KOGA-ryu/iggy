#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayFrameReport.hpp"
#include "runtime/RuntimeGameplayFrameReporter.hpp"
#include "runtime/RuntimeGameplayOrchestratedFrameStep.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayOrchestratedFrameEvent {
	PlayerFrameRan,
	InventoryEventObserved,
	NpcControlChanged,
	NpcControlUnchanged,
	NpcActorMoved,
	NpcActorBlocked,
	NpcActorRejected,
	NpcActorMissing,
	RefreshOccupancy,
	RefreshInteraction,
	RefreshAiMap,
	RefreshRender,
	RefreshVisibility,
	FrameChanged,
	FrameUnchanged,
};

struct RuntimeGameplayOrchestratedFrameReport {
	RuntimeGameplayOrchestratedFrameResult result;
	RuntimeGameplayFrameReport player;
	InventoryEventRecorder2D inventoryEvents;
	std::vector<RuntimeGameplayOrchestratedFrameEvent> events;
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
	bool playerChanged = false;
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

class RuntimeGameplayOrchestratedFrameReporter {
public:
	[[nodiscard]] RuntimeGameplayOrchestratedFrameReport report(
		const RuntimeGameplayOrchestratedFrameResult &result) const;
};

} // namespace iggy::runtime
