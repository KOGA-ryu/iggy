#pragma once

#include "core/resource/ResourceId.hpp"
#include "runtime/RuntimeInventoryState.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/inventory/InventoryEvent2D.hpp"
#include "scene/inventory/PickupPlan2D.hpp"
#include "scene/inventory/PickupTransfer2D.hpp"

namespace iggy::runtime {

enum class RuntimePickupStatus {
	PickedUp,
	PickupNotReady,
	MissingPlayer,
	TransferFailed,
};

struct RuntimePickupConfig {
	PickupPlan2DConfig plan;
	PickupTransfer2DConfig transfer;
};

struct RuntimePickupResult {
	RuntimePickupStatus status = RuntimePickupStatus::PickupNotReady;
	RuntimeInventoryState inventory;
	ResourceId dropId;
	PickupPlan2DResult plan;
	PickupTransfer2DResult transfer;
	bool changed = false;
	InventoryEventRecorder2D events;
};

class RuntimePickupStep {
public:
	[[nodiscard]] RuntimePickupResult pickup(
		const RuntimeSessionState &session,
		const RuntimeInventoryState &inventory,
		ResourceId dropId,
		const RuntimePickupConfig &config = {}) const;
};

} // namespace iggy::runtime
