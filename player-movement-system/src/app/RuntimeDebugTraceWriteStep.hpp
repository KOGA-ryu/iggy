#pragma once

#include "app/RuntimeDebugArtifactLayout.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeTraceService.hpp"

namespace dev {

class RuntimeDebugTraceWriteStep {
public:
	explicit RuntimeDebugTraceWriteStep(RuntimeTraceService traceService = RuntimeTraceService {});

	[[nodiscard]] bool write(
	    const RuntimeDebugArtifactPaths &paths,
	    const GameLoopResult &result) const;

private:
	RuntimeTraceService traceService_;
};

} // namespace dev
