#include "SimulationInventoryFinalizer.hpp"

namespace dev {

std::vector<InventoryTransferResult> SimulationInventoryFinalizer::finalize(SimulationWorld &world, const SimulationFrameEvents &frameEvents) const
{
	return inventory_.applyPickupEvents(world, frameEvents.movementEvents());
}

} // namespace dev
