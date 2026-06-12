#include "runtime/RuntimeSessionCommandTick.hpp"

namespace iggy::runtime {

RuntimeSessionCommandTickResult RuntimeSessionCommandTick::run(
	const RuntimeSessionCommandTickInput &input,
	const physics2d::CollisionWorld2D &collisionWorld) const
{
	RuntimeSessionCommandTickResult result;
	result.playerCommands = RuntimePlayerCommandStep {}.run(
		input.session,
		input.commandFrame,
		collisionWorld,
		input.playerCommandConfig);

	result.npcTargetPosition = input.fallbackPlayerPosition;
	if (result.playerCommands.session.hasPlayer)
		result.npcTargetPosition = result.playerCommands.session.player.position;

	result.tick = RuntimeSessionTick {}.run({
		result.playerCommands.session,
		result.npcTargetPosition,
		input.npcConfig,
	});
	result.session = result.tick.session;
	return result;
}

} // namespace iggy::runtime
