#pragma once

#include "commands/CommandDispatcher.hpp"
#include "commands/CommandQueue.hpp"

namespace dev {

class SimulationCommandQueueDrainStep {
public:
	[[nodiscard]] int drain(CommandQueue &commands, CommandDispatcher &dispatcher) const;
};

} // namespace dev
