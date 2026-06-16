#include "runtime/RuntimeGameplayProfileScenarioRunner.hpp"

namespace iggy::runtime {
namespace {

void CopyLedgerCounts(RuntimeGameplayProfileScenarioRunResult &result)
{
	result.frameCount = result.ledger.frameCount;
	result.changedFrameCount = result.ledger.changedFrameCount;
	result.inventoryEventCount = result.ledger.inventoryEventCount;
	result.npcControlPlannedRequestCount = result.ledger.npcControlPlannedRequestCount;
	result.npcControlAppliedCount = result.ledger.npcControlAppliedCount;
	result.npcControlFailedCount = result.ledger.npcControlFailedCount;
	result.npcControlsChanged = result.ledger.npcControlsChanged;
	result.npcMovementPlannedRequestCount = result.ledger.npcMovementPlannedRequestCount;
	result.npcMovedCount = result.ledger.npcMovedCount;
	result.npcBlockedMovementCount = result.ledger.npcBlockedMovementCount;
	result.npcRejectedMovementCount = result.ledger.npcRejectedMovementCount;
	result.npcMissingActorMovementCount = result.ledger.npcMissingActorMovementCount;
	result.npcActorsChanged = result.ledger.npcActorsChanged;
	result.npcRefreshDirtyTileCount = result.ledger.npcRefreshDirtyTileCount;
	result.npcOccupancyRefreshed = result.ledger.npcOccupancyRefreshed;
	result.npcInteractionRefreshed = result.ledger.npcInteractionRefreshed;
	result.npcAiMapRefreshed = result.ledger.npcAiMapRefreshed;
	result.npcRenderRefreshed = result.ledger.npcRenderRefreshed;
	result.npcVisibilityRefreshed = result.ledger.npcVisibilityRefreshed;
}

RuntimeGameplayScenarioResult NoRunScenarioResult(
	const RuntimeGameplayProfileScenarioDefinition &definition)
{
	RuntimeGameplayScenarioResult result;
	result.scenario.initialState = definition.initialState;
	result.state = definition.initialState;
	return result;
}

} // namespace

bool RuntimeGameplayProfileScenarioRunResult::ran() const
{
	return status == RuntimeGameplayProfileScenarioRunStatus::Ran;
}

bool RuntimeGameplayProfileScenarioRunResult::changed() const
{
	return changedFrameCount > 0;
}

bool RuntimeGameplayProfileScenarioRunResult::refreshedNpcData() const
{
	return npcOccupancyRefreshed
		|| npcInteractionRefreshed
		|| npcAiMapRefreshed
		|| npcRenderRefreshed
		|| npcVisibilityRefreshed;
}

RuntimeGameplayProfileScenarioRunResult RuntimeGameplayProfileScenarioRunner::run(
	const RuntimeGameplayProfileScenarioDefinition &definition) const
{
	RuntimeGameplayProfileScenarioRunResult result;
	result.definition = definition;
	result.validation = RuntimeGameplayProfileScenarioValidator {}.validate(definition);
	result.build = result.validation.build;

	if (!result.validation.ok()) {
		result.status = RuntimeGameplayProfileScenarioRunStatus::ValidationFailed;
		result.scenario = NoRunScenarioResult(definition);
		result.ledger = RuntimeGameplayScenarioLedgerReporter {}.report(result.validation, result.scenario);
		result.state = definition.initialState;
		CopyLedgerCounts(result);
		return result;
	}

	result.status = RuntimeGameplayProfileScenarioRunStatus::Ran;
	result.scenario = RuntimeGameplayScenarioRunner {}.run(result.build.scenario);
	result.ledger = RuntimeGameplayScenarioLedgerReporter {}.report(result.validation, result.scenario);
	result.state = result.scenario.state;
	CopyLedgerCounts(result);
	return result;
}

} // namespace iggy::runtime
