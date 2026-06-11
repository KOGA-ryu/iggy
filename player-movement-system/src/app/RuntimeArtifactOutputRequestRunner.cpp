#include "RuntimeArtifactOutputRequestRunner.hpp"

#include <utility>

namespace dev {

RuntimeArtifactOutputRequestRunner::RuntimeArtifactOutputRequestRunner(
    RuntimeTraceService traceService,
    RuntimeDebugArtifactBundle debugBundle)
    : runTraceOutput_(std::move(traceService))
    , debugBundleOutput_(std::move(debugBundle))
{
}

void RuntimeArtifactOutputRequestRunner::run(
    const RuntimeArtifactOutputRequest &request,
    const GameLoopResult &result,
    RuntimeOutputResultBuilder &output) const
{
	switch (request.kind) {
	case RuntimeArtifactOutputKind::RunTrace:
		runTraceOutput_.save(request.path, result, output);
		break;
	case RuntimeArtifactOutputKind::DebugBundle:
		debugBundleOutput_.save(request.path, result, output);
		break;
	}
}

} // namespace dev
