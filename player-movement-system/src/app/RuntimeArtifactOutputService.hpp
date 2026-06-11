#pragma once

#include "app/RuntimeArtifactOutputPlan.hpp"
#include "app/RuntimeArtifactOutputRequestRunner.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeTraceService.hpp"

namespace dev {

class RuntimeArtifactOutputService {
public:
	explicit RuntimeArtifactOutputService(
	    RuntimeTraceService traceService = RuntimeTraceService {},
	    RuntimeDebugArtifactBundle debugBundle = RuntimeDebugArtifactBundle {});

	[[nodiscard]] RuntimeOutputResult apply(
	    const RuntimeOutputSettings &settings,
	    const GameLoopResult &result) const;

private:
	RuntimeArtifactOutputPlan outputPlan_;
	RuntimeArtifactOutputRequestRunner outputRunner_;
};

} // namespace dev
