#include "SessionWorldSlotSaver.hpp"

#include "session/SessionModePolicy.hpp"

namespace dev {

bool SessionWorldSlotSaver::save(
    const SaveSlotService &saveSlots,
    SaveSlotId slotId,
    const SimulationWorld &world,
    GameSessionMode mode) const
{
	if (!SessionModePolicy {}.hasActiveWorld(mode))
		return false;
	return saveSlots.saveSlot(slotId, world);
}

} // namespace dev
