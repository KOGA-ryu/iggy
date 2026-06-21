#include "runtime3d/Runtime3DClock.hpp"

namespace iggy::runtime3d {

Runtime3DClockTickDecision DecideRuntime3DClockTick(Runtime3DClockState clock)
{
	switch (clock.mode) {
	case Runtime3DClockMode::Normal:
	case Runtime3DClockMode::Slow:
		return { true, true, false };
	case Runtime3DClockMode::Paused:
		return { false, false, false };
	case Runtime3DClockMode::StepRequested:
		return { true, false, true };
	}

	return {};
}

} // namespace iggy::runtime3d
