#pragma once

#include "scene/inventory/InventoryState2D.hpp"
#include "scene/inventory/LevelItemDrop2D.hpp"

namespace iggy::runtime {

struct RuntimeInventoryState {
	InventoryState2D inventory;
	LevelItemDrop2DRegistry drops;
};

} // namespace iggy::runtime
