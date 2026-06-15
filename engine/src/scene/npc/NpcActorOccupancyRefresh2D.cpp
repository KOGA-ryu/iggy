#include "scene/npc/NpcActorOccupancyRefresh2D.hpp"

namespace {

bool RequestsOccupancyRebuild(const iggy::NpcActorMovementRefreshWork2D &work)
{
	for (const iggy::NpcActorMovementRefreshWork2DItem &item : work.items) {
		if (item.type == iggy::NpcActorMovementRefreshWork2DType::OccupancyRebuild) {
			return true;
		}
	}
	return false;
}

} // namespace

namespace iggy {

bool NpcActorOccupancyRefresh2DResult::changed() const
{
	return refreshed;
}

bool NpcActorOccupancyRefresh2DResult::refreshedOccupancy() const
{
	return refreshed;
}

NpcActorOccupancyRefresh2DResult NpcActorOccupancyRefresher2D::refresh(
	const NpcActorState2DRegistry &registry,
	const NpcActorOccupancy2D &previousOccupancy,
	const NpcActorMovementRefreshWork2D &work,
	const NpcActorOccupancyRefresh2DConfig &config) const
{
	NpcActorOccupancyRefresh2DResult result;
	result.registry = registry;
	result.previousOccupancy = previousOccupancy;
	result.work = work;

	if (!RequestsOccupancyRebuild(work)) {
		result.occupancy = previousOccupancy;
		result.status = NpcActorOccupancyRefresh2DStatus::NoRefreshNeeded;
		result.refreshed = false;
		return result;
	}

	result.occupancy = NpcActorOccupancyProjector2D {}.project(registry, config.occupancy);
	result.status = NpcActorOccupancyRefresh2DStatus::Refreshed;
	result.refreshed = true;
	return result;
}

} // namespace iggy
