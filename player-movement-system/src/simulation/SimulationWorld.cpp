#include "SimulationWorld.hpp"

namespace dev {

void SimulationWorld::setCombatEventSink(CombatEventSink *eventSink)
{
	combatEvents = eventSink;
	combat = CombatSystem { eventSink };
}

} // namespace dev

