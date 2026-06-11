#include "RuntimeArtifactOutputService.hpp"

#include "app/RuntimeOutputResultBuilder.hpp"

#include <utility>

namespace dev {

RuntimeArtifactOutputService::RuntimeArtifactOutputService(
    RuntimeTraceService traceService,
    RuntimeDebugArtifactBundle debugBundle)
    : runTraceOutput_(std::move(traceService))
    , debugBundleOutput_(std::move(debugBundle))
{
}

RuntimeOutputResult RuntimeArtifactOutputService::apply(
    const RuntimeOutputSettings &settings,
    const GameLoopResult &result) const
{
	RuntimeOutputResultBuilder output { result.output };

	if (settings.runTracePath.has_value()) {
		runTraceOutput_.save(*settings.runTracePath, result, output);
	}

	if (settings.debugBundlePath.has_value()) {
		debugBundleOutput_.save(*settings.debugBundlePath, result, output);
	}

	return output.result();
}

} // namespace dev
