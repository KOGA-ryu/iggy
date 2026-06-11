#include "PlayerMovement.hpp"

namespace dev {

PlayerMovement::PlayerMovement(const Collision &collision, ActionExecutor actionExecutor, MovementEventSink *eventSink)
    : animationLocks_(eventSink)
    , pathStepper_(collision, eventSink)
    , actions_(actionExecutor)
{
}

void PlayerMovement::update(std::vector<Player> &players, float deltaSeconds) const
{
	for (std::size_t index = 0; index < players.size(); ++index) {
		Player &player = players[index];
		const PlayerId playerId = static_cast<PlayerId>(index);
		if (!animationLocks_.advance(player, deltaSeconds))
			continue;

		if (player.path.empty()) {
			actions_.run(player, playerId);
			continue;
		}

		const PlayerPathStepResult step = pathStepper_.step(player);
		if (step.actionReady)
			actions_.run(player, playerId);
	}
}

} // namespace dev
