#include "GameLoop.hpp"

#include "RuntimeExitCodePolicy.hpp"
#include "RuntimeFrameRunner.hpp"
#include "RuntimeOutputFinalizer.hpp"
#include "RuntimeRunRecorder.hpp"
#include "RuntimeSetupRunner.hpp"
#include "RuntimeSourceDrainer.hpp"
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
		{
		    .sessionCommandSources = settings_.sources.sessionCommandSources,
		    .inventoryScriptSources = settings_.sources.inventoryScriptSources,
		    .inventoryCommandSources = settings_.sources.inventoryCommandSources,
		    .movementCommandSources = settings_.sources.movementCommandSources,
		    .inputPlayerId = settings_.input.playerId,
		},
	};

	auto finish = [&]() {
		result.finalMode = session_.mode();
		RuntimeOutputFinalizer {}.finalize(settings_.output, result);
		return result;
	};

	RuntimeSetupRunResult setup = RuntimeSetupRunner { dispatcher, drainer }.run(settings_.setup);
	result.setup = setup.setup;
	result.summary.inventoryCommandResults.insert(
	    result.summary.inventoryCommandResults.end(),
	    setup.inventoryCommandResults.begin(),
	    setup.inventoryCommandResults.end());
	if (!setup.framesAllowed)
		return finish();

	RuntimeFrameRunner frameRunner {
		session_,
		routedSessionCommands_,
		routedMovementCommands_,
		drainer,
		recorder,
		dispatcher,
		settings_.sources,
		settings_.input,
		settings_.frame,
	};
	for (int frame = 0; frame < settings_.frame.maxFrames; ++frame) {
		frameRunner.runFrame();
	}

	return finish();
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
