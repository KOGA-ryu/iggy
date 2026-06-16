#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayProfileScenarioValidator.hpp"
#include "runtime/RuntimeGameplayScenarioRunner.hpp"
#include "scene/ai/NpcMapPlayControlExplainLedger.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayScenarioLedgerStatus {
	Empty,
	Reported,
};

enum class RuntimeGameplayScenarioLedgerEvent {
	ScenarioStarted,
	FrameChanged,
	FrameUnchanged,
	InventoryChanged,
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
	ScenarioChanged,
	ScenarioUnchanged,
};

struct RuntimeGameplayScenarioLedgerFrame {
	std::size_t frameIndex = 0;
	bool changed = false;
	std::size_t inventoryEventCount = 0;
	std::size_t npcControlPlannedRequestCount = 0;
	std::size_t npcControlAppliedCount = 0;
	std::size_t npcControlFailedCount = 0;
	bool npcControlsChanged = false;
	std::size_t npcMovementPlannedRequestCount = 0;
	std::size_t npcMovedCount = 0;
	std::size_t npcBlockedMovementCount = 0;
	std::size_t npcRejectedMovementCount = 0;
	std::size_t npcMissingActorMovementCount = 0;
	bool npcActorsChanged = false;
	std::size_t npcRefreshDirtyTileCount = 0;
	bool npcOccupancyRefreshed = false;
	bool npcInteractionRefreshed = false;
	bool npcAiMapRefreshed = false;
	bool npcRenderRefreshed = false;
	bool npcVisibilityRefreshed = false;
};

struct RuntimeGameplayScenarioLedger {
	RuntimeGameplayScenarioResult result;
	RuntimeGameplayScenarioLedgerStatus status = RuntimeGameplayScenarioLedgerStatus::Empty;
	bool hasProfileBuild = false;
	bool hasProfileValidation = false;
	RuntimeGameplayProfileScenarioDefinitionBuildResult profileBuild;
	RuntimeGameplayProfileScenarioValidationResult profileValidation;
	std::vector<NpcMapPlayControlExplainLedger> npcAiExplanations;
	std::vector<RuntimeGameplayScenarioLedgerFrame> frames;
	std::vector<RuntimeGameplayScenarioLedgerEvent> events;
	std::size_t frameCount = 0;
	std::size_t changedFrameCount = 0;
	std::size_t inventoryEventCount = 0;
	std::size_t npcControlPlannedRequestCount = 0;
	std::size_t npcControlAppliedCount = 0;
	std::size_t npcControlFailedCount = 0;
	bool npcControlsChanged = false;
	std::size_t npcMovementPlannedRequestCount = 0;
	std::size_t npcMovedCount = 0;
	std::size_t npcBlockedMovementCount = 0;
	std::size_t npcRejectedMovementCount = 0;
	std::size_t npcMissingActorMovementCount = 0;
	bool npcActorsChanged = false;
	std::size_t npcRefreshDirtyTileCount = 0;
	bool npcOccupancyRefreshed = false;
	bool npcInteractionRefreshed = false;
	bool npcAiMapRefreshed = false;
	bool npcRenderRefreshed = false;
	bool npcVisibilityRefreshed = false;
	std::size_t finalNpcActorCount = 0;
	std::size_t finalNpcControlCount = 0;
	std::size_t finalPresentNpcCount = 0;
	std::size_t finalInventoryStackCount = 0;
	std::size_t finalInventoryDropCount = 0;
	std::size_t npcAiExplainFrameCount = 0;
	std::size_t npcAiProfileResolvedCount = 0;
	std::size_t npcAiProfileMissingCount = 0;
	std::size_t npcAiHandDrawnCount = 0;
	std::size_t npcAiMapChangedSelectionCount = 0;
	std::size_t npcAiPlayKeptCount = 0;
	std::size_t npcAiPlayFoldedCount = 0;
	std::size_t npcAiControlProposedCount = 0;
	std::size_t npcAiControlFailedCount = 0;
	std::size_t npcAiControlAppliedCount = 0;
	std::size_t npcAiControlApplyFailedCount = 0;

	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool changed() const;
	[[nodiscard]] bool refreshedNpcData() const;
	[[nodiscard]] bool hasEvents() const;
	[[nodiscard]] bool hasNpcAiExplanations() const;
};

class RuntimeGameplayScenarioLedgerReporter {
public:
	[[nodiscard]] RuntimeGameplayScenarioLedger report(
		const RuntimeGameplayScenarioResult &result) const;

	[[nodiscard]] RuntimeGameplayScenarioLedger report(
		const RuntimeGameplayProfileScenarioDefinitionBuildResult &profileBuild,
		const RuntimeGameplayScenarioResult &result) const;

	[[nodiscard]] RuntimeGameplayScenarioLedger report(
		const RuntimeGameplayProfileScenarioValidationResult &profileValidation,
		const RuntimeGameplayScenarioResult &result) const;
};

} // namespace iggy::runtime
