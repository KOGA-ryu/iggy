#pragma once

#include "app/RuntimeDebugArtifactBundle.hpp"
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
	RuntimeTraceService traceService_;
	RuntimeDebugArtifactBundle debugBundle_;
};

} // namespace dev
