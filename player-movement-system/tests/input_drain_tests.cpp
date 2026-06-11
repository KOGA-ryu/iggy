#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

#include "app/RuntimeInputDrainReportRecorder.hpp"
#include "app/RuntimeInputDrainResultBuilder.hpp"
#include "app/RuntimeInputRouter.hpp"
#include "app/RuntimeInputRouteResultBuilder.hpp"
#include "app/RuntimeInputSourceRouter.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeRawInputDrainer.hpp"
#include "commands/MovementCommandSource.hpp"
#include "input/RawInputSource.hpp"
#include "player/Player.hpp"
#include "session/GameSession.hpp"
#include "session/SessionCommandSource.hpp"
#include "simulation/SimulationWorld.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

dev::Player MakePlayer(dev::Point tile = { 0, 0 })
{
	dev::Player player;
	player.position.tile = tile;
	player.position.future = tile;
	player.position.previous = tile;
	player.position.precise = tile;
	return player;
}

void TestRuntimeInputDrainResultBuilderAggregatesRouteOutcomes()
{
	dev::RuntimeInputRouteResultBuilder routeResults;
	dev::RuntimeInputDrainResultBuilder drainResults;

	drainResults.record(routeResults.unhandled());
	drainResults.record(routeResults.queuedSessionCommand());
	drainResults.record(routeResults.queuedMovementCommand());
	drainResults.record(routeResults.blockedMovement(dev::PlayerActionBlockReason::Focus));
	drainResults.record(routeResults.blockedMovement(dev::PlayerActionBlockReason::Paused));

	const dev::RuntimeInputDrainResult result = drainResults.build();

	Expect(result.handled == 2, "runtime input drain result builder should count handled route outcomes");
	Expect(result.movementBlockReasons.size() == 2, "runtime input drain result builder should collect blocked movement reasons");
	Expect(result.movementBlockReasons.size() == 2 && result.movementBlockReasons[0] == dev::PlayerActionBlockReason::Focus, "runtime input drain result builder should preserve first block reason");
	Expect(result.movementBlockReasons.size() == 2 && result.movementBlockReasons[1] == dev::PlayerActionBlockReason::Paused, "runtime input drain result builder should preserve second block reason");
}

void TestRuntimeInputDrainReportRecorderCopiesFrameAndAggregatesSummary()
{
	dev::RuntimeFrameReport frame;
	dev::RuntimeRunSummary summary;
	summary.rawInputEventsRouted = 1;
	summary.movementInputBlockReasons.push_back(dev::PlayerActionBlockReason::Stunned);

	dev::RuntimeInputDrainReportRecorder {}.record(
	    dev::RuntimeInputDrainResult {
	        .handled = 2,
	        .movementBlockReasons = {
	            dev::PlayerActionBlockReason::Focus,
	            dev::PlayerActionBlockReason::Paused,
	        },
	    },
	    frame,
	    summary);

	Expect(frame.rawInputEventsRouted == 2, "runtime input drain report recorder should copy handled count onto frame report");
	Expect(frame.movementInputBlockReasons.size() == 2 && frame.movementInputBlockReasons[0] == dev::PlayerActionBlockReason::Focus, "runtime input drain report recorder should copy first frame block reason");
	Expect(frame.movementInputBlockReasons.size() == 2 && frame.movementInputBlockReasons[1] == dev::PlayerActionBlockReason::Paused, "runtime input drain report recorder should copy second frame block reason");
	Expect(summary.rawInputEventsRouted == 3, "runtime input drain report recorder should aggregate handled count onto run summary");
	Expect(summary.movementInputBlockReasons.size() == 3 && summary.movementInputBlockReasons[0] == dev::PlayerActionBlockReason::Stunned, "runtime input drain report recorder should preserve existing summary block reasons");
	Expect(summary.movementInputBlockReasons.size() == 3 && summary.movementInputBlockReasons[1] == dev::PlayerActionBlockReason::Focus, "runtime input drain report recorder should append first new block reason");
	Expect(summary.movementInputBlockReasons.size() == 3 && summary.movementInputBlockReasons[2] == dev::PlayerActionBlockReason::Paused, "runtime input drain report recorder should append second new block reason");
}

void TestQueuedRawInputSourceDrainsEventsOnce()
{
	dev::QueuedRawInputSource source;
	source.enqueue({
	    .type = dev::RawInputType::MouseClick,
	    .screenPosition = { 32, 64 },
	    .pressed = true,
	});
	source.enqueue({
	    .type = dev::RawInputType::KeyPress,
	    .code = 'P',
	    .pressed = true,
	});

	Expect(source.size() == 2, "queued raw input source should track queued event count");
	std::vector<dev::RawInputEvent> drained = source.drain();
	Expect(drained.size() == 2, "queued raw input source should drain queued events");
	Expect(source.empty(), "queued raw input source should be empty after drain");
	Expect(source.drain().empty(), "queued raw input source should not drain events twice");
	Expect(drained.size() == 2 && drained[0].screenPosition == dev::Point { 32, 64 }, "queued raw input source should preserve event payloads");
}

void TestRuntimeRawInputDrainerRoutesHandledEventsAndSkipsNullSources()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));

	dev::QueuedRawInputSource rawInput;
	rawInput.enqueue({
	    .type = dev::RawInputType::KeyPress,
	    .code = 'P',
	    .pressed = true,
	});
	rawInput.enqueue({
	    .type = dev::RawInputType::KeyPress,
	    .code = 'P',
	    .pressed = false,
	});
	rawInput.enqueue({
	    .type = dev::RawInputType::MouseClick,
	    .screenPosition = { 32, 0 },
	    .pressed = true,
	});

	dev::QueuedSessionCommandSource sessionCommands;
	dev::QueuedMovementCommandSource movementCommands;
	dev::RuntimeInputRouter router {
		sessionCommands,
		movementCommands,
		dev::RuntimeInputBindings { .pauseKey = 'P', .inventoryKey = 'I', .stopKey = 'S' },
	};
	dev::RuntimeRawInputDrainer drainer { router };

	const dev::RuntimeInputDrainResult routed = drainer.drain(
	    { nullptr, &rawInput },
	    dev::RuntimeInputContext {
	        .world = &world,
	        .focusState = dev::FocusState { .owner = dev::InputOwner::Inventory },
	        .sessionMode = dev::GameSessionMode::Gameplay,
	    });

	Expect(routed.handled == 1, "runtime raw input drainer should count handled routed events only");
	Expect(routed.movementBlockReasons.size() == 1 && routed.movementBlockReasons[0] == dev::PlayerActionBlockReason::Focus, "runtime raw input drainer should collect blocked movement reasons separately");
	Expect(rawInput.empty(), "runtime raw input drainer should drain source events once");
	Expect(sessionCommands.size() == 1, "runtime raw input drainer should route handled hotkeys through session commands");
	Expect(movementCommands.empty(), "runtime raw input drainer should not invent movement commands for hotkeys");
}

void TestRuntimeInputSourceRouterRoutesRawSourcesThroughSessionContext()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_input_source_router_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::QueuedRawInputSource rawInput;
	rawInput.enqueue({
	    .type = dev::RawInputType::MouseClick,
	    .screenPosition = { 96, 64 },
	    .pressed = true,
	});

	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::RuntimeSourceSettings sources {
		.rawInputSources = { nullptr, &rawInput },
	};
	dev::RuntimeInputSettings input;
	dev::RuntimeInputSourceRouter router {
		session,
		routedSessionCommands,
		routedMovementCommands,
		sources,
		input,
	};

	const dev::RuntimeInputDrainResult routed = router.route();

	Expect(routed.handled == 1, "runtime input source router should route handled raw source events");
	Expect(routed.movementBlockReasons.empty(), "runtime input source router should report no movement blocks for handled movement input");
	Expect(rawInput.empty(), "runtime input source router should drain raw input source events once");
	Expect(routedMovementCommands.size() == 1, "runtime input source router should queue routed movement commands");
	std::vector<dev::MovementCommand> commands = routedMovementCommands.drain();
	Expect(commands.size() == 1 && commands[0].destination == dev::Point { 3, 2 }, "runtime input source router should build input context from session world");
	Expect(routedSessionCommands.empty(), "runtime input source router should leave session commands empty for movement input");

	std::filesystem::remove_all(root);
}

} // namespace

int main()
{
	TestRuntimeInputDrainResultBuilderAggregatesRouteOutcomes();
	TestRuntimeInputDrainReportRecorderCopiesFrameAndAggregatesSummary();
	TestQueuedRawInputSourceDrainsEventsOnce();
	TestRuntimeRawInputDrainerRoutesHandledEventsAndSkipsNullSources();
	TestRuntimeInputSourceRouterRoutesRawSourcesThroughSessionContext();

	if (Failures != 0) {
		std::cerr << Failures << " test(s) failed\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
