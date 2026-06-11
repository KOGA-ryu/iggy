#pragma once

#include <vector>

#include "inventory/InventoryService.hpp"
#include "inventory/InventoryTransferResult.hpp"
#include "simulation/SimulationFrameEvents.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SimulationInventoryFinalizer {
public:
	std::vector<InventoryTransferResult> finalize(SimulationWorld &world, const SimulationFrameEvents &frameEvents) const;

private:
	InventoryService inventory_;
};

} // namespace dev
