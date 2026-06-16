#pragma once

#include <cstddef>

#include "runtime/RuntimeGameplayScenarioLedger.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayProfileScenarioRunStatus {
	Ran,
	ValidationFailed,
};

struct RuntimeGameplayProfileScenarioRunResult {
	RuntimeGameplayProfileScenarioDefinition definition;
	RuntimeGameplayProfileScenarioValidationResult validation;
	RuntimeGameplayProfileScenarioDefinitionBuildResult build;
	RuntimeGameplayScenarioResult scenario;
	RuntimeGameplayScenarioLedger ledger;
	RuntimeGameplayState state;
	RuntimeGameplayProfileScenarioRunStatus status =
		RuntimeGameplayProfileScenarioRunStatus::ValidationFailed;
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

	[[nodiscard]] bool ran() const;
	[[nodiscard]] bool changed() const;
	[[nodiscard]] bool refreshedNpcData() const;
};

class RuntimeGameplayProfileScenarioRunner {
public:
	[[nodiscard]] RuntimeGameplayProfileScenarioRunResult run(
		const RuntimeGameplayProfileScenarioDefinition &definition) const;
};

} // namespace iggy::runtime
