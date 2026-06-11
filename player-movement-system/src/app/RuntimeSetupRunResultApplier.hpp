#pragma once

#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeRunRecorder.hpp"
#include "app/RuntimeSetupRunner.hpp"

namespace dev {

class RuntimeSetupRunResultApplier {
public:
	[[nodiscard]] bool apply(
	    const RuntimeSetupRunResult &setupResult,
	    GameLoopResult &result,
	    RuntimeRunRecorder &recorder) const;
};

} // namespace dev
