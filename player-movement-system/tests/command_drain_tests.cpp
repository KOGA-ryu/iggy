#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeMovementCommandIntake.hpp"
#include "app/RuntimeMovementCommandQueueStep.hpp"
#include "app/RuntimeMovementCommandReportRecorder.hpp"
#include "commands/CommandDispatcher.hpp"
#include "commands/CommandQueue.hpp"
#include "commands/MovementCommandSource.hpp"
#include "events/EventRecorder.hpp"
#include "player/Player.hpp"
#include "player/PlayerController.hpp"
#include "simulation/SimulationCommandDrainer.hpp"
#include "simulation/SimulationCommandQueueDrainStep.hpp"
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

void TestQueuedMovementCommandSourceDrainsCommandsOnce()
{
	dev::QueuedMovementCommandSource source;
	source.enqueue({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 3, 4 },
	});
	source.enqueue({
	    .type = dev::MovementCommandType::Stop,
	    .playerId = 0,
	    .destination = { 0, 0 },
	});

	Expect(source.size() == 2, "queued movement command source should track queued command count");
	std::vector<dev::MovementCommand> drained = source.drain();
	Expect(drained.size() == 2, "queued movement command source should drain queued commands");
	Expect(source.empty(), "queued movement command source should be empty after drain");
	Expect(source.drain().empty(), "queued movement command source should not drain commands twice");
	Expect(drained.size() == 2 && drained[0].destination == dev::Point { 3, 4 }, "queued movement command source should preserve command payloads");
}

void TestRuntimeMovementCommandQueueStepQueuesRouteResults()
{
	dev::QueuedMovementCommandSource source;
	dev::RuntimeMovementCommandQueueStep queueStep { source };

	dev::RuntimeInputRouteResult result = queueStep.queue(dev::MovementCommand {
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 1,
	    .destination = { 6, 7 },
	});
	std::vector<dev::MovementCommand> drained = source.drain();

	std::optional<dev::MovementCommand> missingCommand;
	dev::RuntimeInputRouteResult missingResult = queueStep.queue(missingCommand);

	Expect(result.handled, "runtime movement command queue step should mark concrete commands handled");
	Expect(result.queuedMovementCommand, "runtime movement command queue step should report queued movement commands");
	Expect(!result.queuedSessionCommand, "runtime movement command queue step should not report session commands");
	Expect(!result.movementBlockReason.has_value(), "runtime movement command queue step should not invent block reasons");
	Expect(drained.size() == 1, "runtime movement command queue step should enqueue concrete commands");
	Expect(drained.size() == 1 && drained[0].playerId == 1 && drained[0].destination == dev::Point { 6, 7 }, "runtime movement command queue step should preserve command payloads");
	Expect(!missingResult.handled, "runtime movement command queue step should leave missing optional commands unhandled");
	Expect(source.empty(), "runtime movement command queue step should not queue missing optional commands");
}

void TestRuntimeMovementCommandIntakeQueuesCommandsInWorldOrder()
{
	dev::SimulationWorld world;
	std::vector<dev::MovementCommand> commands {
	    {
	        .type = dev::MovementCommandType::WalkTo,
	        .playerId = 0,
	        .destination = { 1, 0 },
	    },
	    {
	        .type = dev::MovementCommandType::Stop,
	        .playerId = 0,
	        .destination = { 2, 0 },
	    },
	};

	const int queued = dev::RuntimeMovementCommandIntake {}.queue(commands, world);

	dev::MovementCommand first {};
	dev::MovementCommand second {};
	dev::MovementCommand none {};
	Expect(queued == 2, "runtime movement command intake should report queued command count");
	Expect(world.commandQueue.tryPop(first), "runtime movement command intake should push first command");
	Expect(world.commandQueue.tryPop(second), "runtime movement command intake should push second command");
	Expect(!world.commandQueue.tryPop(none), "runtime movement command intake should not add extra commands");
	Expect(first.destination == dev::Point { 1, 0 }, "runtime movement command intake should preserve first command order");
	Expect(second.destination == dev::Point { 2, 0 }, "runtime movement command intake should preserve second command order");
}

void TestSimulationCommandQueueDrainStepDispatchesQueuedCommands()
{
	dev::SimulationWorld world;
	dev::EventRecorder movementEvents;
	world.movementEvents = &movementEvents;
	world.players.push_back(MakePlayer({ 0, 0 }));
	dev::PlayerController controller { world.players, world.map, world.collision, world.pathFinder, &movementEvents };
	dev::CommandDispatcher dispatcher { controller, &movementEvents };
	dev::CommandQueue commands;
	commands.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});
	commands.push({
	    .type = dev::MovementCommandType::Stop,
	    .playerId = 0,
	    .destination = { 0, 0 },
	});

	const int drained = dev::SimulationCommandQueueDrainStep {}.drain(commands, dispatcher);

	dev::MovementCommand command;
	Expect(drained == 2, "simulation command queue drain step should report drained command count");
	Expect(!commands.tryPop(command), "simulation command queue drain step should empty command queue");
	Expect(movementEvents.events().size() >= 2, "simulation command queue drain step should dispatch commands through dispatcher");
	Expect(!movementEvents.events().empty() && movementEvents.events()[0].type == dev::MovementEventType::CommandAccepted, "simulation command queue drain step should preserve command dispatch events");
}

void TestSimulationCommandDrainerDispatchesQueuedMovementCommands()
{
	dev::SimulationWorld world;
	dev::EventRecorder movementEvents;
	world.movementEvents = &movementEvents;
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.commandQueue.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	    .destinationAction = std::nullopt,
	});

	const int drained = dev::SimulationCommandDrainer {}.drain(world);

	dev::MovementCommand command;
	Expect(drained == 1, "simulation command drainer should report drained world command count");
	Expect(!world.commandQueue.tryPop(command), "simulation command drainer should empty queued movement commands");
	Expect(world.players[0].moveState == dev::PlayerMoveState::Pathing, "simulation command drainer should dispatch movement commands through player controller");
	Expect(movementEvents.events().size() >= 2, "simulation command drainer should emit command and controller events");
	Expect(!movementEvents.events().empty() && movementEvents.events()[0].type == dev::MovementEventType::CommandAccepted, "simulation command drainer should emit command accepted event");
}

void TestRuntimeMovementCommandReportRecorderCopiesFrameAndAggregatesSummary()
{
	dev::RuntimeFrameReport frame;
	dev::RuntimeRunSummary summary;
	frame.movementCommandsQueued = 1;
	summary.movementCommandsQueued = 3;

	dev::RuntimeMovementCommandReportRecorder {}.recordQueuedCount(2, frame, summary);

	Expect(frame.movementCommandsQueued == 2, "runtime movement command report recorder should replace frame queued command count");
	Expect(summary.movementCommandsQueued == 5, "runtime movement command report recorder should aggregate queued command count into summary");
}

} // namespace

int main()
{
	TestQueuedMovementCommandSourceDrainsCommandsOnce();
	TestRuntimeMovementCommandQueueStepQueuesRouteResults();
	TestRuntimeMovementCommandIntakeQueuesCommandsInWorldOrder();
	TestSimulationCommandQueueDrainStepDispatchesQueuedCommands();
	TestSimulationCommandDrainerDispatchesQueuedMovementCommands();
	TestRuntimeMovementCommandReportRecorderCopiesFrameAndAggregatesSummary();

	if (Failures != 0) {
		std::cerr << Failures << " test(s) failed\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
