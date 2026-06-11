#include "RuntimeRunFinalizer.hpp"

#include <utility>

namespace dev {

RuntimeRunFinalizer::RuntimeRunFinalizer(RuntimeOutputFinalizer outputFinalizer)
    : outputFinalizer_(std::move(outputFinalizer))
{
}

void RuntimeRunFinalizer::finalize(
    const GameSession &session,
    const RuntimeOutputSettings &settings,
    GameLoopResult &result) const
{
	result.finalMode = session.mode();
	outputFinalizer_.finalize(settings, result);
}

} // namespace dev
