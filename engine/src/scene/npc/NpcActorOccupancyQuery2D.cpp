#include "scene/npc/NpcActorOccupancyQuery2D.hpp"

namespace iggy {

bool NpcActorOccupancyQuery2DResult::occupied() const
{
	return status == NpcActorOccupancyQuery2DStatus::Occupied;
}

bool NpcActorOccupancyBlock2DResult::blocked() const
{
	return status == NpcActorOccupancyBlock2DStatus::Blocked;
}

NpcActorOccupancyQuery2DResult NpcActorOccupancyQuery2D::occupantsAt(
	const NpcActorOccupancy2D &occupancy,
	TileCoord tile) const
{
	NpcActorOccupancyQuery2DResult result;
	result.tile = tile;

	for (const NpcActorOccupancyEntry2D &entry : occupancy.entries) {
		if (entry.tile != tile) {
			continue;
		}

		result.entries.push_back(entry);
		result.npcIds.push_back(entry.npcId);
		result.actorIndexes.push_back(entry.actorIndex);
	}

	if (!result.entries.empty()) {
		result.status = NpcActorOccupancyQuery2DStatus::Occupied;
	}

	return result;
}

const NpcActorOccupancyEntry2D *NpcActorOccupancyQuery2D::firstOccupantAt(
	const NpcActorOccupancy2D &occupancy,
	TileCoord tile) const
{
	for (const NpcActorOccupancyEntry2D &entry : occupancy.entries) {
		if (entry.tile == tile) {
			return &entry;
		}
	}
	return nullptr;
}

bool NpcActorOccupancyQuery2D::contains(const NpcActorOccupancy2D &occupancy, TileCoord tile) const
{
	return firstOccupantAt(occupancy, tile) != nullptr;
}

bool NpcActorOccupancyQuery2D::isOccupied(const NpcActorOccupancy2D &occupancy, TileCoord tile) const
{
	return contains(occupancy, tile);
}

NpcActorOccupancyBlock2DResult NpcActorOccupancyQuery2D::blockedFor(
	const NpcActorOccupancy2D &occupancy,
	const ResourceId &movingNpcId,
	TileCoord tile) const
{
	NpcActorOccupancyBlock2DResult result;
	result.tile = tile;
	result.movingNpcId = movingNpcId;
	result.occupancy = occupantsAt(occupancy, tile);

	if (!result.occupancy.occupied()) {
		result.status = NpcActorOccupancyBlock2DStatus::Empty;
		return result;
	}

	if (movingNpcId.empty()) {
		result.status = NpcActorOccupancyBlock2DStatus::Blocked;
		return result;
	}

	for (const ResourceId &occupantId : result.occupancy.npcIds) {
		if (occupantId != movingNpcId) {
			result.status = NpcActorOccupancyBlock2DStatus::Blocked;
			return result;
		}
	}

	result.status = NpcActorOccupancyBlock2DStatus::OnlySelf;
	return result;
}

NpcActorOccupancyQuery2DResult npcActorOccupantsAt(
	const NpcActorOccupancy2D &occupancy,
	TileCoord tile)
{
	return NpcActorOccupancyQuery2D {}.occupantsAt(occupancy, tile);
}

const NpcActorOccupancyEntry2D *npcActorFirstOccupantAt(
	const NpcActorOccupancy2D &occupancy,
	TileCoord tile)
{
	return NpcActorOccupancyQuery2D {}.firstOccupantAt(occupancy, tile);
}

bool npcActorOccupancyContains(const NpcActorOccupancy2D &occupancy, TileCoord tile)
{
	return NpcActorOccupancyQuery2D {}.contains(occupancy, tile);
}

bool npcActorTileOccupied(const NpcActorOccupancy2D &occupancy, TileCoord tile)
{
	return NpcActorOccupancyQuery2D {}.isOccupied(occupancy, tile);
}

NpcActorOccupancyBlock2DResult npcActorTileBlockedFor(
	const NpcActorOccupancy2D &occupancy,
	const ResourceId &movingNpcId,
	TileCoord tile)
{
	return NpcActorOccupancyQuery2D {}.blockedFor(occupancy, movingNpcId, tile);
}

} // namespace iggy
