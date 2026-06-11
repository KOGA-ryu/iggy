#include "SimulationEffectPipeline.hpp"

namespace dev {

SimulationEffectPipeline::SimulationEffectPipeline(SimulationClock *clock)
    : clock_(clock)
{
}

void SimulationEffectPipeline::run(SimulationFrameEvents &frameEvents) const
{
	routeEffects(frameEvents);
	applyEffects(frameEvents);
}

void SimulationEffectPipeline::routeEffects(SimulationFrameEvents &frameEvents) const
{
	EffectRouter router { frameEvents };
	for (const MovementEvent &event : frameEvents.movementEvents()) {
		router.route(event);
	}
	for (const CombatEvent &event : frameEvents.combatEvents()) {
		router.route(event);
	}
}

void SimulationEffectPipeline::applyEffects(const SimulationFrameEvents &frameEvents) const
{
	EffectApplier applier { clock_ };
	for (const EffectRequest &request : frameEvents.effectRequests()) {
		applier.apply(request);
	}
}

} // namespace dev
