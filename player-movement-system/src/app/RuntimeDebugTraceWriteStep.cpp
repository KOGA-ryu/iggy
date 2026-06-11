#include "RuntimeDebugTraceWriteStep.hpp"

namespace dev {

RuntimeDebugTraceWriteStep::RuntimeDebugTraceWriteStep(RuntimeTraceService traceService)
    : traceService_(traceService)
{
}

bool RuntimeDebugTraceWriteStep::write(
    const RuntimeDebugArtifactPaths &paths,
    const GameLoopResult &result) const
{
	return traceService_.saveRunTrace(paths.tracePath, result);
}

} // namespace dev
