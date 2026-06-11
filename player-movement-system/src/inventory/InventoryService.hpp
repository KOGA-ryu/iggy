#pragma once

#include <vector>

#include "events/MovementEvent.hpp"
#include "inventory/InventoryTransferResult.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class InventoryService {
public:
	std::vector<InventoryTransferResult> applyPickupEvents(SimulationWorld &world, const std::vector<MovementEvent> &events) const;

private:
	[[nodiscard]] bool isPickupEvent(const MovementEvent &event) const;
	[[nodiscard]] const Item *findWorldItem(const SimulationWorld &world, TargetId id) const;
};

} // namespace dev
