#include "SimulationFramePolicyDescriber.hpp"

namespace dev {

SimulationFramePolicyDescription SimulationFramePolicyDescriber::describe(SimulationMode mode) const
{
	switch (mode) {
	case SimulationMode::Gameplay:
		return {
		    .mode = mode,
		    .modeName = "Gameplay",
		    .summary = "accept live input and advance all actors",
		    .policy = SimulationFramePolicy::forMode(mode),
		};
	case SimulationMode::Paused:
		return {
		    .mode = mode,
		    .modeName = "Paused",
		    .summary = "preserve queued commands and freeze actors",
		    .policy = SimulationFramePolicy::forMode(mode),
		};
	case SimulationMode::Inventory:
		return {
		    .mode = mode,
		    .modeName = "Inventory",
		    .summary = "hold world simulation while inventory owns input",
		    .policy = SimulationFramePolicy::forMode(mode),
		};
	case SimulationMode::Replay:
		return {
		    .mode = mode,
		    .modeName = "Replay",
		    .summary = "consume recorded commands and advance all actors",
		    .policy = SimulationFramePolicy::forMode(mode),
		};
	case SimulationMode::NetworkPrediction:
		return {
		    .mode = mode,
		    .modeName = "NetworkPrediction",
		    .summary = "accept local commands and predict players without enemies",
		    .policy = SimulationFramePolicy::forMode(mode),
		};
	}

	return {};
}

} // namespace dev
