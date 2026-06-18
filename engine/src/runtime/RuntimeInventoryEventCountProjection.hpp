#pragma once

#include <cstddef>

#include "scene/inventory/InventoryEvent2D.hpp"

namespace iggy::runtime {

struct RuntimeInventoryEventCountProjection {
	std::size_t inventoryEventCount = 0;
	std::size_t itemAddedEventCount = 0;
	std::size_t itemPickedUpEventCount = 0;
	std::size_t dropConsumedEventCount = 0;
	std::size_t pickupNotReadyEventCount = 0;
	std::size_t inventoryAddFailedEventCount = 0;
};

[[nodiscard]] inline RuntimeInventoryEventCountProjection
projectRuntimeInventoryEventCounts(const InventoryEventRecorder2D &events)
{
	RuntimeInventoryEventCountProjection projection;
	projection.inventoryEventCount = events.events.size();
	for (const InventoryEvent2D &event : events.events) {
		switch (event.type) {
		case InventoryEvent2DType::ItemAdded:
			++projection.itemAddedEventCount;
			break;
		case InventoryEvent2DType::ItemPickedUp:
			++projection.itemPickedUpEventCount;
			break;
		case InventoryEvent2DType::DropConsumed:
			++projection.dropConsumedEventCount;
			break;
		case InventoryEvent2DType::PickupNotReady:
			++projection.pickupNotReadyEventCount;
			break;
		case InventoryEvent2DType::InventoryAddFailed:
			++projection.inventoryAddFailedEventCount;
			break;
		}
	}
	return projection;
}

} // namespace iggy::runtime
