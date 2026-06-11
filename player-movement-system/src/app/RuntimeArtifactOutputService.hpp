#pragma once

#include "app/RuntimeDebugBundleOutputStep.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeRunTraceOutputStep.hpp"
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
	RuntimeRunTraceOutputStep runTraceOutput_;
	RuntimeDebugBundleOutputStep debugBundleOutput_;
};

} // namespace dev
