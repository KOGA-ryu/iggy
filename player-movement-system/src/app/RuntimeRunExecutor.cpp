#include "RuntimeRunExecutor.hpp"

#include "app/RuntimeSetupRunResultApplier.hpp"

#include <utility>

namespace dev {

RuntimeRunExecutor::RuntimeRunExecutor(
    GameSession &session,
    RuntimeRunRecorder &recorder,
    RuntimeSetupRunner &setupRunner,
    RuntimeFrameLoopRunner &frameLoopRunner,
    RuntimeRunFinalizer finalizer)
    : session_(session)
    , recorder_(recorder)
    , setupRunner_(setupRunner)
    , frameLoopRunner_(frameLoopRunner)
    , finalizer_(std::move(finalizer))
{
}

void RuntimeRunExecutor::run(
    GameLoopResult &result,
    const RuntimeSetupSettings &setup,
    const RuntimeOutputSettings &output) const
{
	RuntimeSetupRunResult setupResult = setupRunner_.run(setup);
	const bool framesAllowed = RuntimeSetupRunResultApplier {}.apply(
	    setupResult,
	    result,
	    recorder_);

	if (framesAllowed)
		frameLoopRunner_.run();

	finalizer_.finalize(session_, output, result);
}

} // namespace dev
