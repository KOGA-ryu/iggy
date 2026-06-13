#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimePickupEffectFrameStep.hpp"
#include "runtime/RuntimePlayerInputInteractionEffectApplyFrameReport.hpp"
#include "runtime/RuntimePlayerInputInteractionPickupFrameStep.hpp"

namespace iggy::runtime {

enum class RuntimePlayerInputInteractionPickupFrameEvent {
	PlayerCommandAccepted,
	PlayerIntentBlocked,
	PlayerIntentRejected,
	CommandFrameQueued,
	CommandRunnerRan,
	InteractionEffectApplied,
	InteractionEffectDeferred,
	InteractionEventRecorded,
	ItemPickedUp,
	PickupNotReady,
	PickupFailed,
};

struct RuntimePlayerInputInteractionPickupFrameReport {
	RuntimePlayerInputInteractionEffectApplyFrameReport interaction;
	RuntimePickupEffectFrameResult pickup;
	InventoryEventRecorder2D inventoryEvents;
	std::vector<RuntimePlayerInputInteractionPickupFrameEvent> events;
	std::size_t acceptedCommandCount = 0;
	std::size_t blockedIntentCount = 0;
	std::size_t rejectedIntentCount = 0;
	std::size_t interactionAppliedCount = 0;
	std::size_t interactionDeferredCount = 0;
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
	bool interactionMutated = false;
	bool inventoryChanged = false;
};

} // namespace iggy::runtime
