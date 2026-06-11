#include "RuntimeArtifactOutputService.hpp"

#include "app/RuntimeOutputResultBuilder.hpp"

#include <utility>

namespace dev {

RuntimeArtifactOutputService::RuntimeArtifactOutputService(
    RuntimeTraceService traceService,
    RuntimeDebugArtifactBundle debugBundle)
    : outputRunner_(std::move(traceService), std::move(debugBundle))
{
}

RuntimeOutputResult RuntimeArtifactOutputService::apply(
    const RuntimeOutputSettings &settings,
    const GameLoopResult &result) const
{
	RuntimeOutputResultBuilder output { result.output };

	for (const RuntimeArtifactOutputRequest &request : outputPlan_.build(settings)) {
		outputRunner_.run(request, result, output);
	}

	return output.result();
}

} // namespace dev
