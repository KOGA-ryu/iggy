#pragma once

#include <cstddef>

#include "runtime/RuntimeGameplayOrchestratedFrameReport.hpp"
#include "runtime/RuntimeGameplayOrchestratedFrameRunnerReport.hpp"
#include "runtime/RuntimeGameplayOrchestratedFrameStep.hpp"
#include "runtime/RuntimeNpcActorMovementPlannedFrameStep.hpp"
#include "runtime/RuntimeNpcAiControlPlannedFrameStep.hpp"
#include "runtime/RuntimeNpcAiMovementPlannedFrameStep.hpp"
#include "runtime/RuntimeNpcAiMovementRefreshFrameStep.hpp"

namespace iggy::runtime {

struct RuntimeNpcMovementAggregate {
	std::size_t plannedRequestCount = 0;
	std::size_t preReservationRequestCount = 0;
	std::size_t reservationAcceptedCount = 0;
	std::size_t reservationRejectedCount = 0;
	std::size_t movedCount = 0;
	std::size_t blockedMovementCount = 0;
	std::size_t rejectedMovementCount = 0;
	std::size_t missingActorMovementCount = 0;
	std::size_t dirtyTileCount = 0;
	bool actorsChanged = false;
	bool needsOccupancyRefresh = false;
	bool needsAiMapRefresh = false;
	bool needsInteractionRefresh = false;
	bool needsRenderRefresh = false;
	bool needsVisibilityRefresh = false;

	[[nodiscard]] bool hasMovementFacts() const
	{
		return plannedRequestCount > 0
			|| preReservationRequestCount > 0
			|| reservationAcceptedCount > 0
			|| reservationRejectedCount > 0
			|| movedCount > 0
			|| blockedMovementCount > 0
			|| rejectedMovementCount > 0
			|| missingActorMovementCount > 0;
	}

	[[nodiscard]] bool needsAnyRefresh() const
	{
		return needsOccupancyRefresh
			|| needsAiMapRefresh
			|| needsInteractionRefresh
			|| needsRenderRefresh
			|| needsVisibilityRefresh;
	}
};

struct RuntimeNpcControlAggregate {
	std::size_t plannedRequestCount = 0;
	std::size_t appliedCount = 0;
	std::size_t failedCount = 0;
	std::size_t planIssueCount = 0;
	std::size_t mapChangedSelectionCount = 0;
	bool controlsChanged = false;

	[[nodiscard]] bool hasControlFacts() const
	{
		return plannedRequestCount > 0
			|| appliedCount > 0
			|| failedCount > 0
			|| planIssueCount > 0
			|| mapChangedSelectionCount > 0;
	}
};

struct RuntimeNpcRefreshAggregate {
	std::size_t dirtyTileCount = 0;
	std::size_t occupancyRefreshCount = 0;
	std::size_t interactionRefreshCount = 0;
	std::size_t aiMapRefreshCount = 0;
	std::size_t renderRefreshCount = 0;
	std::size_t visibilityRefreshCount = 0;
	bool occupancyRefreshed = false;
	bool interactionRefreshed = false;
	bool aiMapRefreshed = false;
	bool renderRefreshed = false;
	bool visibilityRefreshed = false;

	[[nodiscard]] bool refreshedAny() const
	{
		return occupancyRefreshed
			|| interactionRefreshed
			|| aiMapRefreshed
			|| renderRefreshed
			|| visibilityRefreshed;
	}
};

void foldRuntimeNpcMovementAggregate(
	RuntimeNpcMovementAggregate &aggregate,
	const RuntimeNpcActorMovementPlannedFrameResult &result);

void foldRuntimeNpcMovementAggregate(
	RuntimeNpcMovementAggregate &aggregate,
	const RuntimeNpcAiMovementPlannedFrameResult &result);

void foldRuntimeNpcMovementAggregate(
	RuntimeNpcMovementAggregate &aggregate,
	const RuntimeNpcAiMovementRefreshFrameResult &result);

void foldRuntimeNpcMovementAggregate(
	RuntimeNpcMovementAggregate &aggregate,
	const RuntimeGameplayOrchestratedFrameResult &result);

void foldRuntimeNpcMovementAggregate(
	RuntimeNpcMovementAggregate &aggregate,
	const RuntimeGameplayOrchestratedFrameReport &report);

void foldRuntimeNpcMovementAggregate(
	RuntimeNpcMovementAggregate &aggregate,
	const RuntimeGameplayOrchestratedFrameRunnerReport &report);

void foldRuntimeNpcControlAggregate(
	RuntimeNpcControlAggregate &aggregate,
	const RuntimeNpcAiControlPlannedFrameResult &result);

void foldRuntimeNpcControlAggregate(
	RuntimeNpcControlAggregate &aggregate,
	const RuntimeNpcAiMovementPlannedFrameResult &result);

void foldRuntimeNpcControlAggregate(
	RuntimeNpcControlAggregate &aggregate,
	const RuntimeNpcAiMovementRefreshFrameResult &result);

void foldRuntimeNpcControlAggregate(
	RuntimeNpcControlAggregate &aggregate,
	const RuntimeGameplayOrchestratedFrameResult &result);

void foldRuntimeNpcControlAggregate(
	RuntimeNpcControlAggregate &aggregate,
	const RuntimeGameplayOrchestratedFrameReport &report);

void foldRuntimeNpcControlAggregate(
	RuntimeNpcControlAggregate &aggregate,
	const RuntimeGameplayOrchestratedFrameRunnerReport &report);

void foldRuntimeNpcRefreshAggregate(
	RuntimeNpcRefreshAggregate &aggregate,
	const RuntimeNpcAiMovementRefreshFrameResult &result);

void foldRuntimeNpcRefreshAggregate(
	RuntimeNpcRefreshAggregate &aggregate,
	const RuntimeGameplayOrchestratedFrameResult &result);

void foldRuntimeNpcRefreshAggregate(
	RuntimeNpcRefreshAggregate &aggregate,
	const RuntimeGameplayOrchestratedFrameReport &report);

void foldRuntimeNpcRefreshAggregate(
	RuntimeNpcRefreshAggregate &aggregate,
	const RuntimeGameplayOrchestratedFrameRunnerReport &report);

} // namespace iggy::runtime
