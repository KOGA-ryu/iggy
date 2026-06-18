#include "runtime/RuntimeGameplayScenarioLedger.hpp"

#include "runtime/RuntimeGameplayScenarioReportProjection.hpp"

namespace iggy::runtime {
namespace {

void CopyFinalStateFacts(RuntimeGameplayScenarioLedger &ledger)
{
	ledger.finalNpcActorCount = ledger.result.state.npcActors.actors.size();
	ledger.finalNpcControlCount = ledger.result.state.npcControls.entries.size();
	ledger.finalInventoryStackCount = ledger.result.state.inventory.inventory.stacks.size();
	ledger.finalInventoryDropCount = ledger.result.state.inventory.drops.drops.size();
	for (const NpcActorState2D &actor : ledger.result.state.npcActors.actors) {
		if (actor.present) {
			++ledger.finalPresentNpcCount;
		}
	}
}

void AppendEvents(RuntimeGameplayScenarioLedger &ledger)
{
	ledger.events.push_back(RuntimeGameplayScenarioLedgerEvent::ScenarioStarted);
	for (const RuntimeGameplayScenarioLedgerFrame &frame : ledger.frames) {
		ledger.events.push_back(
			frame.changed
				? RuntimeGameplayScenarioLedgerEvent::FrameChanged
				: RuntimeGameplayScenarioLedgerEvent::FrameUnchanged);
	}
	if (ledger.inventoryEventCount > 0) {
		ledger.events.push_back(RuntimeGameplayScenarioLedgerEvent::InventoryChanged);
	}
	if (ledger.npcControlsChanged) {
		ledger.events.push_back(RuntimeGameplayScenarioLedgerEvent::NpcControlChanged);
	}
	if (ledger.npcMovedCount > 0) {
		ledger.events.push_back(RuntimeGameplayScenarioLedgerEvent::NpcActorMoved);
	}
	if (ledger.npcBlockedMovementCount > 0) {
		ledger.events.push_back(RuntimeGameplayScenarioLedgerEvent::NpcActorBlocked);
	}
	if (ledger.npcRejectedMovementCount > 0) {
		ledger.events.push_back(RuntimeGameplayScenarioLedgerEvent::NpcActorRejected);
	}
	if (ledger.npcMissingActorMovementCount > 0) {
		ledger.events.push_back(RuntimeGameplayScenarioLedgerEvent::NpcActorMissing);
	}
	if (ledger.npcOccupancyRefreshed) {
		ledger.events.push_back(RuntimeGameplayScenarioLedgerEvent::RefreshOccupancy);
	}
	if (ledger.npcInteractionRefreshed) {
		ledger.events.push_back(RuntimeGameplayScenarioLedgerEvent::RefreshInteraction);
	}
	if (ledger.npcAiMapRefreshed) {
		ledger.events.push_back(RuntimeGameplayScenarioLedgerEvent::RefreshAiMap);
	}
	if (ledger.npcRenderRefreshed) {
		ledger.events.push_back(RuntimeGameplayScenarioLedgerEvent::RefreshRender);
	}
	if (ledger.npcVisibilityRefreshed) {
		ledger.events.push_back(RuntimeGameplayScenarioLedgerEvent::RefreshVisibility);
	}
	ledger.events.push_back(
		ledger.changed() || ledger.refreshedNpcData()
			? RuntimeGameplayScenarioLedgerEvent::ScenarioChanged
			: RuntimeGameplayScenarioLedgerEvent::ScenarioUnchanged);
}

void AddAiExplanationCounts(
	RuntimeGameplayScenarioLedger &ledger,
	const NpcMapPlayControlExplainLedger &explanation)
{
	ledger.npcAiProfileResolvedCount += explanation.profileResolvedCount;
	ledger.npcAiProfileMissingCount += explanation.profileMissingCount;
	ledger.npcAiHandDrawnCount += explanation.handDrawnCount;
	ledger.npcAiMapChangedSelectionCount += explanation.mapChangedSelectionCount;
	ledger.npcAiPlayKeptCount += explanation.playKeptCount;
	ledger.npcAiPlayFoldedCount += explanation.playFoldedCount;
	ledger.npcAiControlProposedCount += explanation.controlProposedCount;
	ledger.npcAiControlFailedCount += explanation.controlFailedCount;
	ledger.npcAiControlAppliedCount += explanation.controlAppliedCount;
	ledger.npcAiControlApplyFailedCount += explanation.controlApplyFailedCount;
}

void AddAiExplanations(
	RuntimeGameplayScenarioLedger &ledger,
	const RuntimeGameplayProfileScenarioDefinitionBuildResult &profileBuild)
{
	const std::size_t frameCount =
		profileBuild.profileFrames.size() < ledger.result.runner.frameResults.size()
			? profileBuild.profileFrames.size()
			: ledger.result.runner.frameResults.size();

	ledger.npcAiExplanations.clear();
	ledger.npcAiExplanations.reserve(frameCount);
	NpcMapPlayControlExplainLedgerReporter reporter;
	for (std::size_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
		const RuntimeGameplayOrchestratedFrameResult &frame =
			ledger.result.runner.frameResults[frameIndex];
		const RuntimeNpcAiControlPlannedFrameResult &control =
			frame.npcFrame.planned.control;
		NpcMapPlayControlExplainLedger explanation = reporter.report(
			profileBuild.profileFrames[frameIndex],
			control.plan,
			control.step);
		AddAiExplanationCounts(ledger, explanation);
		ledger.npcAiExplanations.push_back(explanation);
	}
	ledger.npcAiExplainFrameCount = ledger.npcAiExplanations.size();
}

} // namespace

bool RuntimeGameplayScenarioLedger::empty() const
{
	return status == RuntimeGameplayScenarioLedgerStatus::Empty;
}

bool RuntimeGameplayScenarioLedger::changed() const
{
	return changedFrameCount > 0;
}

bool RuntimeGameplayScenarioLedger::refreshedNpcData() const
{
	return npcOccupancyRefreshed
		|| npcInteractionRefreshed
		|| npcAiMapRefreshed
		|| npcRenderRefreshed
		|| npcVisibilityRefreshed;
}

bool RuntimeGameplayScenarioLedger::hasEvents() const
{
	return !events.empty();
}

bool RuntimeGameplayScenarioLedger::hasNpcAiExplanations() const
{
	return !npcAiExplanations.empty();
}

RuntimeGameplayScenarioLedger RuntimeGameplayScenarioLedgerReporter::report(
	const RuntimeGameplayScenarioResult &result) const
{
	RuntimeGameplayScenarioLedger ledger;
	ledger.result = result;
	ledger.status = result.hasFrames()
		? RuntimeGameplayScenarioLedgerStatus::Reported
		: RuntimeGameplayScenarioLedgerStatus::Empty;
	applyRuntimeGameplayScenarioLedgerSummaryProjection(ledger, result);
	CopyFinalStateFacts(ledger);

	ledger.frames.reserve(result.report.frames.size());
	for (std::size_t frameIndex = 0; frameIndex < result.report.frames.size(); ++frameIndex) {
		ledger.frames.push_back(
			projectRuntimeGameplayScenarioLedgerFrame(frameIndex, result.report.frames[frameIndex]));
	}

	AppendEvents(ledger);
	return ledger;
}

RuntimeGameplayScenarioLedger RuntimeGameplayScenarioLedgerReporter::report(
	const RuntimeGameplayProfileScenarioDefinitionBuildResult &profileBuild,
	const RuntimeGameplayScenarioResult &result) const
{
	RuntimeGameplayScenarioLedger ledger = report(result);
	ledger.hasProfileBuild = true;
	ledger.profileBuild = profileBuild;
	AddAiExplanations(ledger, profileBuild);
	return ledger;
}

RuntimeGameplayScenarioLedger RuntimeGameplayScenarioLedgerReporter::report(
	const RuntimeGameplayProfileScenarioValidationResult &profileValidation,
	const RuntimeGameplayScenarioResult &result) const
{
	RuntimeGameplayScenarioLedger ledger = report(result);
	ledger.hasProfileBuild = true;
	ledger.hasProfileValidation = true;
	ledger.profileBuild = profileValidation.build;
	ledger.profileValidation = profileValidation;
	AddAiExplanations(ledger, profileValidation.build);
	return ledger;
}

} // namespace iggy::runtime
