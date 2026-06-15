#include "scene/npc/NpcActorAiMapRefresh2D.hpp"

#include "scene/level/TileCoord.hpp"

namespace {

void AddDirtyTile(std::vector<iggy::TileCoord> &dirtyTiles, iggy::TileCoord tile)
{
	for (iggy::TileCoord existing : dirtyTiles) {
		if (existing == tile) {
			return;
		}
	}
	dirtyTiles.push_back(tile);
}

bool ContainsTile(const std::vector<iggy::TileCoord> &tiles, iggy::TileCoord tile)
{
	for (iggy::TileCoord existing : tiles) {
		if (existing == tile) {
			return true;
		}
	}
	return false;
}

std::vector<iggy::TileCoord> AiMapDirtyTiles(const iggy::NpcActorMovementRefreshWork2D &work)
{
	std::vector<iggy::TileCoord> dirtyTiles;
	for (const iggy::NpcActorMovementRefreshWork2DItem &item : work.items) {
		if (item.type != iggy::NpcActorMovementRefreshWork2DType::AiMapQueryRefresh) {
			continue;
		}
		for (iggy::TileCoord tile : item.dirtyTiles) {
			AddDirtyTile(dirtyTiles, tile);
		}
	}
	return dirtyTiles;
}

void AddAffectedActors(
	iggy::NpcActorAiMapRefresh2DResult &result,
	const iggy::NpcActorState2DRegistry &actors,
	const iggy::AiMap2D &map,
	const iggy::NpcActorAiMapRefresh2DConfig &config)
{
	for (std::size_t index = 0; index < actors.actors.size(); ++index) {
		const iggy::NpcActorState2D &actor = actors.actors[index];
		if (!config.includeAbsentActors && !actor.present) {
			continue;
		}

		const iggy::TileCoord tile = iggy::tileForPoint(actor.position);
		if (!ContainsTile(result.dirtyTiles, tile)) {
			continue;
		}

		iggy::NpcActorAiMapRefreshActor2D affected;
		affected.npcId = actor.npcId;
		affected.tile = tile;
		affected.actorIndex = index;
		affected.actor = actor;
		if (config.queryAffectedActors) {
			affected.query = iggy::AiMapQuery2D {}.query(map, actor.position);
		} else {
			affected.query.position = actor.position;
		}
		result.affectedActors.push_back(affected);
	}
}

} // namespace

namespace iggy {

bool NpcActorAiMapRefresh2DResult::hasWork() const
{
	return refreshed;
}

bool NpcActorAiMapRefresh2DResult::hasAffectedActors() const
{
	return !affectedActors.empty();
}

NpcActorAiMapRefresh2DResult NpcActorAiMapRefresher2D::refresh(
	const NpcActorMovementRefreshWork2D &work,
	const NpcActorState2DRegistry &actors,
	const AiMap2D &map,
	const NpcActorAiMapRefresh2DConfig &config) const
{
	NpcActorAiMapRefresh2DResult result;
	result.work = work;
	result.actors = actors;
	result.map = map;
	result.dirtyTiles = AiMapDirtyTiles(work);
	result.dirtyTileCount = result.dirtyTiles.size();

	if (result.dirtyTiles.empty()) {
		result.status = NpcActorAiMapRefresh2DStatus::NoRefreshNeeded;
		result.refreshed = false;
		return result;
	}

	AddAffectedActors(result, actors, map, config);
	result.affectedActorCount = result.affectedActors.size();
	result.status = NpcActorAiMapRefresh2DStatus::Refreshed;
	result.refreshed = true;
	return result;
}

} // namespace iggy
