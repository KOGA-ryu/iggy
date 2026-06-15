#include <cstdlib>
#include <vector>

#include "runtime/RuntimeGameplayOrchestratedFrameRunnerReport.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

bool SameEvents(
	const std::vector<iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerEvent> &actual,
	std::vector<iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerEvent> expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index] != expected[index])
			return false;
	}
	return true;
}

iggy::runtime::RuntimeGameplayOrchestratedFrameResult PickupResult()
{
	iggy::runtime::RuntimeGameplayOrchestratedFrameResult result;
	result.acceptedCommandCount = 1;
	result.pickedUpCount = 1;
	result.inventoryChanged = true;
	result.inventoryEvents.events = {
		iggy::itemAddedInventoryEvent(Id("item:orchestrated-runner-report"), 1),
		iggy::dropConsumedInventoryEvent(Id("drop:orchestrated-runner-report")),
		iggy::itemPickedUpInventoryEvent(
			Id("drop:orchestrated-runner-report"),
			Id("item:orchestrated-runner-report"),
			1),
	};
	result.inventoryEventCount = result.inventoryEvents.events.size();
	result.playerFrame.report.acceptedCommandCount = result.acceptedCommandCount;
	result.playerFrame.report.pickedUpCount = result.pickedUpCount;
	result.playerFrame.report.inventoryChanged = true;
	result.playerFrame.inventoryEvents = result.inventoryEvents;
	return result;
}

iggy::runtime::RuntimeGameplayOrchestratedFrameResult NpcMovedResult()
{
	iggy::runtime::RuntimeGameplayOrchestratedFrameResult result;
	result.npcControlPlannedRequestCount = 1;
	result.npcControlAppliedCount = 1;
	result.npcMovementPlannedRequestCount = 1;
	result.npcMovedCount = 1;
	result.npcRefreshDirtyTileCount = 2;
	result.npcControlsChanged = true;
	result.npcActorsChanged = true;
	result.npcOccupancyRefreshed = true;
	result.npcInteractionRefreshed = true;
	result.npcAiMapRefreshed = true;
	result.npcRenderRefreshed = true;
	result.npcVisibilityRefreshed = true;
	return result;
}

iggy::runtime::RuntimeGameplayOrchestratedFrameResult NpcBlockedResult()
{
	iggy::runtime::RuntimeGameplayOrchestratedFrameResult result;
	result.npcControlPlannedRequestCount = 1;
	result.npcControlAppliedCount = 1;
	result.npcMovementPlannedRequestCount = 1;
	result.npcBlockedMovementCount = 1;
	result.npcControlsChanged = true;
	return result;
}

iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerResult RunnerResult(
	std::vector<iggy::runtime::RuntimeGameplayOrchestratedFrameResult> frames)
{
	iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerResult result;
	result.frameCount = frames.size();
	result.frameResults = frames;
	return result;
}

void TestEmptyRunnerReportHasZeroCountsAndUnchangedEvent()
{
	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerResult result;

	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerReport report =
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerReporter {}.report(result);

	Expect(report.frameCount == 0 && report.frames.empty(), "empty orchestrated runner report should have no frames");
	Expect(report.inventoryEventCount == 0 && report.npcMovedCount == 0, "empty orchestrated runner report should have zero counts");
	Expect(!report.changed() && !report.refreshedNpcData(), "empty orchestrated runner report should not mark change or refresh");
	Expect(SameEvents(report.events, { iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerEvent::RunnerUnchanged }), "empty orchestrated runner report should emit unchanged event");
}

void TestRunnerReportAggregatesFrameReportsInOrder()
{
	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerResult result =
		RunnerResult({ PickupResult(), NpcMovedResult(), NpcBlockedResult() });

	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerReport report =
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerReporter {}.report(result);

	Expect(report.frameCount == 3 && report.frames.size() == 3, "runner report should preserve per-frame report count");
	Expect(report.frames[0].pickedUpCount == 1, "runner report should preserve first frame report in order");
	Expect(report.frames[1].npcMovedCount == 1, "runner report should preserve second frame report in order");
	Expect(report.frames[2].npcBlockedMovementCount == 1, "runner report should preserve third frame report in order");
	Expect(report.inventoryEventCount == 3 && report.inventoryEvents.events.size() == 3, "runner report should aggregate inventory events");
	Expect(report.npcControlPlannedRequestCount == 2 && report.npcControlAppliedCount == 2, "runner report should aggregate NPC control counts");
	Expect(report.npcMovedCount == 1 && report.npcBlockedMovementCount == 1, "runner report should aggregate NPC movement counts");
	Expect(report.npcRefreshDirtyTileCount == 2, "runner report should aggregate dirty tile counts");
	Expect(report.changedFrameCount == 3, "runner report should aggregate changed frame count");
	Expect(report.npcOccupancyRefreshed && report.npcVisibilityRefreshed, "runner report should OR refresh flags");
}

void TestRunnerReportEventOrderIsDeterministic()
{
	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerResult result =
		RunnerResult({ PickupResult(), NpcMovedResult(), NpcBlockedResult() });

	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerReport report =
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerReporter {}.report(result);

	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerEvent::FrameChangedObserved,
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerEvent::InventoryEventObserved,
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerEvent::NpcControlChanged,
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerEvent::NpcActorMoved,
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerEvent::NpcActorBlocked,
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerEvent::RefreshOccupancy,
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerEvent::RefreshInteraction,
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerEvent::RefreshAiMap,
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerEvent::RefreshRender,
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerEvent::RefreshVisibility,
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerEvent::RunnerChanged,
	}), "runner report should emit deterministic aggregate events");
}

void TestManualFrameAggregationMatchesRunnerReport()
{
	const std::vector<iggy::runtime::RuntimeGameplayOrchestratedFrameResult> frames {
		PickupResult(),
		NpcMovedResult(),
		NpcBlockedResult(),
	};
	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerReport runnerReport =
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerReporter {}.report(RunnerResult(frames));

	std::size_t inventoryEvents = 0;
	std::size_t moved = 0;
	std::size_t blocked = 0;
	std::size_t dirtyTiles = 0;
	std::size_t changedFrames = 0;
	iggy::runtime::RuntimeGameplayOrchestratedFrameReporter frameReporter;
	for (const iggy::runtime::RuntimeGameplayOrchestratedFrameResult &frame : frames) {
		const iggy::runtime::RuntimeGameplayOrchestratedFrameReport report = frameReporter.report(frame);
		inventoryEvents += report.inventoryEventCount;
		moved += report.npcMovedCount;
		blocked += report.npcBlockedMovementCount;
		dirtyTiles += report.npcRefreshDirtyTileCount;
		if (report.changed() || report.refreshedNpcData()) {
			++changedFrames;
		}
	}

	Expect(runnerReport.inventoryEventCount == inventoryEvents, "runner report should match manual inventory aggregation");
	Expect(runnerReport.npcMovedCount == moved, "runner report should match manual moved aggregation");
	Expect(runnerReport.npcBlockedMovementCount == blocked, "runner report should match manual blocked aggregation");
	Expect(runnerReport.npcRefreshDirtyTileCount == dirtyTiles, "runner report should match manual dirty tile aggregation");
	Expect(runnerReport.changedFrameCount == changedFrames, "runner report should match manual changed frame aggregation");
}

void TestRunnerReporterDoesNotMutateInputResult()
{
	iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerResult result =
		RunnerResult({ PickupResult(), NpcMovedResult() });
	const std::size_t beforeFrameCount = result.frameCount;
	const std::size_t beforeResultCount = result.frameResults.size();

	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerReport report =
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerReporter {}.report(result);

	Expect(report.frameCount == 2, "immutability setup should project frame count");
	Expect(result.frameCount == beforeFrameCount, "runner reporter should not mutate frame count");
	Expect(result.frameResults.size() == beforeResultCount, "runner reporter should not mutate frame results");
}

} // namespace

int main()
{
	TestEmptyRunnerReportHasZeroCountsAndUnchangedEvent();
	TestRunnerReportAggregatesFrameReportsInOrder();
	TestRunnerReportEventOrderIsDeterministic();
	TestManualFrameAggregationMatchesRunnerReport();
	TestRunnerReporterDoesNotMutateInputResult();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
