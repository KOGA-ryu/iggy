#pragma once

#include "app/RuntimeFrameRunner.hpp"
#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeFrameLoopRunner {
public:
	RuntimeFrameLoopRunner(RuntimeFrameRunner &frameRunner, const RuntimeFrameSettings &settings);

	int run();

private:
	RuntimeFrameRunner &frameRunner_;
	const RuntimeFrameSettings &settings_;
};

} // namespace dev
