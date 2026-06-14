#include "scene/npc/NpcActorOccupancy2D.hpp"

#include "scene/level/TileCoord.hpp"

namespace {

iggy::NpcActorOccupiedTile2D *FindOccupiedTile(
	std::vector<iggy::NpcActorOccupiedTile2D> &occupiedTiles,
	iggy::TileCoord tile)
{
	for (iggy::NpcActorOccupiedTile2D &occupiedTile : occupiedTiles) {
		if (occupiedTile.tile == tile) {
			return &occupiedTile;
		}
	}
	return nullptr;
}

iggy::NpcActorOccupancyIssue2D DuplicateIssue(
	const iggy::NpcActorOccupiedTile2D &occupiedTile,
	std::size_t laterActorIndex)
{
	return {
		iggy::NpcActorOccupancyIssue2DCode::DuplicateOccupiedTile,
		occupiedTile.tile,
		occupiedTile.actorIndexes.empty() ? 0 : occupiedTile.actorIndexes.front(),
		laterActorIndex,
		occupiedTile,
	};
}

} // namespace

namespace iggy {

bool NpcActorOccupancy2D::hasIssues() const
{
	return !issues.empty();
}

NpcActorOccupancy2D NpcActorOccupancyProjector2D::project(
	const NpcActorState2DRegistry &registry,
	const NpcActorOccupancy2DConfig &config) const
{
	NpcActorOccupancy2D result;

	for (std::size_t actorIndex = 0; actorIndex < registry.actors.size(); ++actorIndex) {
		const NpcActorState2D &actor = registry.actors[actorIndex];
		if (!actor.present && !config.includeAbsent) {
			continue;
		}

		const TileCoord tile = tileForPoint(actor.position);
		result.entries.push_back({
			actor.npcId,
			tile,
			actorIndex,
			actor,
		});

		NpcActorOccupiedTile2D *occupiedTile = FindOccupiedTile(result.occupiedTiles, tile);
		if (occupiedTile == nullptr) {
			result.occupiedTiles.push_back({
				tile,
				{ actor.npcId },
				{ actorIndex },
			});
			continue;
		}

		occupiedTile->npcIds.push_back(actor.npcId);
		occupiedTile->actorIndexes.push_back(actorIndex);
		result.issues.push_back(DuplicateIssue(*occupiedTile, actorIndex));
	}

	return result;
}

} // namespace iggy
