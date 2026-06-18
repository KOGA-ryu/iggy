#include "runtime/RuntimeGameplayProductInputFrameTargetContext.hpp"

namespace iggy::runtime {
namespace {

bool EligiblePrimaryTileEvent(const RuntimeGameplayProductInputEvent2D &event)
{
	return event.control == RuntimeGameplayProductInputControl2D::PrimaryTile &&
		event.kind == RuntimeGameplayProductInputEventKind::Pressed &&
		event.hasTile;
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
		if (!EligiblePrimaryTileEvent(event))
			continue;

		result.hasPrimaryTileEvent = true;
		result.primaryTileEventIndex = index;
		result.primaryTile = event.tile;
	}

	if (!result.hasPrimaryTileEvent) {
		result.status =
			RuntimeGameplayProductInputFrameTargetContextStatus::NoEligiblePrimaryTile;
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

	RuntimeGameplayProductInputTargetContextInput targetContextInput;
	targetContextInput.base = input.frame.bindingContext;
	targetContextInput.target = result.target;
	result.targetContext =
		RuntimeGameplayProductInputTargetContext {}.project(targetContextInput);
	result.frame.bindingContext = result.targetContext.bindingContext;

	if (result.targetContext.status ==
		RuntimeGameplayProductInputTargetContextStatus::TargetProjected) {
		result.status =
			RuntimeGameplayProductInputFrameTargetContextStatus::TargetProjected;
	}

	return result;
}

} // namespace iggy::runtime
