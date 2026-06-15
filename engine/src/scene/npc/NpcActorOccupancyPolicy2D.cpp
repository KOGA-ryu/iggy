#include "scene/npc/NpcActorOccupancyPolicy2D.hpp"

namespace {

iggy::ResourceId FirstBlockingNpcId(
	const iggy::NpcActorOccupancyQuery2DResult &occupancy,
	const iggy::ResourceId &movingNpcId)
{
	if (occupancy.npcIds.empty()) {
		return {};
	}
	if (movingNpcId.empty()) {
		return occupancy.npcIds.front();
	}
	for (const iggy::ResourceId &npcId : occupancy.npcIds) {
		if (npcId != movingNpcId) {
			return npcId;
		}
	}
	return {};
}

} // namespace

namespace iggy {

bool NpcActorOccupancyPolicy2DResult::allowed() const
{
	return status == NpcActorOccupancyPolicy2DStatus::Allowed;
}

bool NpcActorOccupancyPolicy2DResult::blocked() const
{
	return status == NpcActorOccupancyPolicy2DStatus::Blocked;
}

NpcActorOccupancyPolicy2DResult NpcActorOccupancyPolicy2D::evaluate(
	const NpcActorOccupancy2D &occupancy,
	const ResourceId &movingNpcId,
	TileCoord tile,
	const NpcActorOccupancyPolicy2DConfig &config) const
{
	NpcActorOccupancyPolicy2DResult result;
	result.occupancy = npcActorOccupantsAt(occupancy, tile);
	result.movingNpcId = movingNpcId;
	result.tile = tile;
	result.occupancyCount = result.occupancy.npcIds.size();
	result.capacity = config.maxOccupantsPerTile;

	for (const ResourceId &npcId : result.occupancy.npcIds) {
		if (movingNpcId.empty() || npcId != movingNpcId) {
			++result.effectiveOccupancyCount;
		}
	}

	if (result.occupancy.npcIds.empty()) {
		result.status = config.maxOccupantsPerTile > 0 ? NpcActorOccupancyPolicy2DStatus::Allowed : NpcActorOccupancyPolicy2DStatus::Blocked;
		return result;
	}

	if (movingNpcId.empty()) {
		result.blockingNpcId = FirstBlockingNpcId(result.occupancy, movingNpcId);
		result.status = NpcActorOccupancyPolicy2DStatus::Blocked;
		return result;
	}

	const std::size_t projectedOccupants = result.effectiveOccupancyCount + 1;
	if (projectedOccupants <= config.maxOccupantsPerTile) {
		result.status = NpcActorOccupancyPolicy2DStatus::Allowed;
		return result;
	}

	result.blockingNpcId = FirstBlockingNpcId(result.occupancy, movingNpcId);
	result.status = NpcActorOccupancyPolicy2DStatus::Blocked;
	return result;
}

} // namespace iggy
