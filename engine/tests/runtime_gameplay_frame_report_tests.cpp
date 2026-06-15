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

iggy::runtime::RuntimeGameplayFrameResult ResultWithNestedReport(
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport nested)
{
	iggy::runtime::RuntimeGameplayFrameResult result;
	result.report = nested;
	result.inventoryEvents = nested.inventoryEvents;
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
	Expect(report.inventoryEventCount == 0, "empty gameplay report should have zero inventory events");
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
	const iggy::ResourceId dropId { "drop:potion" };
	const iggy::ResourceId itemId { "item:potion" };
	nested.acceptedCommandCount = 1;
	nested.pickedUpCount = 2;
	nested.inventoryChanged = true;
	nested.inventoryEvents.events = {
		iggy::itemAddedInventoryEvent(itemId, 3),
		iggy::dropConsumedInventoryEvent(dropId),
		iggy::itemPickedUpInventoryEvent(dropId, itemId, 3),
	};

	const iggy::runtime::RuntimeGameplayFrameReport report =
		iggy::runtime::RuntimeGameplayFrameReporter {}.report(ResultWithNestedReport(nested));

	Expect(report.pickedUpCount == 2, "pickup gameplay report should project picked-up count");
	Expect(report.inventoryChanged, "pickup gameplay report should project inventory changed flag");
	Expect(report.inventoryEventCount == 3, "pickup gameplay report should project inventory event count");
	Expect(report.itemAddedEventCount == 1, "pickup gameplay report should count item added event");
	Expect(report.dropConsumedEventCount == 1, "pickup gameplay report should count drop consumed event");
	Expect(report.itemPickedUpEventCount == 1, "pickup gameplay report should count item picked up event");
	Expect(report.pickupNotReadyEventCount == 0 && report.inventoryAddFailedEventCount == 0, "pickup gameplay report should count no not-ready/failure events");
	if (report.inventoryEvents.events.size() == 3) {
		ExpectInventoryEvent(report.inventoryEvents.events[0], iggy::InventoryEvent2DType::ItemAdded, itemId, {}, 3, "pickup gameplay report should preserve item added event");
		ExpectInventoryEvent(report.inventoryEvents.events[1], iggy::InventoryEvent2DType::DropConsumed, {}, dropId, 0, "pickup gameplay report should preserve drop consumed event");
		ExpectInventoryEvent(report.inventoryEvents.events[2], iggy::InventoryEvent2DType::ItemPickedUp, itemId, dropId, 3, "pickup gameplay report should preserve item picked up event");
	}
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::InventoryChanged), "pickup gameplay report should emit inventory changed event");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::ItemPickedUp), "pickup gameplay report should emit item picked up event");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::InventoryEventRecorded), "pickup gameplay report should emit inventory event recorded event");
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
	nested.inventoryEvents.events = {};

	const iggy::runtime::RuntimeGameplayFrameReport report =
		iggy::runtime::RuntimeGameplayFrameReporter {}.report(ResultWithNestedReport(nested));

	Expect(report.rejectedIntentCount == 1, "queue-rejected gameplay report should project rejected intent count");
	Expect(report.inventoryEventCount == 0, "queue-rejected gameplay report should project zero inventory events");
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
	nested.inventoryEventCount = 5;
	nested.itemAddedEventCount = 1;
	nested.itemPickedUpEventCount = 1;
	nested.dropConsumedEventCount = 1;
	nested.pickupNotReadyEventCount = 1;
	nested.inventoryAddFailedEventCount = 1;
	nested.inventoryEvents.events = {
		iggy::itemAddedInventoryEvent(iggy::ResourceId { "item:one" }, 1),
		iggy::dropConsumedInventoryEvent(iggy::ResourceId { "drop:one" }),
		iggy::itemPickedUpInventoryEvent(iggy::ResourceId { "drop:one" }, iggy::ResourceId { "item:one" }, 1),
		iggy::pickupNotReadyInventoryEvent(iggy::ResourceId { "drop:missing" }),
		iggy::inventoryAddFailedEvent(iggy::ResourceId { "item:bad" }, 2),
	};
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
	Expect(report.input.inventoryEventCount == 5, "gameplay report should preserve nested inventory event count");
	Expect(report.inventoryEventCount == 5, "gameplay report should project inventory event count");
	Expect(report.itemAddedEventCount == 1, "gameplay report should project item added event count");
	Expect(report.dropConsumedEventCount == 1, "gameplay report should project drop consumed event count");
	Expect(report.itemPickedUpEventCount == 1, "gameplay report should project item picked up event count");
	Expect(report.pickupNotReadyEventCount == 1, "gameplay report should project pickup not-ready inventory event count");
	Expect(report.inventoryAddFailedEventCount == 1, "gameplay report should project inventory add failed event count");
	Expect(report.input.interaction.playerInput.queuedFrameCount == 10, "gameplay report should preserve nested player input report");
	Expect(report.input.interaction.playerInput.tickResultCount == 11, "gameplay report should preserve nested tick count");
	Expect(report.pickupNotReadyCount == 8, "gameplay report should project pickup not-ready count");
	Expect(report.pickupFailedCount == 9, "gameplay report should project pickup failed count");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::PickupNotReady), "gameplay report should emit pickup not-ready event");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::PickupFailed), "gameplay report should emit pickup failed event");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::InventoryEventRecorded), "gameplay report should emit inventory event recorded event");
}

void TestReporterProjectsNpcMovementFactsAndEvents()
{
	iggy::runtime::RuntimeGameplayFrameResult result;
	result.npcMovement.report.movedCount = 1;
	result.npcMovement.report.blockedCount = 1;
	result.npcMovement.report.rejectedCount = 1;
	result.npcMovement.report.missingActorCount = 1;
	result.npcMovement.report.changedCount = 1;
	result.npcMovement.report.apply.changed = true;
	result.npcMovement.report.dirtyTiles = {
		iggy::TileCoord { 1, 2 },
		iggy::TileCoord { 2, 2 },
	};
	result.npcMovement.report.needsOccupancyRebuild = true;
	result.npcMovement.report.needsAiMapQueryRefresh = true;
	result.npcMovement.report.needsInteractionRefresh = true;
	result.npcMovement.report.needsRenderRefresh = true;
	result.npcMovement.report.needsVisibilityRefresh = true;
	result.npcMovement.report.events = {
		iggy::NpcActorMovementFrameEvent2D::MovementApplied,
		iggy::NpcActorMovementFrameEvent2D::MovementBlocked,
		iggy::NpcActorMovementFrameEvent2D::MovementRejected,
		iggy::NpcActorMovementFrameEvent2D::ActorNotFound,
		iggy::NpcActorMovementFrameEvent2D::MovementFrameChanged,
	};

	const iggy::runtime::RuntimeGameplayFrameReport report =
		iggy::runtime::RuntimeGameplayFrameReporter {}.report(result);

	Expect(report.npcMovedCount == 1, "gameplay report should project NPC moved count");
	Expect(report.npcBlockedMovementCount == 1, "gameplay report should project NPC blocked count");
	Expect(report.npcRejectedMovementCount == 1, "gameplay report should project NPC rejected count");
	Expect(report.npcMissingActorMovementCount == 1, "gameplay report should project NPC missing actor count");
	Expect(report.npcMovementDirtyTileCount == 2, "gameplay report should project NPC dirty tile count");
	Expect(report.npcMovementChanged, "gameplay report should project NPC movement changed");
	Expect(report.npcMovementNeedsOccupancyRebuild, "gameplay report should project NPC occupancy refresh");
	Expect(report.npcMovementNeedsAiMapQueryRefresh, "gameplay report should project NPC AI map refresh");
	Expect(report.npcMovementNeedsInteractionRefresh, "gameplay report should project NPC interaction refresh");
	Expect(report.npcMovementNeedsRenderRefresh, "gameplay report should project NPC render refresh");
	Expect(report.npcMovementNeedsVisibilityRefresh, "gameplay report should project NPC visibility refresh");
	Expect(report.npcMovement.dirtyTiles.size() == 2, "gameplay report should preserve nested NPC movement report");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::NpcMovementChanged), "gameplay report should emit NPC changed event");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::NpcMovementBlocked), "gameplay report should emit NPC blocked event");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::NpcMovementRejected), "gameplay report should emit NPC rejected event");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::NpcMovementActorMissing), "gameplay report should emit NPC missing actor event");
	Expect(HasEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::NpcMovementRefreshNeeded), "gameplay report should emit NPC refresh event");
}

void TestReporterDoesNotMutateInputResult()
{
	iggy::runtime::RuntimeGameplayFrameResult result;
	result.report.acceptedCommandCount = 1;
	result.report.inventoryChanged = true;
	result.inventoryEvents.events = {
		iggy::itemAddedInventoryEvent(iggy::ResourceId { "item:potion" }, 1),
	};
	const iggy::runtime::RuntimeGameplayFrameResult before = result;

	(void)iggy::runtime::RuntimeGameplayFrameReporter {}.report(result);

	Expect(result.report.acceptedCommandCount == before.report.acceptedCommandCount, "gameplay reporter should not mutate accepted command count");
	Expect(result.report.inventoryChanged == before.report.inventoryChanged, "gameplay reporter should not mutate inventory changed flag");
	Expect(result.inventoryEvents.events.size() == before.inventoryEvents.events.size(), "gameplay reporter should not mutate inventory events");
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
	TestReporterProjectsNpcMovementFactsAndEvents();
	TestReporterDoesNotMutateInputResult();

	return Failures;
}
