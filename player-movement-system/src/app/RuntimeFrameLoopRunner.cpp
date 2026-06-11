#include "RuntimeFrameLoopRunner.hpp"

namespace dev {

RuntimeFrameLoopRunner::RuntimeFrameLoopRunner(RuntimeFrameRunner &frameRunner, const RuntimeFrameSettings &settings)
    : frameRunner_(frameRunner)
    , settings_(settings)
{
}

int RuntimeFrameLoopRunner::run()
{
	int framesRun = 0;
	for (int frame = 0; frame < settings_.maxFrames; ++frame) {
		frameRunner_.runFrame();
		++framesRun;
	}
	return framesRun;
}

} // namespace dev
