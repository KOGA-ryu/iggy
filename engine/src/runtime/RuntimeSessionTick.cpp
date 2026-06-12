#include "runtime/RuntimeSessionTick.hpp"

namespace iggy::runtime {

RuntimeSessionTickResult RuntimeSessionTick::run(const RuntimeSessionTickInput &input) const
{
	const RuntimeTickResult tick = RuntimeTick {}.run({ input.session.level, input.playerPosition, input.npcConfig });

	RuntimeSessionTickResult result;
	result.session = input.session;
	result.session.level = tick.state;
	result.session.tickIndex = input.session.tickIndex + 1;
	result.npcReports = tick.npcReports;
	return result;
}

} // namespace iggy::runtime
