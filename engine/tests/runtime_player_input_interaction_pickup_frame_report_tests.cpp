#include <cstdlib>
#include <vector>

#include "runtime/RuntimePlayerInputInteractionPickupFrameReporter.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

bool SameSummaryEvents(
	const std::vector<iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent> &actual,
	const std::vector<iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index] != expected[index])
			return false;
	}
	return true;
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

iggy::runtime::RuntimeInteractionEffectApplyFrameEntry InteractionEntry(
	iggy::runtime::RuntimeInteractionEffectApplyStatus status,
	std::size_t deferredCount = 0)
{
	iggy::runtime::RuntimeInteractionEffectApplyFrameEntry entry;
	entry.result.status = status;
	entry.result.application.deferredCount = deferredCount;
	return entry;
}

iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult Result(
	std::size_t acceptedCommands,
	std::size_t blockedIntents,
	std::size_t rejectedIntents,
	std::size_t queuedFrames,
	std::size_t tickResults)
{
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result;
	result.interaction.playerInput.report.acceptedCommandCount = acceptedCommands;
	result.interaction.playerInput.report.blockedIntentCount = blockedIntents;
	result.interaction.playerInput.report.rejectedIntentCount = rejectedIntents;
	result.interaction.playerInput.report.queuedFrameCount = queuedFrames;
	result.interaction.playerInput.report.tickResultCount = tickResults;
	return result;
}

void TestNoPickupEffectsReportsInteractionFactsOnly()
{
	const iggy::ResourceId targetId { "target:toggled" };
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result = Result(1, 0, 0, 1, 1);
	result.interaction.application.status = iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Applied;
	result.interaction.application.appliedCount = 1;
	result.interaction.application.entries = { InteractionEntry(iggy::runtime::RuntimeInteractionEffectApplyStatus::Applied) };
	result.interaction.application.mutated = true;
	result.interaction.events.events = { iggy::targetToggledInteractionEvent(targetId, false) };
	result.pickup.status = iggy::runtime::RuntimePickupEffectFrameStatus::NoPickupEffects;

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameReporter {}.report(result);

	Expect(report.acceptedCommandCount == 1, "no-pickup report should count accepted command");
	Expect(report.interactionAppliedCount == 1, "no-pickup report should count interaction application");
	Expect(report.interactionEventCount == 1, "no-pickup report should count interaction event");
	Expect(report.pickedUpCount == 0 && report.pickupNotReadyCount == 0 && report.pickupFailedCount == 0, "no-pickup report should count no pickup facts");
	Expect(report.inventoryEventCount == 0, "no-pickup report should count no inventory events");
	Expect(report.interactionMutated, "no-pickup report should mirror interaction mutation");
	Expect(!report.inventoryChanged, "no-pickup report should not mark inventory changed");
	Expect(report.pickup.status == iggy::runtime::RuntimePickupEffectFrameStatus::NoPickupEffects, "no-pickup report should preserve pickup result");
	Expect(SameSummaryEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::CommandRunnerRan,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::InteractionEffectApplied,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::InteractionEventRecorded,
	}), "no-pickup report should emit interaction-only summary events");
}

void TestSuccessfulPickupReportsItemPickedUp()
{
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result = Result(1, 0, 0, 1, 1);
	const iggy::ResourceId dropId { "drop:potion" };
	const iggy::ResourceId itemId { "item:potion" };
	result.pickup.status = iggy::runtime::RuntimePickupEffectFrameStatus::PickedUp;
	result.pickup.pickedUpCount = 1;
	result.pickup.changed = true;
	result.inventoryEvents.events = {
		iggy::itemAddedInventoryEvent(itemId, 2),
		iggy::dropConsumedInventoryEvent(dropId),
		iggy::itemPickedUpInventoryEvent(dropId, itemId, 2),
	};

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameReporter {}.report(result);

	Expect(report.acceptedCommandCount == 1, "successful pickup report should count accepted command");
	Expect(report.pickedUpCount == 1, "successful pickup report should count pickup");
	Expect(report.pickupNotReadyCount == 0 && report.pickupFailedCount == 0, "successful pickup report should count no pickup problems");
	Expect(report.inventoryEventCount == 3, "successful pickup report should count inventory events");
	Expect(report.itemAddedEventCount == 1, "successful pickup report should count item added event");
	Expect(report.dropConsumedEventCount == 1, "successful pickup report should count drop consumed event");
	Expect(report.itemPickedUpEventCount == 1, "successful pickup report should count item picked up event");
	Expect(report.pickupNotReadyEventCount == 0 && report.inventoryAddFailedEventCount == 0, "successful pickup report should count no failure/not-ready events");
	if (report.inventoryEvents.events.size() == 3) {
		ExpectInventoryEvent(report.inventoryEvents.events[0], iggy::InventoryEvent2DType::ItemAdded, itemId, {}, 2, "successful pickup report should preserve item added event");
		ExpectInventoryEvent(report.inventoryEvents.events[1], iggy::InventoryEvent2DType::DropConsumed, {}, dropId, 0, "successful pickup report should preserve drop consumed event");
		ExpectInventoryEvent(report.inventoryEvents.events[2], iggy::InventoryEvent2DType::ItemPickedUp, itemId, dropId, 2, "successful pickup report should preserve item picked up event");
	}
	Expect(report.inventoryChanged, "successful pickup report should mark inventory changed");
	Expect(SameSummaryEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::CommandRunnerRan,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::ItemPickedUp,
	}), "successful pickup report should emit item picked up event");
}

void TestExistingStackPickupAlsoReportsItemPickedUp()
{
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result = Result(1, 0, 0, 1, 1);
	result.pickup.status = iggy::runtime::RuntimePickupEffectFrameStatus::PickedUp;
	result.pickup.pickedUpCount = 1;
	result.pickup.changed = true;
	result.pickup.entries.resize(1);
	result.pickup.entries[0].result.status = iggy::runtime::RuntimePickupEffectStatus::PickedUp;
	result.inventoryEvents.events = {
		iggy::itemAddedInventoryEvent(iggy::ResourceId { "item:potion" }, 3),
		iggy::dropConsumedInventoryEvent(iggy::ResourceId { "drop:potion" }),
		iggy::itemPickedUpInventoryEvent(iggy::ResourceId { "drop:potion" }, iggy::ResourceId { "item:potion" }, 3),
	};

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameReporter {}.report(result);

	Expect(report.pickedUpCount == 1, "existing-stack pickup report should count pickup");
	Expect(report.inventoryChanged, "existing-stack pickup report should mark inventory changed");
	Expect(report.pickup.entries.size() == 1, "existing-stack pickup report should preserve pickup entries");
	Expect(report.inventoryEventCount == 3, "existing-stack pickup report should count inventory events");
	Expect(report.itemAddedEventCount == 1 && report.itemPickedUpEventCount == 1 && report.dropConsumedEventCount == 1, "existing-stack pickup report should count typed inventory events");
	if (report.inventoryEvents.events.size() == 3) {
		ExpectInventoryEvent(report.inventoryEvents.events[0], iggy::InventoryEvent2DType::ItemAdded, iggy::ResourceId { "item:potion" }, {}, 3, "existing-stack pickup report should preserve requested add count");
		ExpectInventoryEvent(report.inventoryEvents.events[2], iggy::InventoryEvent2DType::ItemPickedUp, iggy::ResourceId { "item:potion" }, iggy::ResourceId { "drop:potion" }, 3, "existing-stack pickup report should preserve requested pickup count");
	}
	Expect(SameSummaryEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::CommandRunnerRan,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::ItemPickedUp,
	}), "existing-stack pickup report should emit item picked up event");
}

void TestPickupNotReadyReportsWithoutInventoryChanged()
{
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result = Result(1, 0, 0, 1, 1);
	result.pickup.status = iggy::runtime::RuntimePickupEffectFrameStatus::PickupNotReady;
	result.pickup.notReadyCount = 2;
	result.inventoryEvents.events = {
		iggy::pickupNotReadyInventoryEvent(iggy::ResourceId { "drop:missing" }),
		iggy::pickupNotReadyInventoryEvent(iggy::ResourceId { "drop:far" }),
	};

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameReporter {}.report(result);

	Expect(report.pickedUpCount == 0, "not-ready pickup report should count no pickups");
	Expect(report.pickupNotReadyCount == 2, "not-ready pickup report should count not-ready pickups");
	Expect(report.pickupFailedCount == 0, "not-ready pickup report should count no failures");
	Expect(report.inventoryEventCount == 2, "not-ready pickup report should count inventory events");
	Expect(report.pickupNotReadyEventCount == 2, "not-ready pickup report should count not-ready inventory events");
	Expect(report.itemAddedEventCount == 0 && report.itemPickedUpEventCount == 0 && report.dropConsumedEventCount == 0, "not-ready pickup report should count no successful inventory events");
	if (report.inventoryEvents.events.size() == 2) {
		ExpectInventoryEvent(report.inventoryEvents.events[0], iggy::InventoryEvent2DType::PickupNotReady, {}, iggy::ResourceId { "drop:missing" }, 0, "not-ready pickup report should preserve first not-ready event");
		ExpectInventoryEvent(report.inventoryEvents.events[1], iggy::InventoryEvent2DType::PickupNotReady, {}, iggy::ResourceId { "drop:far" }, 0, "not-ready pickup report should preserve second not-ready event");
	}
	Expect(!report.inventoryChanged, "not-ready pickup report should not mark inventory changed");
	Expect(SameSummaryEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::CommandRunnerRan,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::PickupNotReady,
	}), "not-ready pickup report should emit not-ready summary event");
}

void TestPickupFailureReportsFailed()
{
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result = Result(1, 0, 0, 1, 1);
	result.pickup.status = iggy::runtime::RuntimePickupEffectFrameStatus::Failed;
	result.pickup.failedCount = 1;
	result.inventoryEvents.events = {
		iggy::inventoryAddFailedEvent(iggy::ResourceId { "item:bad" }, 2),
	};

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameReporter {}.report(result);

	Expect(report.pickupFailedCount == 1, "failed pickup report should count failure");
	Expect(report.pickedUpCount == 0 && report.pickupNotReadyCount == 0, "failed pickup report should count no other pickup facts");
	Expect(report.inventoryEventCount == 1, "failed pickup report should count inventory event");
	Expect(report.inventoryAddFailedEventCount == 1, "failed pickup report should count inventory add failure event");
	Expect(report.itemAddedEventCount == 0 && report.dropConsumedEventCount == 0 && report.itemPickedUpEventCount == 0, "failed pickup report should count no successful inventory events");
	if (report.inventoryEvents.events.size() == 1)
		ExpectInventoryEvent(report.inventoryEvents.events[0], iggy::InventoryEvent2DType::InventoryAddFailed, iggy::ResourceId { "item:bad" }, {}, 2, "failed pickup report should preserve inventory add failure event");
	Expect(!report.inventoryChanged, "failed pickup report should not mark inventory changed by default");
	Expect(SameSummaryEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::CommandRunnerRan,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::PickupFailed,
	}), "failed pickup report should emit failed summary event");
}

void TestContextBlockedAndQueueRejectedReportNoPickupCounts()
{
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult blocked = Result(0, 1, 0, 1, 1);
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult rejected = Result(1, 0, 0, 0, 0);
	rejected.interaction.playerInput.report.status = iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected;

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport blockedReport =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameReporter {}.report(blocked);
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport rejectedReport =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameReporter {}.report(rejected);

	Expect(blockedReport.blockedIntentCount == 1, "blocked pickup report should count blocked intent");
	Expect(blockedReport.pickedUpCount == 0 && blockedReport.pickupNotReadyCount == 0 && blockedReport.pickupFailedCount == 0, "blocked pickup report should count no pickup facts");
	Expect(blockedReport.inventoryEventCount == 0, "blocked pickup report should count no inventory events");
	Expect(!blockedReport.inventoryChanged, "blocked pickup report should not mark inventory changed");
	Expect(SameSummaryEvents(blockedReport.events, {
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::PlayerIntentBlocked,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::CommandRunnerRan,
	}), "blocked pickup report should emit blocked/player events only");
	Expect(rejectedReport.interaction.playerInput.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "queue-rejected pickup report should preserve nested queue rejection");
	Expect(rejectedReport.pickedUpCount == 0 && rejectedReport.pickupNotReadyCount == 0 && rejectedReport.pickupFailedCount == 0, "queue-rejected pickup report should count no pickup facts");
	Expect(rejectedReport.inventoryEventCount == 0, "queue-rejected pickup report should count no inventory events");
	Expect(SameSummaryEvents(rejectedReport.events, {
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::PlayerCommandAccepted,
	}), "queue-rejected pickup report should emit accepted mapping fact only");
}

void TestToggleTargetAndPickupReportsBothInteractionAndPickupFacts()
{
	const iggy::ResourceId targetId { "target:combo" };
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result = Result(1, 0, 0, 1, 1);
	result.interaction.application.status = iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Applied;
	result.interaction.application.appliedCount = 1;
	result.interaction.application.entries = { InteractionEntry(iggy::runtime::RuntimeInteractionEffectApplyStatus::Applied, 1) };
	result.interaction.application.mutated = true;
	result.interaction.events.events = { iggy::targetToggledInteractionEvent(targetId, false) };
	result.pickup.status = iggy::runtime::RuntimePickupEffectFrameStatus::PickedUp;
	result.pickup.pickedUpCount = 1;
	result.pickup.changed = true;
	result.inventoryEvents.events = {
		iggy::itemAddedInventoryEvent(iggy::ResourceId { "item:key" }, 1),
		iggy::dropConsumedInventoryEvent(iggy::ResourceId { "drop:key" }),
		iggy::itemPickedUpInventoryEvent(iggy::ResourceId { "drop:key" }, iggy::ResourceId { "item:key" }, 1),
	};

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameReporter {}.report(result);

	Expect(report.interactionAppliedCount == 1, "combo pickup report should count interaction application");
	Expect(report.interactionDeferredCount == 1, "combo pickup report should count deferred pickup effect");
	Expect(report.interactionEventCount == 1, "combo pickup report should count interaction event");
	Expect(report.pickedUpCount == 1, "combo pickup report should count pickup");
	Expect(report.inventoryEventCount == 3, "combo pickup report should count inventory events");
	Expect(report.itemAddedEventCount == 1 && report.dropConsumedEventCount == 1 && report.itemPickedUpEventCount == 1, "combo pickup report should count typed pickup inventory events");
	Expect(report.interactionMutated, "combo pickup report should mark interaction mutation");
	Expect(report.inventoryChanged, "combo pickup report should mark inventory changed");
	Expect(SameSummaryEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::CommandRunnerRan,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::InteractionEffectApplied,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::InteractionEffectDeferred,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::InteractionEventRecorded,
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::ItemPickedUp,
	}), "combo pickup report should emit deterministic combined summary events");
}

void TestReporterPreservesNestedResultsAndDoesNotMutateInput()
{
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result = Result(2, 1, 1, 1, 1);
	result.interaction.application.status = iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Applied;
	result.interaction.application.appliedCount = 1;
	result.interaction.application.mutated = true;
	result.pickup.status = iggy::runtime::RuntimePickupEffectFrameStatus::PickedUp;
	result.pickup.pickedUpCount = 1;
	result.pickup.changed = true;
	result.inventoryEvents.events = {
		iggy::itemAddedInventoryEvent(iggy::ResourceId { "item:potion" }, 2),
	};
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult before = result;

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport report =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameReporter {}.report(result);

	Expect(report.interaction.application.appliedCount == 1, "preservation pickup report should copy nested interaction application");
	Expect(report.pickup.pickedUpCount == 1, "preservation pickup report should copy nested pickup result");
	Expect(report.inventoryEvents.events.size() == 1, "preservation pickup report should copy inventory events");
	Expect(report.inventoryEventCount == 1 && report.itemAddedEventCount == 1, "preservation pickup report should count copied inventory events");
	Expect(result.interaction.application.appliedCount == before.interaction.application.appliedCount, "pickup reporter should not mutate interaction application count");
	Expect(result.interaction.application.mutated == before.interaction.application.mutated, "pickup reporter should not mutate interaction mutation");
	Expect(result.pickup.pickedUpCount == before.pickup.pickedUpCount, "pickup reporter should not mutate pickup count");
	Expect(result.pickup.changed == before.pickup.changed, "pickup reporter should not mutate pickup changed flag");
	Expect(result.inventoryEvents.events.size() == before.inventoryEvents.events.size(), "pickup reporter should not mutate inventory events");
}

} // namespace

int main()
{
	TestNoPickupEffectsReportsInteractionFactsOnly();
	TestSuccessfulPickupReportsItemPickedUp();
	TestExistingStackPickupAlsoReportsItemPickedUp();
	TestPickupNotReadyReportsWithoutInventoryChanged();
	TestPickupFailureReportsFailed();
	TestContextBlockedAndQueueRejectedReportNoPickupCounts();
	TestToggleTargetAndPickupReportsBothInteractionAndPickupFacts();
	TestReporterPreservesNestedResultsAndDoesNotMutateInput();

	return Failures;
}
