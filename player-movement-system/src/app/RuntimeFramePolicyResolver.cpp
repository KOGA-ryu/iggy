#include "RuntimeFramePolicyResolver.hpp"

#include "session/SessionModePolicy.hpp"

namespace dev {

SimulationFramePolicyDescription RuntimeFramePolicyResolver::resolve(const GameSession &session) const
{
	const SimulationMode simulationMode = SessionModePolicy {}.simulationModeFor(session.mode());
	return SimulationFramePolicyDescriber {}.describe(simulationMode);
}

} // namespace dev
