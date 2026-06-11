#include "GameLoop.hpp"

#include "RuntimeExitCodePolicy.hpp"
#include "RuntimeFrameLoopRunner.hpp"
#include "RuntimeFrameRunner.hpp"
#include "RuntimeInputSourceRouter.hpp"
#include "RuntimeRunExecutor.hpp"
#include "RuntimeRunRecorder.hpp"
#include "RuntimeSetupRunner.hpp"
#include "RuntimeSourceDrainer.hpp"
#include "RuntimeSourceDrainerSettingsBuilder.hpp"
#include "session/SessionCommandDispatcher.hpp"

#include <utility>

namespace dev {

GameLoop::GameLoop(GameLoopSettings settings)
    : settings_(std::move(settings))
    , session_(settings_.saveRoot)
{
}

int GameLoop::run()
{
	const GameLoopResult result = runForResult();
	return RuntimeExitCodePolicy {}.exitCodeFor(result);
}

GameLoopResult GameLoop::runForResult()
{
	GameLoopResult result;
	RuntimeRunRecorder recorder { result, sessionEvents_, inventoryEvents_ };
	SessionCommandDispatcher dispatcher { session_, &sessionEvents_ };
	RuntimeSourceDrainer drainer {
		session_,
		inventoryEvents_,
		routedSessionCommands_,
		routedMovementCommands_,
		RuntimeSourceDrainerSettingsBuilder {}.build(settings_.sources, settings_.input),
	};
	RuntimeInputSourceRouter inputSourceRouter {
		session_,
		routedSessionCommands_,
		routedMovementCommands_,
		settings_.sources,
		settings_.input,
	};
	RuntimeFrameRunner frameRunner {
		session_,
		inputSourceRouter,
		drainer,
		recorder,
		dispatcher,
		settings_.frame,
	};
	RuntimeFrameLoopRunner frameLoopRunner { frameRunner, settings_.frame };
	RuntimeSetupRunner setupRunner { dispatcher, drainer };

	RuntimeRunExecutor { session_, recorder, setupRunner, frameLoopRunner }.run(
	    result,
	    settings_.setup,
	    settings_.output);
	return result;
}

GameSession &GameLoop::session()
{
	return session_;
}

const GameSession &GameLoop::session() const
{
	return session_;
}

const SessionEventRecorder &GameLoop::sessionEvents() const
{
	return sessionEvents_;
}

const InventoryEventRecorder &GameLoop::inventoryEvents() const
{
	return inventoryEvents_;
}

} // namespace dev
