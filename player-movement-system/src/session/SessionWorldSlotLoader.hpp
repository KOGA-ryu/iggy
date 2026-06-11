#pragma once

#include "save/SaveSlotService.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SessionWorldSlotLoader {
public:
	[[nodiscard]] bool load(const SaveSlotService &saveSlots, SaveSlotId slotId, SimulationWorld &world) const;
};

} // namespace dev
