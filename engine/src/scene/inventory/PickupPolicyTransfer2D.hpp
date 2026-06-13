#pragma once

#include "scene/inventory/InventoryEvent2D.hpp"
#include "scene/inventory/InventoryPolicyAddItem2D.hpp"
#include "scene/inventory/ItemDefinition2D.hpp"
#include "scene/inventory/LevelItemDropConsume2D.hpp"
#include "scene/inventory/PickupPlan2D.hpp"

namespace iggy {

enum class PickupPolicyTransfer2DStatus {
	Transferred,
	PickupNotReady,
	InventoryAddFailed,
	DropConsumeFailed,
};

struct PickupPolicyTransfer2DConfig {
	LevelItemDropConsume2DMode consumeMode = LevelItemDropConsume2DMode::Disable;
};

struct PickupPolicyTransfer2DResult {
	PickupPolicyTransfer2DStatus status = PickupPolicyTransfer2DStatus::PickupNotReady;
	PickupPlan2DResult plan;
	InventoryPolicyAddItem2DResult add;
	LevelItemDropConsume2DResult consume;
	InventoryState2D inventory;
	LevelItemDrop2DRegistry drops;
	InventoryEventRecorder2D events;
	bool changed = false;
};

class PickupPolicyTransfer2D {
public:
	[[nodiscard]] PickupPolicyTransfer2DResult transfer(
		const PickupPlan2DResult &plan,
		const InventoryState2D &inventory,
		const LevelItemDrop2DRegistry &drops,
		const ItemDefinition2DCatalog &catalog,
		const PickupPolicyTransfer2DConfig &config = {}) const;
};

} // namespace iggy
