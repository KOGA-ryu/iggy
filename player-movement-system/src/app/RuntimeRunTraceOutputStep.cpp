#include "RuntimeRunTraceOutputStep.hpp"

#include <utility>

namespace dev {

RuntimeRunTraceOutputStep::RuntimeRunTraceOutputStep(RuntimeTraceService traceService)
    : traceService_(std::move(traceService))
{
}

void RuntimeRunTraceOutputStep::save(
    const std::filesystem::path &path,
    const GameLoopResult &result,
    RuntimeOutputResultBuilder &output) const
{
	output.beginRunTraceSave();
	output.completeRunTraceSave(traceService_.saveRunTrace(path, output.applyTo(result)));
}

} // namespace dev
