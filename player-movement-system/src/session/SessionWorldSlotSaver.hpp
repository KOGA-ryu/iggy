#pragma once

#include "save/SaveSlotService.hpp"
#include "session/GameSessionMode.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SessionWorldSlotSaver {
public:
	[[nodiscard]] bool save(
	    const SaveSlotService &saveSlots,
	    SaveSlotId slotId,
	    const SimulationWorld &world,
	    GameSessionMode mode) const;
};

} // namespace dev
