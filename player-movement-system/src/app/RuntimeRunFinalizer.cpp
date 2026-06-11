#include "RuntimeRunFinalizer.hpp"

#include <utility>

namespace dev {

RuntimeRunFinalizer::RuntimeRunFinalizer(
    RuntimeFinalModeRecorder finalModeRecorder,
    RuntimeOutputFinalizer outputFinalizer)
    : finalModeRecorder_(std::move(finalModeRecorder))
    , outputFinalizer_(std::move(outputFinalizer))
{
}

void RuntimeRunFinalizer::finalize(
    const GameSession &session,
    const RuntimeOutputSettings &settings,
    GameLoopResult &result) const
{
	finalModeRecorder_.record(session, result);
	outputFinalizer_.finalize(settings, result);
}

} // namespace dev
