#include "runtime/RuntimeGameplayProductInputFrameTargetContext.hpp"

namespace iggy::runtime {
namespace {

bool EligiblePrimaryTileEvent(const RuntimeGameplayProductInputEvent2D &event)
{
	return event.control == RuntimeGameplayProductInputControl2D::PrimaryTile &&
		event.kind == RuntimeGameplayProductInputEventKind::Pressed &&
		event.hasTile;
}

bool EligibleActionTargetEvent(const RuntimeGameplayProductInputEvent2D &event)
{
	const bool actionNeedsTarget =
		event.control == RuntimeGameplayProductInputControl2D::Interact ||
		event.control == RuntimeGameplayProductInputControl2D::Inspect;
	return actionNeedsTarget &&
		event.kind == RuntimeGameplayProductInputEventKind::Pressed &&
		(!event.hasTargetId || event.targetId.empty());
}

InteractionTargetSpatialQuery2DConfig ActionSpatialConfig(
	const InteractionReach2DConfig &reachConfig)
{
	InteractionTargetSpatialQuery2DConfig config;
	config.extraRadius = reachConfig.extraReach;
	return config;
}

RuntimeGameplayProductInteractionTargetQueryResult FindTargetAtPlayerReach(
	const RuntimeGameplayProductInputFrameTargetContextInput &input)
{
	RuntimeGameplayProductInteractionTargetQueryResult result;
	if (!input.state.loop.loaded)
		return result;
	if (!input.state.loop.currentState.session.hasPlayer) {
		result.status =
			RuntimeGameplayProductInteractionTargetQueryStatus::MissingQuery;
		return result;
	}

	RuntimeGameplayProductInteractionTargetQueryInput queryInput;
	queryInput.state = input.state;
	queryInput.kind =
		RuntimeGameplayProductInteractionTargetQueryKind::Point;
	queryInput.point =
		input.state.loop.currentState.session.player.position;
	queryInput.spatialConfig = ActionSpatialConfig(input.reachConfig);
	queryInput.reachConfig = input.reachConfig;
	return RuntimeGameplayProductInteractionTargetQuery {}.find(queryInput);
}

void ProjectTargetContext(
	RuntimeGameplayProductInputFrameTargetContextResult &result)
{
	RuntimeGameplayProductInputTargetContextInput targetContextInput;
	targetContextInput.base = result.frame.bindingContext;
	targetContextInput.target = result.target;
	result.targetContext =
		RuntimeGameplayProductInputTargetContext {}.project(targetContextInput);
	result.frame.bindingContext = result.targetContext.bindingContext;

	if (result.targetContext.status ==
		RuntimeGameplayProductInputTargetContextStatus::TargetProjected) {
		result.status =
			RuntimeGameplayProductInputFrameTargetContextStatus::TargetProjected;
	}
}

} // namespace

RuntimeGameplayProductInputFrameTargetContextResult RuntimeGameplayProductInputFrameTargetContext::enrich(
	const RuntimeGameplayProductInputFrameTargetContextInput &input) const
{
	RuntimeGameplayProductInputFrameTargetContextResult result;
	result.frame = input.frame;

	for (std::size_t index = 0; index < input.frame.events.size(); ++index) {
		const RuntimeGameplayProductInputEvent2D &event =
			input.frame.events[index];
		if (EligiblePrimaryTileEvent(event)) {
			result.hasPrimaryTileEvent = true;
			result.primaryTileEventIndex = index;
			result.primaryTile = event.tile;
		}
		if (EligibleActionTargetEvent(event)) {
			result.hasActionTargetEvent = true;
			result.actionTargetEventIndex = index;
			result.actionTargetControl = event.control;
		}
	}

	if (!result.hasPrimaryTileEvent) {
		if (!result.hasActionTargetEvent) {
			result.status =
				RuntimeGameplayProductInputFrameTargetContextStatus::NoEligiblePrimaryTile;
			return result;
		}

		result.target = FindTargetAtPlayerReach(input);
		if (result.target.status ==
				RuntimeGameplayProductInteractionTargetQueryStatus::TargetFound &&
			result.target.hasTarget &&
			result.target.hasReach &&
			result.target.reachable) {
			ProjectTargetContext(result);
		}
		return result;
	}

	RuntimeGameplayProductInteractionTargetQueryInput queryInput;
	queryInput.state = input.state;
	queryInput.kind =
		RuntimeGameplayProductInteractionTargetQueryKind::TileCenter;
	queryInput.tile = result.primaryTile;
	queryInput.spatialConfig = input.spatialConfig;
	queryInput.reachConfig = input.reachConfig;
	result.target =
		RuntimeGameplayProductInteractionTargetQuery {}.find(queryInput);

	ProjectTargetContext(result);

	return result;
}

} // namespace iggy::runtime
