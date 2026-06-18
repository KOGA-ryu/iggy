#pragma once

#include <cstddef>

#include "runtime/RuntimeGameplayOrchestratedFrameReport.hpp"
#include "runtime/RuntimeGameplayOrchestratedFrameRunnerReport.hpp"
#include "runtime/RuntimeGameplayScenarioLedger.hpp"
#include "runtime/RuntimeGameplayScenarioRunner.hpp"

namespace iggy::runtime {

struct RuntimeGameplayScenarioNpcReportProjection {
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

[[nodiscard]] RuntimeGameplayScenarioNpcReportProjection projectRuntimeGameplayScenarioNpcReport(
	const RuntimeGameplayOrchestratedFrameReport &report);

[[nodiscard]] RuntimeGameplayScenarioNpcReportProjection projectRuntimeGameplayScenarioNpcReport(
	const RuntimeGameplayOrchestratedFrameRunnerReport &report);

void applyRuntimeGameplayScenarioResultProjection(
	RuntimeGameplayScenarioResult &result,
	const RuntimeGameplayOrchestratedFrameRunnerReport &report);

[[nodiscard]] RuntimeGameplayScenarioLedgerFrame projectRuntimeGameplayScenarioLedgerFrame(
	std::size_t frameIndex,
	const RuntimeGameplayOrchestratedFrameReport &report);

void applyRuntimeGameplayScenarioLedgerSummaryProjection(
	RuntimeGameplayScenarioLedger &ledger,
	const RuntimeGameplayScenarioResult &result);

} // namespace iggy::runtime
