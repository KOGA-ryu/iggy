#include "runtime/RuntimeSessionTickRunner.hpp"

namespace iggy::runtime {

RuntimeSessionTickRunResult RuntimeSessionTickRunner::run(const RuntimeSessionTickRunInput &input) const
{
	RuntimeSessionTickRunResult result;
	result.finalSession = input.initialSession;
	result.reportsByTick.reserve(input.tickCount);

	for (std::size_t tickIndex = 0; tickIndex < input.tickCount; ++tickIndex) {
		const RuntimeSessionTickResult tick = RuntimeSessionTick {}.run({ result.finalSession, input.playerPosition, input.npcConfig });
		result.finalSession = tick.session;
		result.reportsByTick.push_back(tick.npcReports);
	}

	return result;
}

} // namespace iggy::runtime
