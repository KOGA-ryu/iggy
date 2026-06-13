#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimePlayerInputInteractionPickupFrameReport.hpp"
#include "scene/inventory/InventoryEvent2D.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayFrameEvent {
	PlayerCommandAccepted,
	PlayerIntentBlocked,
	PlayerIntentRejected,
	CommandFrameQueued,
	CommandRunnerRan,
	InteractionChanged,
	InventoryChanged,
	ItemPickedUp,
	PickupNotReady,
	PickupFailed,
	InteractionEventRecorded,
	InventoryEventRecorded,
};

struct RuntimeGameplayFrameReport {
	RuntimePlayerInputInteractionPickupFrameReport input;
	InventoryEventRecorder2D inventoryEvents;
	std::vector<RuntimeGameplayFrameEvent> events;
	std::size_t acceptedCommandCount = 0;
	std::size_t blockedIntentCount = 0;
	std::size_t rejectedIntentCount = 0;
	std::size_t interactionEventCount = 0;
	std::size_t pickedUpCount = 0;
	std::size_t pickupNotReadyCount = 0;
	std::size_t pickupFailedCount = 0;
	std::size_t inventoryEventCount = 0;
	std::size_t itemAddedEventCount = 0;
	std::size_t itemPickedUpEventCount = 0;
	std::size_t dropConsumedEventCount = 0;
	std::size_t pickupNotReadyEventCount = 0;
	std::size_t inventoryAddFailedEventCount = 0;
	bool interactionChanged = false;
	bool inventoryChanged = false;

	[[nodiscard]] bool hasEvents() const;
};

} // namespace iggy::runtime
