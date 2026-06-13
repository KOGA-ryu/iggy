#pragma once

#include "core/resource/ResourceId.hpp"
#include "runtime/RuntimeInventoryState.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/inventory/InventoryEvent2D.hpp"
#include "scene/inventory/ItemDefinition2D.hpp"
#include "scene/inventory/PickupPlan2D.hpp"
#include "scene/inventory/PickupPolicyTransfer2D.hpp"

namespace iggy::runtime {

enum class RuntimePolicyPickupStatus {
	PickedUp,
	MissingPlayer,
	PickupNotReady,
	TransferFailed,
};

struct RuntimePolicyPickupConfig {
	PickupPlan2DConfig plan;
	PickupPolicyTransfer2DConfig transfer;
};

struct RuntimePolicyPickupResult {
	RuntimePolicyPickupStatus status = RuntimePolicyPickupStatus::PickupNotReady;
	ResourceId dropId;
	PickupPlan2DResult plan;
	PickupPolicyTransfer2DResult transfer;
	RuntimeInventoryState inventory;
	InventoryEventRecorder2D events;
	bool changed = false;
};

class RuntimePolicyPickupStep {
public:
	[[nodiscard]] RuntimePolicyPickupResult pickup(
		const RuntimeSessionState &session,
		const RuntimeInventoryState &inventory,
		const ItemDefinition2DCatalog &catalog,
		ResourceId dropId,
		const RuntimePolicyPickupConfig &config = {}) const;
};

} // namespace iggy::runtime
