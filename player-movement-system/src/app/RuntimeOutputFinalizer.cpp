#include "RuntimeOutputFinalizer.hpp"

namespace dev {

RuntimeOutputFinalizer::RuntimeOutputFinalizer(RuntimeArtifactOutputService outputService)
    : outputService_(outputService)
{
}

void RuntimeOutputFinalizer::finalize(const RuntimeOutputSettings &settings, GameLoopResult &result) const
{
	result.output = outputService_.apply(settings, result);
}

bool RuntimeOutputFinalizer::failed(const RuntimeOutputResult &result)
{
	return (result.runTraceSaveAttempted && !result.runTraceSaved)
	    || (result.debugBundleSaveAttempted && !result.debugBundleSaved);
}

} // namespace dev
