#pragma once

#include "app/RuntimeArtifactOutputPlan.hpp"
#include "app/RuntimeDebugBundleOutputStep.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeOutputResultBuilder.hpp"
#include "app/RuntimeRunTraceOutputStep.hpp"
#include "app/RuntimeTraceService.hpp"

namespace dev {

class RuntimeArtifactOutputRequestRunner {
public:
	explicit RuntimeArtifactOutputRequestRunner(
	    RuntimeTraceService traceService = RuntimeTraceService {},
	    RuntimeDebugArtifactBundle debugBundle = RuntimeDebugArtifactBundle {});

	void run(
	    const RuntimeArtifactOutputRequest &request,
	    const GameLoopResult &result,
	    RuntimeOutputResultBuilder &output) const;

private:
	RuntimeRunTraceOutputStep runTraceOutput_;
	RuntimeDebugBundleOutputStep debugBundleOutput_;
};

} // namespace dev
