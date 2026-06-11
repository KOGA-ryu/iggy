#include "SimulationWorld.hpp"

namespace dev {

void SimulationWorld::setCombatEventSink(CombatEventSink *eventSink)
{
	combatEvents = eventSink;
	combat.setEventSink(eventSink);
}

} // namespace dev
