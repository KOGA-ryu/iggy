#include "SimulationFrameEvents.hpp"

namespace dev {

SimulationFrameEvents::SimulationFrameEvents(MovementEventSink *movementForward, CombatEventSink *combatForward)
    : movementForward_(movementForward)
    , combatForward_(combatForward)
{
}

void SimulationFrameEvents::emit(const MovementEvent &event)
{
	movementEvents_.push_back(event);
	if (movementForward_ != nullptr)
		movementForward_->emit(event);
}

void SimulationFrameEvents::emit(const CombatEvent &event)
{
	combatEvents_.push_back(event);
	if (combatForward_ != nullptr)
		combatForward_->emit(event);
}

void SimulationFrameEvents::emit(const EffectRequest &request)
{
	effectRequests_.push_back(request);
}

void SimulationFrameEvents::clear()
{
	movementEvents_.clear();
	combatEvents_.clear();
	effectRequests_.clear();
}

const std::vector<MovementEvent> &SimulationFrameEvents::movementEvents() const
{
	return movementEvents_;
}

const std::vector<CombatEvent> &SimulationFrameEvents::combatEvents() const
{
	return combatEvents_;
}

const std::vector<EffectRequest> &SimulationFrameEvents::effectRequests() const
{
	return effectRequests_;
}

} // namespace dev
