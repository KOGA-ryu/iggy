#include "scene/npc/NpcActorMovementRefreshFrame2D.hpp"

namespace iggy {

bool NpcActorMovementRefreshFrame2DResult::hasRefreshWork() const
{
	return work.hasWork();
}

bool NpcActorMovementRefreshFrame2DResult::refreshedAny() const
{
	return occupancyRefreshed
		|| interactionRefreshed
		|| aiMapRefreshed
		|| renderRefreshed
		|| visibilityRefreshed;
}

NpcActorMovementRefreshFrame2DResult NpcActorMovementRefreshFrameProjector2D::project(
	const NpcActorMovementRefreshFrame2DInput &input) const
{
	NpcActorMovementRefreshFrame2DResult result;
	result.input = input;
	result.work = NpcActorMovementRefreshWorkProjector2D {}.project(input.movementReport);
	result.dirtyTileCount = result.work.dirtyTiles.size();

	result.occupancy = NpcActorOccupancyRefresher2D {}.refresh(
		input.actors,
		input.previousOccupancy,
		result.work,
		input.config.occupancy);
	result.interaction = NpcActorInteractionRefresher2D {}.refresh(
		result.work,
		input.actors,
		input.interactionTargets,
		input.config.interaction);
	result.aiMap = NpcActorAiMapRefresher2D {}.refresh(
		result.work,
		input.actors,
		input.aiMap,
		input.config.aiMap);
	result.visual = NpcActorVisualRefresher2D {}.refresh(
		result.work,
		input.actors,
		input.config.visual);

	result.occupancyRefreshed = result.occupancy.refreshedOccupancy();
	result.interactionRefreshed = result.interaction.hasWork();
	result.aiMapRefreshed = result.aiMap.hasWork();
	result.renderRefreshed = result.visual.render.hasWork();
	result.visibilityRefreshed = result.visual.visibility.hasWork();
	return result;
}

} // namespace iggy
