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

} // namespace dev
