#include "RuntimeMovementScriptIntake.hpp"

#include "commands/CommandDispatcher.hpp"
#include "player/PlayerController.hpp"

namespace dev {

MovementScriptRunResult RuntimeMovementScriptIntake::run(
    const std::filesystem::path &path,
    SimulationWorld *world) const
{
	if (world == nullptr)
		return { .status = MovementScriptRunStatus::NoActiveWorld };

	PlayerController playerController {
		world->players,
		world->map,
		world->collision,
		world->pathFinder,
		world->movementEvents,
	};
	CommandDispatcher dispatcher { playerController, world->movementEvents };
	MovementScriptRunner runner { dispatcher };
	return runner.run(path);
}

} // namespace dev
