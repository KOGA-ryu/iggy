#include <cstdlib>
#include <vector>

#include "runtime/RuntimePolicyGameplayFrameReporter.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

bool HasEvent(
	const std::vector<iggy::runtime::RuntimePolicyGameplayFrameEvent> &events,
	iggy::runtime::RuntimePolicyGameplayFrameEvent expected)
{
	for (const iggy::runtime::RuntimePolicyGameplayFrameEvent event : events) {
		if (event == expected)
			return true;
	}
	return false;
}

void ExpectSummaryEventAt(
	const iggy::runtime::RuntimePolicyGameplayFrameReport &report,
	std::size_t index,
	iggy::runtime::RuntimePolicyGameplayFrameEvent expected,
	const char *message)
{
	Expect(report.events.size() > index, message);
	if (report.events.size() > index)
		Expect(report.events[index] == expected, message);
}

void ExpectInventoryEvent(
	const iggy::InventoryEvent2D &event,
	iggy::InventoryEvent2DType type,
	const iggy::ResourceId &itemId,
	const iggy::ResourceId &dropId,
	std::uint32_t count,
	const char *message)
{
	Expect(event.type == type, message);
	Expect(event.itemId == itemId, message);
	Expect(event.dropId == dropId, message);
	Expect(event.count == count, message);
}

iggy::runtime::RuntimePolicyGameplayFrameResult EmptyResult()
{
	return {};
}

void TestEmptyMoveOnlyReportProjectsPlayerFactsOnly()
{
	iggy::runtime::RuntimePolicyGameplayFrameResult result = EmptyResult();
	result.frame.interaction.playerInput.report.acceptedCommandCount = 1;
	result.frame.interaction.playerInput.report.queuedFrameCount = 1;
	result.frame.interaction.playerInput.report.tickResultCount = 1;

	const iggy::runtime::RuntimePolicyGameplayFrameReport report =
		iggy::runtime::RuntimePolicyGameplayFrameReporter {}.report(result);

	Expect(report.acceptedCommandCount == 1, "policy gameplay report should project accepted command count");
	Expect(report.blockedIntentCount == 0, "policy gameplay report should have no blocked intents");
	Expect(report.rejectedIntentCount == 0, "policy gameplay report should have no rejected intents");
	Expect(report.pickedUpCount == 0, "policy gameplay report should have no pickups");
	Expect(report.inventoryEventCount == 0, "policy gameplay report should have no inventory events");
	Expect(!report.interactionChanged, "policy gameplay report should not mark interaction changed");
	Expect(!report.inventoryChanged, "policy gameplay report should not mark inventory changed");
	Expect(report.hasEvents(), "policy gameplay report should expose player summary events");
	ExpectSummaryEventAt(report, 0, iggy::runtime::RuntimePolicyGameplayFrameEvent::PlayerCommandAccepted, "policy gameplay report should emit accepted event first");
	ExpectSummaryEventAt(report, 1, iggy::runtime::RuntimePolicyGameplayFrameEvent::CommandFrameQueued, "policy gameplay report should emit queued event second");
	ExpectSummaryEventAt(report, 2, iggy::runtime::RuntimePolicyGameplayFrameEvent::CommandRunnerRan, "policy gameplay report should emit runner event third");
}

void TestSuccessfulPolicyPickupReportsCountsAndEvents()
{
	iggy::runtime::RuntimePolicyGameplayFrameResult result = EmptyResult();
	const iggy::ResourceId itemId = Id("item:potion");
	const iggy::ResourceId dropId = Id("drop:potion");
	result.frame.interaction.playerInput.report.acceptedCommandCount = 1;
	result.frame.pickup.pickedUpCount = 1;
	result.frame.pickup.changed = true;
	result.inventoryEvents.events = {
		iggy::itemAddedInventoryEvent(itemId, 2),
		iggy::dropConsumedInventoryEvent(dropId),
		iggy::itemPickedUpInventoryEvent(dropId, itemId, 2),
	};

	const iggy::runtime::RuntimePolicyGameplayFrameReport report =
		iggy::runtime::RuntimePolicyGameplayFrameReporter {}.report(result);

	Expect(report.pickedUpCount == 1, "successful policy gameplay report should count pickup");
	Expect(report.pickupFailedCount == 0, "successful policy gameplay report should not count failure");
	Expect(report.inventoryChanged, "successful policy gameplay report should mark inventory changed");
	Expect(report.inventoryEventCount == 3, "successful policy gameplay report should count inventory events");
	Expect(report.itemAddedEventCount == 1, "successful policy gameplay report should count item added");
	Expect(report.dropConsumedEventCount == 1, "successful policy gameplay report should count drop consumed");
	Expect(report.itemPickedUpEventCount == 1, "successful policy gameplay report should count item picked up");
	if (report.inventoryEvents.events.size() == 3) {
		ExpectInventoryEvent(report.inventoryEvents.events[0], iggy::InventoryEvent2DType::ItemAdded, itemId, {}, 2, "successful policy gameplay report should preserve item-added payload");
		ExpectInventoryEvent(report.inventoryEvents.events[1], iggy::InventoryEvent2DType::DropConsumed, {}, dropId, 0, "successful policy gameplay report should preserve drop-consumed payload");
		ExpectInventoryEvent(report.inventoryEvents.events[2], iggy::InventoryEvent2DType::ItemPickedUp, itemId, dropId, 2, "successful policy gameplay report should preserve picked-up payload");
	}
	Expect(HasEvent(report.events, iggy::runtime::RuntimePolicyGameplayFrameEvent::InventoryChanged), "successful policy gameplay report should emit inventory changed");
	Expect(HasEvent(report.events, iggy::runtime::RuntimePolicyGameplayFrameEvent::ItemPickedUp), "successful policy gameplay report should emit item picked up");
	Expect(HasEvent(report.events, iggy::runtime::RuntimePolicyGameplayFrameEvent::InventoryEventRecorded), "successful policy gameplay report should emit inventory event recorded");
}

void TestExistingStackExactlyToMaxPreservesEventPayloadCount()
{
	iggy::runtime::RuntimePolicyGameplayFrameResult result = EmptyResult();
	const iggy::ResourceId itemId = Id("item:potion");
	const iggy::ResourceId dropId = Id("drop:potion");
	result.frame.pickup.pickedUpCount = 1;
	result.frame.pickup.changed = true;
	result.inventoryEvents.events = {
		iggy::itemAddedInventoryEvent(itemId, 2),
		iggy::dropConsumedInventoryEvent(dropId),
		iggy::itemPickedUpInventoryEvent(dropId, itemId, 2),
	};

	const iggy::runtime::RuntimePolicyGameplayFrameReport report =
		iggy::runtime::RuntimePolicyGameplayFrameReporter {}.report(result);

	Expect(report.itemAddedEventCount == 1 && report.itemPickedUpEventCount == 1, "exactly-max policy gameplay report should count success events");
	if (report.inventoryEvents.events.size() == 3) {
		Expect(report.inventoryEvents.events[0].count == 2, "exactly-max policy gameplay report should preserve requested add count");
		Expect(report.inventoryEvents.events[2].count == 2, "exactly-max policy gameplay report should preserve picked-up count");
	}
}

void TestOverCapacityReportsPolicyFailureWithoutSuccessEvents()
{
	iggy::runtime::RuntimePolicyGameplayFrameResult result = EmptyResult();
	iggy::runtime::RuntimePolicyPickupEffectFrameEntry entry;
	entry.effectIndex = 0;
	entry.result.status = iggy::runtime::RuntimePolicyPickupEffectStatus::TransferFailed;
	entry.result.pickup.transfer.add.plan.status = iggy::InventoryStackPolicy2DStatus::StackLimitExceeded;
	result.frame.pickup.failedCount = 1;
	result.frame.pickup.entries = { entry };

	const iggy::runtime::RuntimePolicyGameplayFrameReport report =
		iggy::runtime::RuntimePolicyGameplayFrameReporter {}.report(result);

	Expect(report.pickupFailedCount == 1, "over-capacity policy gameplay report should count pickup failure");
	Expect(report.pickedUpCount == 0, "over-capacity policy gameplay report should not count picked-up");
	Expect(report.inventoryEventCount == 0, "over-capacity policy gameplay report should have no inventory events");
	Expect(report.itemAddedEventCount == 0 && report.dropConsumedEventCount == 0 && report.itemPickedUpEventCount == 0, "over-capacity policy gameplay report should have no success event counts");
	Expect(report.pickup.entries.size() == 1, "over-capacity policy gameplay report should preserve nested pickup entry");
	if (!report.pickup.entries.empty())
		Expect(report.pickup.entries[0].result.pickup.transfer.add.plan.status == iggy::InventoryStackPolicy2DStatus::StackLimitExceeded, "over-capacity policy gameplay report should preserve nested stack diagnostics");
	Expect(HasEvent(report.events, iggy::runtime::RuntimePolicyGameplayFrameEvent::PickupFailed), "over-capacity policy gameplay report should emit pickup failed");
}

void TestMissingItemDefinitionReportsPolicyFailure()
{
	iggy::runtime::RuntimePolicyGameplayFrameResult result = EmptyResult();
	iggy::runtime::RuntimePolicyPickupEffectFrameEntry entry;
	entry.effectIndex = 0;
	entry.result.status = iggy::runtime::RuntimePolicyPickupEffectStatus::TransferFailed;
	entry.result.pickup.transfer.add.plan.status = iggy::InventoryStackPolicy2DStatus::ItemDefinitionNotFound;
	result.frame.pickup.failedCount = 1;
	result.frame.pickup.entries = { entry };

	const iggy::runtime::RuntimePolicyGameplayFrameReport report =
		iggy::runtime::RuntimePolicyGameplayFrameReporter {}.report(result);

	Expect(report.pickupFailedCount == 1, "missing-definition policy gameplay report should count pickup failure");
	Expect(report.inventoryEventCount == 0, "missing-definition policy gameplay report should have no inventory events");
	if (!report.pickup.entries.empty())
		Expect(report.pickup.entries[0].result.pickup.transfer.add.plan.status == iggy::InventoryStackPolicy2DStatus::ItemDefinitionNotFound, "missing-definition policy gameplay report should preserve nested definition diagnostics");
}

void TestPickupNotReadyReportsInventoryNotReadyEvent()
{
	iggy::runtime::RuntimePolicyGameplayFrameResult result = EmptyResult();
	const iggy::ResourceId dropId = Id("drop:far");
	result.frame.pickup.notReadyCount = 1;
	result.inventoryEvents.events = {
		iggy::pickupNotReadyInventoryEvent(dropId),
	};

	const iggy::runtime::RuntimePolicyGameplayFrameReport report =
		iggy::runtime::RuntimePolicyGameplayFrameReporter {}.report(result);

	Expect(report.pickupNotReadyCount == 1, "not-ready policy gameplay report should count pickup not ready");
	Expect(report.pickupNotReadyEventCount == 1, "not-ready policy gameplay report should count not-ready inventory event");
	Expect(report.inventoryEventCount == 1, "not-ready policy gameplay report should count inventory event");
	if (report.inventoryEvents.events.size() == 1)
		ExpectInventoryEvent(report.inventoryEvents.events[0], iggy::InventoryEvent2DType::PickupNotReady, {}, dropId, 0, "not-ready policy gameplay report should preserve drop id");
	Expect(HasEvent(report.events, iggy::runtime::RuntimePolicyGameplayFrameEvent::PickupNotReady), "not-ready policy gameplay report should emit pickup not ready");
}

void TestContextBlockedAndQueueRejectedFactsProjectWithNoInventoryEvents()
{
	iggy::runtime::RuntimePolicyGameplayFrameResult blocked = EmptyResult();
	blocked.frame.interaction.playerInput.report.blockedIntentCount = 1;
	iggy::runtime::RuntimePolicyGameplayFrameResult rejected = EmptyResult();
	rejected.frame.interaction.playerInput.report.rejectedIntentCount = 1;

	const iggy::runtime::RuntimePolicyGameplayFrameReport blockedReport =
		iggy::runtime::RuntimePolicyGameplayFrameReporter {}.report(blocked);
	const iggy::runtime::RuntimePolicyGameplayFrameReport rejectedReport =
		iggy::runtime::RuntimePolicyGameplayFrameReporter {}.report(rejected);

	Expect(blockedReport.blockedIntentCount == 1, "blocked policy gameplay report should project blocked intent count");
	Expect(blockedReport.inventoryEventCount == 0 && blockedReport.pickedUpCount == 0, "blocked policy gameplay report should have no inventory or pickup success");
	Expect(HasEvent(blockedReport.events, iggy::runtime::RuntimePolicyGameplayFrameEvent::PlayerIntentBlocked), "blocked policy gameplay report should emit blocked event");
	Expect(rejectedReport.rejectedIntentCount == 1, "queue-rejected policy gameplay report should project rejected intent count");
	Expect(rejectedReport.inventoryEventCount == 0 && rejectedReport.pickedUpCount == 0, "queue-rejected policy gameplay report should have no inventory or pickup success");
	Expect(HasEvent(rejectedReport.events, iggy::runtime::RuntimePolicyGameplayFrameEvent::PlayerIntentRejected), "queue-rejected policy gameplay report should emit rejected event");
}

void TestToggleAndPickupReportsInteractionAndInventoryChanged()
{
	iggy::runtime::RuntimePolicyGameplayFrameResult result = EmptyResult();
	result.frame.interaction.playerInput.report.acceptedCommandCount = 1;
	result.frame.interaction.application.appliedCount = 1;
	result.frame.interaction.application.mutated = true;
	result.frame.interaction.events.events = {
		iggy::targetToggledInteractionEvent(Id("target:door"), false),
	};
	result.frame.pickup.pickedUpCount = 1;
	result.frame.pickup.changed = true;
	result.inventoryEvents.events = {
		iggy::itemAddedInventoryEvent(Id("item:key"), 1),
		iggy::dropConsumedInventoryEvent(Id("drop:key")),
		iggy::itemPickedUpInventoryEvent(Id("drop:key"), Id("item:key"), 1),
	};

	const iggy::runtime::RuntimePolicyGameplayFrameReport report =
		iggy::runtime::RuntimePolicyGameplayFrameReporter {}.report(result);

	Expect(report.interactionChanged, "combo policy gameplay report should mark interaction changed");
	Expect(report.inventoryChanged, "combo policy gameplay report should mark inventory changed");
	Expect(report.interactionEventCount == 1, "combo policy gameplay report should count interaction event");
	Expect(report.pickedUpCount == 1, "combo policy gameplay report should count pickup");
	Expect(HasEvent(report.events, iggy::runtime::RuntimePolicyGameplayFrameEvent::InteractionChanged), "combo policy gameplay report should emit interaction changed");
	Expect(HasEvent(report.events, iggy::runtime::RuntimePolicyGameplayFrameEvent::InventoryChanged), "combo policy gameplay report should emit inventory changed");
	Expect(HasEvent(report.events, iggy::runtime::RuntimePolicyGameplayFrameEvent::InteractionEventRecorded), "combo policy gameplay report should emit interaction event recorded");
}

void TestMixedSummaryEventOrderIsDeterministic()
{
	iggy::runtime::RuntimePolicyGameplayFrameResult result = EmptyResult();
	result.frame.interaction.playerInput.report.acceptedCommandCount = 1;
	result.frame.interaction.playerInput.report.blockedIntentCount = 1;
	result.frame.interaction.playerInput.report.rejectedIntentCount = 1;
	result.frame.interaction.playerInput.report.queuedFrameCount = 1;
	result.frame.interaction.playerInput.report.tickResultCount = 1;
	result.frame.interaction.application.mutated = true;
	result.frame.interaction.events.events = {
		iggy::targetToggledInteractionEvent(Id("target:door"), false),
	};
	result.frame.pickup.pickedUpCount = 1;
	result.frame.pickup.notReadyCount = 1;
	result.frame.pickup.failedCount = 1;
	result.frame.pickup.changed = true;
	result.inventoryEvents.events = {
		iggy::itemAddedInventoryEvent(Id("item:potion"), 1),
	};

	const iggy::runtime::RuntimePolicyGameplayFrameReport report =
		iggy::runtime::RuntimePolicyGameplayFrameReporter {}.report(result);

	ExpectSummaryEventAt(report, 0, iggy::runtime::RuntimePolicyGameplayFrameEvent::PlayerCommandAccepted, "mixed policy gameplay report event order 0");
	ExpectSummaryEventAt(report, 1, iggy::runtime::RuntimePolicyGameplayFrameEvent::PlayerIntentBlocked, "mixed policy gameplay report event order 1");
	ExpectSummaryEventAt(report, 2, iggy::runtime::RuntimePolicyGameplayFrameEvent::PlayerIntentRejected, "mixed policy gameplay report event order 2");
	ExpectSummaryEventAt(report, 3, iggy::runtime::RuntimePolicyGameplayFrameEvent::CommandFrameQueued, "mixed policy gameplay report event order 3");
	ExpectSummaryEventAt(report, 4, iggy::runtime::RuntimePolicyGameplayFrameEvent::CommandRunnerRan, "mixed policy gameplay report event order 4");
	ExpectSummaryEventAt(report, 5, iggy::runtime::RuntimePolicyGameplayFrameEvent::InteractionChanged, "mixed policy gameplay report event order 5");
	ExpectSummaryEventAt(report, 6, iggy::runtime::RuntimePolicyGameplayFrameEvent::InteractionEventRecorded, "mixed policy gameplay report event order 6");
	ExpectSummaryEventAt(report, 7, iggy::runtime::RuntimePolicyGameplayFrameEvent::InventoryChanged, "mixed policy gameplay report event order 7");
	ExpectSummaryEventAt(report, 8, iggy::runtime::RuntimePolicyGameplayFrameEvent::ItemPickedUp, "mixed policy gameplay report event order 8");
	ExpectSummaryEventAt(report, 9, iggy::runtime::RuntimePolicyGameplayFrameEvent::PickupNotReady, "mixed policy gameplay report event order 9");
	ExpectSummaryEventAt(report, 10, iggy::runtime::RuntimePolicyGameplayFrameEvent::PickupFailed, "mixed policy gameplay report event order 10");
	ExpectSummaryEventAt(report, 11, iggy::runtime::RuntimePolicyGameplayFrameEvent::InventoryEventRecorded, "mixed policy gameplay report event order 11");
}

void TestReporterDoesNotMutateInputResult()
{
	iggy::runtime::RuntimePolicyGameplayFrameResult result = EmptyResult();
	result.frame.interaction.playerInput.report.acceptedCommandCount = 1;
	result.frame.pickup.pickedUpCount = 1;
	result.inventoryEvents.events = {
		iggy::itemAddedInventoryEvent(Id("item:potion"), 1),
	};
	const iggy::runtime::RuntimePolicyGameplayFrameResult before = result;

	(void)iggy::runtime::RuntimePolicyGameplayFrameReporter {}.report(result);

	Expect(result.frame.interaction.playerInput.report.acceptedCommandCount == before.frame.interaction.playerInput.report.acceptedCommandCount, "policy gameplay reporter should not mutate player report");
	Expect(result.frame.pickup.pickedUpCount == before.frame.pickup.pickedUpCount, "policy gameplay reporter should not mutate pickup result");
	Expect(result.inventoryEvents.events.size() == before.inventoryEvents.events.size(), "policy gameplay reporter should not mutate inventory events");
}

} // namespace

int main()
{
	TestEmptyMoveOnlyReportProjectsPlayerFactsOnly();
	TestSuccessfulPolicyPickupReportsCountsAndEvents();
	TestExistingStackExactlyToMaxPreservesEventPayloadCount();
	TestOverCapacityReportsPolicyFailureWithoutSuccessEvents();
	TestMissingItemDefinitionReportsPolicyFailure();
	TestPickupNotReadyReportsInventoryNotReadyEvent();
	TestContextBlockedAndQueueRejectedFactsProjectWithNoInventoryEvents();
	TestToggleAndPickupReportsInteractionAndInventoryChanged();
	TestMixedSummaryEventOrderIsDeterministic();
	TestReporterDoesNotMutateInputResult();

	return Failures;
}
