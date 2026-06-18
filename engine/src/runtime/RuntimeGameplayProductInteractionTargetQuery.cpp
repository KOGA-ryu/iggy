#include "runtime/RuntimeGameplayProductInteractionTargetQuery.hpp"

namespace iggy::runtime {

RuntimeGameplayProductInteractionTargetQueryResult RuntimeGameplayProductInteractionTargetQuery::find(
	const RuntimeGameplayProductInteractionTargetQueryInput &input) const
{
	RuntimeGameplayProductInteractionTargetQueryResult result;
	result.kind = input.kind;

	if (!input.state.loop.loaded)
		return result;

	const InteractionTarget2DRegistry &targets =
		input.state.loop.currentState.interaction.targets;
	switch (input.kind) {
	case RuntimeGameplayProductInteractionTargetQueryKind::None:
		result.status =
			RuntimeGameplayProductInteractionTargetQueryStatus::MissingQuery;
		return result;
	case RuntimeGameplayProductInteractionTargetQueryKind::Point:
		result.spatial = InteractionTargetSpatialQuery2D {}.find(
			targets,
			input.point,
			input.spatialConfig);
		break;
	case RuntimeGameplayProductInteractionTargetQueryKind::TileCenter:
		result.spatial = InteractionTargetSpatialQuery2D {}.findTileCenter(
			targets,
			input.tile,
			input.spatialConfig);
		break;
	}

	if (result.spatial.status ==
		InteractionTargetSpatialQuery2DStatus::NotFound) {
		result.status =
			RuntimeGameplayProductInteractionTargetQueryStatus::TargetNotFound;
		return result;
	}

	result.status =
		RuntimeGameplayProductInteractionTargetQueryStatus::TargetFound;
	result.targetId = result.spatial.targetId;
	result.target = result.spatial.target;
	result.hasTarget = true;

	if (!input.state.loop.currentState.session.hasPlayer)
		return result;

	result.hasPlayer = true;
	result.targetQuery =
		InteractionTargetQuery2D {}.find(targets, result.targetId);
	result.reach = InteractionReach2D {}.evaluate(
		input.state.loop.currentState.session.player.position,
		result.targetQuery,
		input.reachConfig);
	result.hasReach = true;
	result.reachable = result.reach.reachable();
	return result;
}

} // namespace iggy::runtime
