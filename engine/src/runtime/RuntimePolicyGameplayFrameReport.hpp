#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimePlayerInputInteractionEffectApplyFrameReport.hpp"
#include "runtime/RuntimePolicyPickupEffectFrameStep.hpp"
#include "scene/inventory/InventoryEvent2D.hpp"
#include "scene/npc/NpcActorMovementFrameReport2D.hpp"

namespace iggy::runtime {

enum class RuntimePolicyGameplayFrameEvent {
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
	NpcMovementChanged,
	NpcMovementBlocked,
	NpcMovementRejected,
	NpcMovementActorMissing,
	NpcMovementRefreshNeeded,
};

struct RuntimePolicyGameplayFrameReport {
	RuntimePlayerInputInteractionEffectApplyFrameReport interaction;
	RuntimePolicyPickupEffectFrameResult pickup;
	InventoryEventRecorder2D inventoryEvents;
	NpcActorMovementFrameReport2D npcMovement;
	std::vector<RuntimePolicyGameplayFrameEvent> events;
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
	std::size_t npcMovedCount = 0;
	std::size_t npcBlockedMovementCount = 0;
	std::size_t npcRejectedMovementCount = 0;
	std::size_t npcMissingActorMovementCount = 0;
	std::size_t npcMovementDirtyTileCount = 0;
	bool interactionChanged = false;
	bool inventoryChanged = false;
	bool npcMovementChanged = false;
	bool npcMovementNeedsOccupancyRebuild = false;
	bool npcMovementNeedsAiMapQueryRefresh = false;
	bool npcMovementNeedsInteractionRefresh = false;
	bool npcMovementNeedsRenderRefresh = false;
	bool npcMovementNeedsVisibilityRefresh = false;

	[[nodiscard]] bool hasEvents() const;
};

} // namespace iggy::runtime
