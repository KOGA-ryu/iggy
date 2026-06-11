#include "RuntimeArtifactOutputService.hpp"

namespace dev {

RuntimeArtifactOutputService::RuntimeArtifactOutputService(
    RuntimeTraceService traceService,
    RuntimeDebugArtifactBundle debugBundle)
    : traceService_(traceService)
    , debugBundle_(debugBundle)
{
}

RuntimeOutputResult RuntimeArtifactOutputService::apply(
    const RuntimeOutputSettings &settings,
    const GameLoopResult &result) const
{
	RuntimeOutputResult output = result.output;

	if (settings.runTracePath.has_value()) {
		output.runTraceSaveAttempted = true;
		GameLoopResult outputResult = result;
		outputResult.output = output;
		output.runTraceSaved = traceService_.saveRunTrace(*settings.runTracePath, outputResult);
	}

	if (settings.debugBundlePath.has_value()) {
		output.debugBundleSaveAttempted = true;
		GameLoopResult outputResult = result;
		outputResult.output = output;
		output.debugBundleSaved = debugBundle_.save(*settings.debugBundlePath, outputResult).saved();
	}

	return output;
}

} // namespace dev
