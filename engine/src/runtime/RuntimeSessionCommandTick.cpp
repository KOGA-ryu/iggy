#include "runtime/RuntimeSessionCommandTick.hpp"

namespace iggy::runtime {

namespace {

RuntimeSessionCommandTickResult RunWithCollisionWorldRequest(
	const RuntimeSessionCommandTickInput &input,
	RuntimeCollisionWorldRequest request)
{
	const RuntimeCollisionWorldResult collisionWorld = RuntimeCollisionWorldProvider {}.resolve(input.session, request);

	RuntimeSessionCommandTickResult result;
	result.playerCommands = RuntimePlayerCommandStep {}.run(
		input.session,
		input.commandFrame,
		collisionWorld.world,
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

} // namespace

RuntimeSessionCommandTickResult RuntimeSessionCommandTick::run(const RuntimeSessionCommandTickInput &input) const
{
	return RunWithCollisionWorldRequest(input, {});
}

RuntimeSessionCommandTickResult RuntimeSessionCommandTick::run(
	const RuntimeSessionCommandTickInput &input,
	const physics2d::CollisionWorld2D &collisionWorld) const
{
	RuntimeCollisionWorldRequest request;
	request.explicitWorld = &collisionWorld;
	return RunWithCollisionWorldRequest(input, request);
}

} // namespace iggy::runtime
