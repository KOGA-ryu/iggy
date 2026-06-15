#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayOrchestratedFrameRunner.hpp"
#include "runtime/RuntimeGameplayOrchestratedFrameRunnerReport.hpp"

namespace iggy::runtime {

struct RuntimeGameplayScenarioFrame {
	RuntimeGameplayOrchestratedFrameRunnerFrame frame;
};

struct RuntimeGameplayScenario {
	RuntimeGameplayState initialState;
	std::vector<RuntimeGameplayScenarioFrame> frames;
};

struct RuntimeGameplayScenarioResult {
	RuntimeGameplayScenario scenario;
	RuntimeGameplayOrchestratedFrameRunnerResult runner;
	RuntimeGameplayOrchestratedFrameRunnerReport report;
	RuntimeGameplayState state;
	std::size_t frameCount = 0;
	std::size_t changedFrameCount = 0;
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

class RuntimeGameplayScenarioRunner {
public:
	[[nodiscard]] RuntimeGameplayScenarioResult run(const RuntimeGameplayScenario &scenario) const;
};

} // namespace iggy::runtime
