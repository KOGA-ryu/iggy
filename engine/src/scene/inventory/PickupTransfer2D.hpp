#pragma once

#include "scene/inventory/InventoryAddItem2D.hpp"
#include "scene/inventory/LevelItemDropConsume2D.hpp"
#include "scene/inventory/PickupPlan2D.hpp"

namespace iggy {

enum class PickupTransfer2DStatus {
	Transferred,
	PickupNotReady,
	InventoryAddFailed,
	DropConsumeFailed,
};

struct PickupTransfer2DConfig {
	LevelItemDropConsume2DMode consumeMode = LevelItemDropConsume2DMode::Disable;
};

struct PickupTransfer2DResult {
	PickupTransfer2DStatus status = PickupTransfer2DStatus::PickupNotReady;
	InventoryState2D inventory;
	LevelItemDrop2DRegistry drops;
	PickupPlan2DResult plan;
	InventoryAddItem2DResult add;
	LevelItemDropConsume2DResult consume;
	bool changed = false;
	InventoryEventRecorder2D events;
};

class PickupTransfer2D {
public:
	[[nodiscard]] PickupTransfer2DResult transfer(
		const InventoryState2D &inventory,
		const LevelItemDrop2DRegistry &drops,
		const PickupPlan2DResult &plan,
		const PickupTransfer2DConfig &config = {}) const;
};

} // namespace iggy
