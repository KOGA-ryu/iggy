#pragma once

#include <cstdint>
#include <vector>

#include "core/resource/ResourceId.hpp"

namespace iggy {

enum class InventoryEvent2DType {
	ItemAdded,
	ItemPickedUp,
	DropConsumed,
	PickupNotReady,
	InventoryAddFailed,
};

struct InventoryEvent2D {
	InventoryEvent2DType type = InventoryEvent2DType::ItemAdded;
	ResourceId itemId;
	ResourceId dropId;
	std::uint32_t count = 0;
};

[[nodiscard]] InventoryEvent2D itemAddedInventoryEvent(ResourceId itemId, std::uint32_t count);
[[nodiscard]] InventoryEvent2D itemPickedUpInventoryEvent(ResourceId dropId, ResourceId itemId, std::uint32_t count);
[[nodiscard]] InventoryEvent2D dropConsumedInventoryEvent(ResourceId dropId);
[[nodiscard]] InventoryEvent2D pickupNotReadyInventoryEvent(ResourceId dropId);
[[nodiscard]] InventoryEvent2D inventoryAddFailedEvent(ResourceId itemId, std::uint32_t count);

struct InventoryEventRecorder2D {
	std::vector<InventoryEvent2D> events;
};

void recordInventoryEvent(InventoryEventRecorder2D &recorder, const InventoryEvent2D &event);

} // namespace iggy
