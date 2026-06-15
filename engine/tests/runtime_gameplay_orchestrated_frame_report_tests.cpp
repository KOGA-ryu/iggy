#include <cstdlib>
#include <vector>

#include "runtime/RuntimeGameplayOrchestratedFrameReport.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

bool SameEvents(
	const std::vector<iggy::runtime::RuntimeGameplayOrchestratedFrameEvent> &actual,
	std::vector<iggy::runtime::RuntimeGameplayOrchestratedFrameEvent> expected)
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
		iggy::itemAddedInventoryEvent(Id("item:orchestrated-report"), 1),
		iggy::dropConsumedInventoryEvent(Id("drop:orchestrated-report")),
		iggy::itemPickedUpInventoryEvent(
			Id("drop:orchestrated-report"),
			Id("item:orchestrated-report"),
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

void TestEmptyNoOpReportHasZeroCountsAndUnchangedEvent()
{
	const iggy::runtime::RuntimeGameplayOrchestratedFrameResult result;

	const iggy::runtime::RuntimeGameplayOrchestratedFrameReport report =
		iggy::runtime::RuntimeGameplayOrchestratedFrameReporter {}.report(result);

	Expect(report.acceptedCommandCount == 0, "empty orchestrated report should have zero player commands");
	Expect(report.inventoryEventCount == 0, "empty orchestrated report should have zero inventory events");
	Expect(report.npcMovedCount == 0 && report.npcBlockedMovementCount == 0, "empty orchestrated report should have zero NPC movement counts");
	Expect(!report.changed() && !report.refreshedNpcData(), "empty orchestrated report should not mark change or refresh");
	Expect(SameEvents(report.events, { iggy::runtime::RuntimeGameplayOrchestratedFrameEvent::FrameUnchanged }), "empty orchestrated report should emit only unchanged event");
}

void TestPlayerPickupReportProjectsPlayerAndInventoryFacts()
{
	const iggy::runtime::RuntimeGameplayOrchestratedFrameResult result = PickupResult();

	const iggy::runtime::RuntimeGameplayOrchestratedFrameReport report =
		iggy::runtime::RuntimeGameplayOrchestratedFrameReporter {}.report(result);

	Expect(report.acceptedCommandCount == 1 && report.pickedUpCount == 1, "pickup orchestrated report should project player counts");
	Expect(report.inventoryEventCount == 3 && report.inventoryEvents.events.size() == 3, "pickup orchestrated report should preserve inventory events");
	Expect(report.player.pickedUpCount == 1, "pickup orchestrated report should preserve nested gameplay report facts");
	Expect(report.npcMovedCount == 0 && report.npcControlPlannedRequestCount == 0, "pickup orchestrated report should not invent NPC counts");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimeGameplayOrchestratedFrameEvent::PlayerFrameRan,
		iggy::runtime::RuntimeGameplayOrchestratedFrameEvent::InventoryEventObserved,
		iggy::runtime::RuntimeGameplayOrchestratedFrameEvent::FrameChanged,
	}), "pickup orchestrated report should emit deterministic player events");
}

void TestNpcMovementReportProjectsControlMovementAndRefreshFacts()
{
	const iggy::runtime::RuntimeGameplayOrchestratedFrameResult result = NpcMovedResult();

	const iggy::runtime::RuntimeGameplayOrchestratedFrameReport report =
		iggy::runtime::RuntimeGameplayOrchestratedFrameReporter {}.report(result);

	Expect(report.npcControlPlannedRequestCount == 1 && report.npcControlAppliedCount == 1, "moved orchestrated report should project control counts");
	Expect(report.npcMovedCount == 1 && report.npcRefreshDirtyTileCount == 2, "moved orchestrated report should project movement refresh facts");
	Expect(report.npcControlsChanged && report.npcActorsChanged, "moved orchestrated report should project changed flags");
	Expect(report.refreshedNpcData(), "moved orchestrated report should mark refresh packets");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimeGameplayOrchestratedFrameEvent::NpcControlChanged,
		iggy::runtime::RuntimeGameplayOrchestratedFrameEvent::NpcActorMoved,
		iggy::runtime::RuntimeGameplayOrchestratedFrameEvent::RefreshOccupancy,
		iggy::runtime::RuntimeGameplayOrchestratedFrameEvent::RefreshInteraction,
		iggy::runtime::RuntimeGameplayOrchestratedFrameEvent::RefreshAiMap,
		iggy::runtime::RuntimeGameplayOrchestratedFrameEvent::RefreshRender,
		iggy::runtime::RuntimeGameplayOrchestratedFrameEvent::RefreshVisibility,
		iggy::runtime::RuntimeGameplayOrchestratedFrameEvent::FrameChanged,
	}), "moved orchestrated report should emit deterministic NPC events");
}

void TestBlockedNpcMovementReportsBlockedWithoutRefresh()
{
	const iggy::runtime::RuntimeGameplayOrchestratedFrameResult result = NpcBlockedResult();

	const iggy::runtime::RuntimeGameplayOrchestratedFrameReport report =
		iggy::runtime::RuntimeGameplayOrchestratedFrameReporter {}.report(result);

	Expect(report.npcBlockedMovementCount == 1, "blocked orchestrated report should project blocked count");
	Expect(report.npcMovedCount == 0 && report.npcRefreshDirtyTileCount == 0, "blocked orchestrated report should not project moved dirty tiles");
	Expect(!report.refreshedNpcData(), "blocked orchestrated report should not refresh unless nested result says so");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimeGameplayOrchestratedFrameEvent::NpcControlChanged,
		iggy::runtime::RuntimeGameplayOrchestratedFrameEvent::NpcActorBlocked,
		iggy::runtime::RuntimeGameplayOrchestratedFrameEvent::FrameChanged,
	}), "blocked orchestrated report should emit deterministic blocked events");
}

void TestReporterDoesNotMutateInputResult()
{
	iggy::runtime::RuntimeGameplayOrchestratedFrameResult result = NpcMovedResult();
	const iggy::runtime::RuntimeGameplayOrchestratedFrameResult before = result;

	const iggy::runtime::RuntimeGameplayOrchestratedFrameReport report =
		iggy::runtime::RuntimeGameplayOrchestratedFrameReporter {}.report(result);

	Expect(report.npcMovedCount == 1, "immutability setup should project moved count");
	Expect(result.npcMovedCount == before.npcMovedCount, "orchestrated reporter should not mutate moved count");
	Expect(result.npcRefreshDirtyTileCount == before.npcRefreshDirtyTileCount, "orchestrated reporter should not mutate dirty tile count");
	Expect(result.npcOccupancyRefreshed == before.npcOccupancyRefreshed, "orchestrated reporter should not mutate refresh flags");
}

} // namespace

int main()
{
	TestEmptyNoOpReportHasZeroCountsAndUnchangedEvent();
	TestPlayerPickupReportProjectsPlayerAndInventoryFacts();
	TestNpcMovementReportProjectsControlMovementAndRefreshFacts();
	TestBlockedNpcMovementReportsBlockedWithoutRefresh();
	TestReporterDoesNotMutateInputResult();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
