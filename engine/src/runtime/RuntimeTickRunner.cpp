#include "runtime/RuntimeTickRunner.hpp"

namespace iggy::runtime {

RuntimeTickRunResult RuntimeTickRunner::run(const RuntimeTickRunInput &input) const
{
	RuntimeTickRunResult result;
	result.finalState = input.initialState;
	result.reportsByTick.reserve(input.tickCount);

	for (std::size_t tickIndex = 0; tickIndex < input.tickCount; ++tickIndex) {
		const RuntimeTickResult tick = RuntimeTick {}.run({ result.finalState, input.playerPosition, input.npcConfig });
		result.finalState = tick.state;
		result.reportsByTick.push_back(tick.npcReports);
	}

	return result;
}

} // namespace iggy::runtime
