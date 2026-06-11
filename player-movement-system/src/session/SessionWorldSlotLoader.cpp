#include "SessionWorldSlotLoader.hpp"

#include <utility>

namespace dev {

bool SessionWorldSlotLoader::load(const SaveSlotService &saveSlots, SaveSlotId slotId, SimulationWorld &world) const
{
	MovementEventSink *movementEvents = world.movementEvents;
	CombatEventSink *combatEvents = world.combatEvents;
	SimulationWorld loadedWorld;
	loadedWorld.movementEvents = movementEvents;
	loadedWorld.setCombatEventSink(combatEvents);
	if (!saveSlots.loadSlot(slotId, loadedWorld))
		return false;

	world = std::move(loadedWorld);
	world.movementEvents = movementEvents;
	world.setCombatEventSink(combatEvents);
	return true;
}

} // namespace dev
