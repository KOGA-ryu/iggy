#include <cstdlib>
#include <vector>

#include "runtime/RuntimeGameplayFrameReporter.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

bool HasEvent(
	const std::vector<iggy::runtime::RuntimeGameplayFrameEvent> &events,
	iggy::runtime::RuntimeGameplayFrameEvent expected)
{
	for (const iggy::runtime::RuntimeGameplayFrameEvent event : events) {
		if (event == expected)
			return true;
	}
	return false;
}

iggy::runtime::RuntimeGameplayFrameResult ResultWithNestedReport(
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport nested)
{
	iggy::runtime::RuntimeGameplayFrameResult result;
	result.report = nested;
	return result;
}

void TestEmptyNoOpReportHasZeroCountsAndPreservesNestedReport()
{
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport nested;
	const iggy::runtime::RuntimeGameplayFrameResult result = ResultWithNestedReport(nested);

	const iggy::runtime::RuntimeGameplayFrameReport report =
		iggy::runtime::RuntimeGameplayFrameReporter {}.report(result);

	Expect(report.acceptedCommandCount == 0, "empty gameplay report should have zero accepted commands");
	Expect(report.blockedIntentCount == 0, "empty gameplay report should have zero blocked intents");
	Expect(report.rejectedIntentCount == 0, "empty gameplay report should have zero rejected intents");
	Expect(report.interactionEventCount == 0, "empty gameplay report should have zero interaction events");
	Expect(report.pickedUpCount == 0, "empty gameplay report should have zero pickups");
	Expect(!report.interactionChanged, "empty gameplay report should not mark interaction changed");
	Expect(!report.inventoryChanged, "empty gameplay report should not mark inventory changed");
	Expect(!report.hasEvents(), "empty gameplay report should have no summary events");
	Expect(report.input.acceptedCommandCount == nested.acceptedCommandCount, "empty gameplay report should preserve nested report");
}

void TestMoveOnlyReportProjectsPlayerAndCommandFacts()
{
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport nested;
	nested.acceptedCommandCount = 1;
	nested.interaction.playerInput.queuedFrameCount = 1;
	nested.interaction.playerInput.tickResultCount = 1;

	const iggy::runtime::RuntimeGameplayFrameReport report =
		iggy::runtime::RuntimeGameplayFrameReporter {}.report(ResultWithNestedReport(nested));

	Expect(report.acceptedCommandCount == 1, "move gameplay report should project accepted command count");
	Expect(!report.interactionChanged, "move gameplay report should not mark interaction changed");
	Expect(!report.inventoryChanged, "move gameplay report should not mark inventory changed");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::PlayerCommandAccepted), "move gameplay report should emit accepted command event");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::CommandFrameQueued), "move gameplay report should emit command frame queued event");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::CommandRunnerRan), "move gameplay report should emit command runner ran event");
}

void TestToggleTargetReportProjectsInteractionChangeAndEventFacts()
{
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport nested;
	nested.acceptedCommandCount = 1;
	nested.interactionMutated = true;
	nested.interactionEventCount = 1;
	nested.interactionAppliedCount = 1;

	const iggy::runtime::RuntimeGameplayFrameReport report =
		iggy::runtime::RuntimeGameplayFrameReporter {}.report(ResultWithNestedReport(nested));

	Expect(report.acceptedCommandCount == 1, "toggle gameplay report should preserve accepted command count");
	Expect(report.interactionChanged, "toggle gameplay report should project interaction changed flag");
	Expect(report.interactionEventCount == 1, "toggle gameplay report should project interaction event count");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::InteractionChanged), "toggle gameplay report should emit interaction changed event");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::InteractionEventRecorded), "toggle gameplay report should emit interaction event recorded event");
}

void TestPickupReportProjectsItemPickedUpAndInventoryChanged()
{
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport nested;
	nested.acceptedCommandCount = 1;
	nested.pickedUpCount = 2;
	nested.inventoryChanged = true;

	const iggy::runtime::RuntimeGameplayFrameReport report =
		iggy::runtime::RuntimeGameplayFrameReporter {}.report(ResultWithNestedReport(nested));

	Expect(report.pickedUpCount == 2, "pickup gameplay report should project picked-up count");
	Expect(report.inventoryChanged, "pickup gameplay report should project inventory changed flag");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::InventoryChanged), "pickup gameplay report should emit inventory changed event");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::ItemPickedUp), "pickup gameplay report should emit item picked up event");
}

void TestContextBlockedInputReportsBlockedIntentOnly()
{
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport nested;
	nested.blockedIntentCount = 1;

	const iggy::runtime::RuntimeGameplayFrameReport report =
		iggy::runtime::RuntimeGameplayFrameReporter {}.report(ResultWithNestedReport(nested));

	Expect(report.blockedIntentCount == 1, "blocked gameplay report should project blocked intent count");
	Expect(!report.interactionChanged, "blocked gameplay report should not mark interaction changed");
	Expect(!report.inventoryChanged, "blocked gameplay report should not mark inventory changed");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::PlayerIntentBlocked), "blocked gameplay report should emit blocked intent event");
	Expect(!HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::InteractionChanged), "blocked gameplay report should not emit interaction changed event");
}

void TestQueueRejectedInputReportsRejectionAndNoMutation()
{
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport nested;
	nested.rejectedIntentCount = 1;

	const iggy::runtime::RuntimeGameplayFrameReport report =
		iggy::runtime::RuntimeGameplayFrameReporter {}.report(ResultWithNestedReport(nested));

	Expect(report.rejectedIntentCount == 1, "queue-rejected gameplay report should project rejected intent count");
	Expect(!report.interactionChanged, "queue-rejected gameplay report should not mark interaction changed");
	Expect(!report.inventoryChanged, "queue-rejected gameplay report should not mark inventory changed");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::PlayerIntentRejected), "queue-rejected gameplay report should emit rejected intent event");
	Expect(!HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::ItemPickedUp), "queue-rejected gameplay report should not emit pickup event");
}

void TestReporterPreservesNestedLowerLevelReportDetails()
{
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport nested;
	nested.acceptedCommandCount = 3;
	nested.blockedIntentCount = 1;
	nested.rejectedIntentCount = 2;
	nested.interactionAppliedCount = 4;
	nested.interactionDeferredCount = 5;
	nested.interactionEventCount = 6;
	nested.pickedUpCount = 7;
	nested.pickupNotReadyCount = 8;
	nested.pickupFailedCount = 9;
	nested.interactionMutated = true;
	nested.inventoryChanged = true;
	nested.interaction.playerInput.queuedFrameCount = 10;
	nested.interaction.playerInput.tickResultCount = 11;

	const iggy::runtime::RuntimeGameplayFrameReport report =
		iggy::runtime::RuntimeGameplayFrameReporter {}.report(ResultWithNestedReport(nested));

	Expect(report.input.acceptedCommandCount == 3, "gameplay report should preserve nested accepted command count");
	Expect(report.input.blockedIntentCount == 1, "gameplay report should preserve nested blocked count");
	Expect(report.input.rejectedIntentCount == 2, "gameplay report should preserve nested rejected count");
	Expect(report.input.interactionAppliedCount == 4, "gameplay report should preserve nested interaction applied count");
	Expect(report.input.interactionDeferredCount == 5, "gameplay report should preserve nested deferred count");
	Expect(report.input.interactionEventCount == 6, "gameplay report should preserve nested interaction event count");
	Expect(report.input.pickedUpCount == 7, "gameplay report should preserve nested pickup count");
	Expect(report.input.pickupNotReadyCount == 8, "gameplay report should preserve nested pickup not-ready count");
	Expect(report.input.pickupFailedCount == 9, "gameplay report should preserve nested pickup failed count");
	Expect(report.input.interaction.playerInput.queuedFrameCount == 10, "gameplay report should preserve nested player input report");
	Expect(report.input.interaction.playerInput.tickResultCount == 11, "gameplay report should preserve nested tick count");
	Expect(report.pickupNotReadyCount == 8, "gameplay report should project pickup not-ready count");
	Expect(report.pickupFailedCount == 9, "gameplay report should project pickup failed count");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::PickupNotReady), "gameplay report should emit pickup not-ready event");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::PickupFailed), "gameplay report should emit pickup failed event");
}

void TestReporterDoesNotMutateInputResult()
{
	iggy::runtime::RuntimeGameplayFrameResult result;
	result.report.acceptedCommandCount = 1;
	result.report.inventoryChanged = true;
	const iggy::runtime::RuntimeGameplayFrameResult before = result;

	(void)iggy::runtime::RuntimeGameplayFrameReporter {}.report(result);

	Expect(result.report.acceptedCommandCount == before.report.acceptedCommandCount, "gameplay reporter should not mutate accepted command count");
	Expect(result.report.inventoryChanged == before.report.inventoryChanged, "gameplay reporter should not mutate inventory changed flag");
}

} // namespace

int main()
{
	TestEmptyNoOpReportHasZeroCountsAndPreservesNestedReport();
	TestMoveOnlyReportProjectsPlayerAndCommandFacts();
	TestToggleTargetReportProjectsInteractionChangeAndEventFacts();
	TestPickupReportProjectsItemPickedUpAndInventoryChanged();
	TestContextBlockedInputReportsBlockedIntentOnly();
	TestQueueRejectedInputReportsRejectionAndNoMutation();
	TestReporterPreservesNestedLowerLevelReportDetails();
	TestReporterDoesNotMutateInputResult();

	return Failures;
}
