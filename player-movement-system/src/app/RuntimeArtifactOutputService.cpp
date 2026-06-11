#include "RuntimeArtifactOutputService.hpp"

#include "app/RuntimeOutputResultBuilder.hpp"

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
	RuntimeOutputResultBuilder output { result.output };

	if (settings.runTracePath.has_value()) {
		output.beginRunTraceSave();
		output.completeRunTraceSave(traceService_.saveRunTrace(*settings.runTracePath, output.applyTo(result)));
	}

	if (settings.debugBundlePath.has_value()) {
		output.beginDebugBundleSave();
		output.completeDebugBundleSave(debugBundle_.save(*settings.debugBundlePath, output.applyTo(result)).saved());
	}

	return output.result();
}

} // namespace dev
