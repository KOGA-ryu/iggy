#include "SimulationFrameEventCapture.hpp"

namespace dev {

SimulationFrameEventCapture::SimulationFrameEventCapture(SimulationWorld &world)
    : world_(world)
    , previousMovementEvents_(world.movementEvents)
    , previousCombatEvents_(world.combatEvents)
    , events_(previousMovementEvents_, previousCombatEvents_)
{
	world_.movementEvents = &events_;
	world_.setCombatEventSink(&events_);
}

SimulationFrameEventCapture::~SimulationFrameEventCapture()
{
	restore();
}

SimulationFrameEvents &SimulationFrameEventCapture::events()
{
	return events_;
}

const SimulationFrameEvents &SimulationFrameEventCapture::events() const
{
	return events_;
}

void SimulationFrameEventCapture::restore()
{
	if (restored_)
		return;

	world_.movementEvents = previousMovementEvents_;
	world_.setCombatEventSink(previousCombatEvents_);
	restored_ = true;
}

} // namespace dev
