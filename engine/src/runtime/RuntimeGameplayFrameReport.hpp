#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimePlayerInputInteractionPickupFrameReport.hpp"

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
};

struct RuntimeGameplayFrameReport {
	RuntimePlayerInputInteractionPickupFrameReport input;
	std::vector<RuntimeGameplayFrameEvent> events;
	std::size_t acceptedCommandCount = 0;
	std::size_t blockedIntentCount = 0;
	std::size_t rejectedIntentCount = 0;
	std::size_t interactionEventCount = 0;
	std::size_t pickedUpCount = 0;
	std::size_t pickupNotReadyCount = 0;
	std::size_t pickupFailedCount = 0;
	bool interactionChanged = false;
	bool inventoryChanged = false;

	[[nodiscard]] bool hasEvents() const;
};

} // namespace iggy::runtime
