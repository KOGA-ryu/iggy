#include "RuntimeOutputFinalizer.hpp"

namespace dev {

RuntimeOutputFinalizer::RuntimeOutputFinalizer(
    RuntimeTraceService traceService,
    RuntimeDebugArtifactBundle debugBundle)
    : traceService_(traceService)
    , debugBundle_(debugBundle)
{
}

void RuntimeOutputFinalizer::finalize(const RuntimeOutputSettings &settings, GameLoopResult &result) const
{
	if (settings.runTracePath.has_value()) {
		result.output.runTraceSaveAttempted = true;
		result.output.runTraceSaved = traceService_.saveRunTrace(*settings.runTracePath, result);
	}

	if (settings.debugBundlePath.has_value()) {
		result.output.debugBundleSaveAttempted = true;
		result.output.debugBundleSaved = debugBundle_.save(*settings.debugBundlePath, result).saved();
	}
}

bool RuntimeOutputFinalizer::failed(const RuntimeOutputResult &result)
{
	return (result.runTraceSaveAttempted && !result.runTraceSaved)
	    || (result.debugBundleSaveAttempted && !result.debugBundleSaved);
}

} // namespace dev
