#include "scene/inventory/InventoryEvent2D.hpp"

#include <utility>

namespace iggy {

InventoryEvent2D itemAddedInventoryEvent(ResourceId itemId, std::uint32_t count)
{
	InventoryEvent2D event;
	event.type = InventoryEvent2DType::ItemAdded;
	event.itemId = std::move(itemId);
	event.count = count;
	return event;
}

InventoryEvent2D itemPickedUpInventoryEvent(ResourceId dropId, ResourceId itemId, std::uint32_t count)
{
	InventoryEvent2D event;
	event.type = InventoryEvent2DType::ItemPickedUp;
	event.dropId = std::move(dropId);
	event.itemId = std::move(itemId);
	event.count = count;
	return event;
}

InventoryEvent2D dropConsumedInventoryEvent(ResourceId dropId)
{
	InventoryEvent2D event;
	event.type = InventoryEvent2DType::DropConsumed;
	event.dropId = std::move(dropId);
	return event;
}

InventoryEvent2D pickupNotReadyInventoryEvent(ResourceId dropId)
{
	InventoryEvent2D event;
	event.type = InventoryEvent2DType::PickupNotReady;
	event.dropId = std::move(dropId);
	return event;
}

InventoryEvent2D inventoryAddFailedEvent(ResourceId itemId, std::uint32_t count)
{
	InventoryEvent2D event;
	event.type = InventoryEvent2DType::InventoryAddFailed;
	event.itemId = std::move(itemId);
	event.count = count;
	return event;
}

void recordInventoryEvent(InventoryEventRecorder2D &recorder, const InventoryEvent2D &event)
{
	recorder.events.push_back(event);
}

} // namespace iggy
