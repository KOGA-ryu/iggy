#include <cstdlib>

#include "scene/inventory/InventoryEvent2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

bool SameEvent(const iggy::InventoryEvent2D &actual, const iggy::InventoryEvent2D &expected)
{
	return actual.type == expected.type
		&& actual.itemId == expected.itemId
		&& actual.dropId == expected.dropId
		&& actual.count == expected.count;
}

void TestItemAddedFactoryPreservesPayload()
{
	const iggy::ResourceId itemId { "item:potion" };
	const iggy::InventoryEvent2D event = iggy::itemAddedInventoryEvent(itemId, 3);

	Expect(event.type == iggy::InventoryEvent2DType::ItemAdded, "item added event should preserve type");
	Expect(event.itemId == itemId, "item added event should preserve item id");
	Expect(event.count == 3, "item added event should preserve count");
	Expect(event.dropId.empty(), "item added event should leave drop id defaulted");
}

void TestItemPickedUpFactoryPreservesPayload()
{
	const iggy::ResourceId dropId { "drop:potion" };
	const iggy::ResourceId itemId { "item:potion" };
	const iggy::InventoryEvent2D event = iggy::itemPickedUpInventoryEvent(dropId, itemId, 2);

	Expect(event.type == iggy::InventoryEvent2DType::ItemPickedUp, "item picked up event should preserve type");
	Expect(event.dropId == dropId, "item picked up event should preserve drop id");
	Expect(event.itemId == itemId, "item picked up event should preserve item id");
	Expect(event.count == 2, "item picked up event should preserve count");
}

void TestDropConsumedFactoryPreservesPayload()
{
	const iggy::ResourceId dropId { "drop:key" };
	const iggy::InventoryEvent2D event = iggy::dropConsumedInventoryEvent(dropId);

	Expect(event.type == iggy::InventoryEvent2DType::DropConsumed, "drop consumed event should preserve type");
	Expect(event.dropId == dropId, "drop consumed event should preserve drop id");
	Expect(event.itemId.empty(), "drop consumed event should leave item id defaulted");
	Expect(event.count == 0, "drop consumed event should leave count defaulted");
}

void TestPickupNotReadyFactoryPreservesPayload()
{
	const iggy::ResourceId dropId { "drop:far" };
	const iggy::InventoryEvent2D event = iggy::pickupNotReadyInventoryEvent(dropId);

	Expect(event.type == iggy::InventoryEvent2DType::PickupNotReady, "pickup not ready event should preserve type");
	Expect(event.dropId == dropId, "pickup not ready event should preserve drop id");
	Expect(event.itemId.empty(), "pickup not ready event should leave item id defaulted");
	Expect(event.count == 0, "pickup not ready event should leave count defaulted");
}

void TestInventoryAddFailedFactoryPreservesPayload()
{
	const iggy::ResourceId itemId { "item:invalid" };
	const iggy::InventoryEvent2D event = iggy::inventoryAddFailedEvent(itemId, 4);

	Expect(event.type == iggy::InventoryEvent2DType::InventoryAddFailed, "inventory add failed event should preserve type");
	Expect(event.itemId == itemId, "inventory add failed event should preserve item id");
	Expect(event.count == 4, "inventory add failed event should preserve count");
	Expect(event.dropId.empty(), "inventory add failed event should leave drop id defaulted");
}

void TestRecorderAppendsEventsInOrder()
{
	const iggy::InventoryEvent2D first = iggy::itemAddedInventoryEvent(iggy::ResourceId { "item:potion" }, 1);
	const iggy::InventoryEvent2D second = iggy::itemPickedUpInventoryEvent(iggy::ResourceId { "drop:potion" }, iggy::ResourceId { "item:potion" }, 1);
	const iggy::InventoryEvent2D third = iggy::dropConsumedInventoryEvent(iggy::ResourceId { "drop:potion" });
	iggy::InventoryEventRecorder2D recorder;

	Expect(recorder.events.empty(), "inventory event recorder should start empty");
	iggy::recordInventoryEvent(recorder, first);
	iggy::recordInventoryEvent(recorder, second);
	iggy::recordInventoryEvent(recorder, third);

	Expect(recorder.events.size() == 3, "inventory event recorder should append events");
	if (recorder.events.size() == 3) {
		Expect(SameEvent(recorder.events[0], first), "inventory event recorder should preserve first event");
		Expect(SameEvent(recorder.events[1], second), "inventory event recorder should preserve second event");
		Expect(SameEvent(recorder.events[2], third), "inventory event recorder should preserve third event");
	}
}

void TestRecorderCopiesEventData()
{
	iggy::InventoryEvent2D source = iggy::itemPickedUpInventoryEvent(
		iggy::ResourceId { "drop:copy" },
		iggy::ResourceId { "item:copy" },
		5);
	iggy::InventoryEventRecorder2D recorder;

	iggy::recordInventoryEvent(recorder, source);
	source.dropId = iggy::ResourceId { "drop:changed" };
	source.itemId = iggy::ResourceId { "item:changed" };
	source.count = 9;

	Expect(recorder.events.size() == 1, "inventory event recorder copy setup should record one event");
	if (recorder.events.size() == 1) {
		Expect(recorder.events[0].dropId == iggy::ResourceId { "drop:copy" }, "inventory event recorder should copy drop id");
		Expect(recorder.events[0].itemId == iggy::ResourceId { "item:copy" }, "inventory event recorder should copy item id");
		Expect(recorder.events[0].count == 5, "inventory event recorder should copy count");
	}
}

void TestEmptyPayloadsArePreservedAsData()
{
	const iggy::InventoryEvent2D added = iggy::itemAddedInventoryEvent({}, 0);
	const iggy::InventoryEvent2D pickedUp = iggy::itemPickedUpInventoryEvent({}, {}, 0);
	const iggy::InventoryEvent2D consumed = iggy::dropConsumedInventoryEvent({});
	const iggy::InventoryEvent2D notReady = iggy::pickupNotReadyInventoryEvent({});
	const iggy::InventoryEvent2D failed = iggy::inventoryAddFailedEvent({}, 0);

	Expect(added.itemId.empty() && added.count == 0, "item added event should preserve empty id and zero count");
	Expect(pickedUp.dropId.empty() && pickedUp.itemId.empty() && pickedUp.count == 0, "item picked up event should preserve empty ids and zero count");
	Expect(consumed.dropId.empty(), "drop consumed event should preserve empty drop id");
	Expect(notReady.dropId.empty(), "pickup not ready event should preserve empty drop id");
	Expect(failed.itemId.empty() && failed.count == 0, "inventory add failed event should preserve empty id and zero count");
}

} // namespace

int main()
{
	TestItemAddedFactoryPreservesPayload();
	TestItemPickedUpFactoryPreservesPayload();
	TestDropConsumedFactoryPreservesPayload();
	TestPickupNotReadyFactoryPreservesPayload();
	TestInventoryAddFailedFactoryPreservesPayload();
	TestRecorderAppendsEventsInOrder();
	TestRecorderCopiesEventData();
	TestEmptyPayloadsArePreservedAsData();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
