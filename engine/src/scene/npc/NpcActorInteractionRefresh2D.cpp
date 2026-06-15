#include "scene/npc/NpcActorInteractionRefresh2D.hpp"

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

std::vector<iggy::TileCoord> InteractionDirtyTiles(const iggy::NpcActorMovementRefreshWork2D &work)
{
	std::vector<iggy::TileCoord> dirtyTiles;
	for (const iggy::NpcActorMovementRefreshWork2DItem &item : work.items) {
		if (item.type != iggy::NpcActorMovementRefreshWork2DType::InteractionRefresh) {
			continue;
		}
		for (iggy::TileCoord tile : item.dirtyTiles) {
			AddDirtyTile(dirtyTiles, tile);
		}
	}
	return dirtyTiles;
}

void AddAffectedActors(
	iggy::NpcActorInteractionRefresh2DResult &result,
	const iggy::NpcActorState2DRegistry &actors,
	const iggy::NpcActorInteractionRefresh2DConfig &config)
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

		result.affectedActors.push_back({
			actor.npcId,
			tile,
			index,
			actor,
		});
	}
}

void AddAffectedTargets(
	iggy::NpcActorInteractionRefresh2DResult &result,
	const iggy::InteractionTarget2DRegistry &targets,
	const iggy::NpcActorInteractionRefresh2DConfig &config)
{
	const std::vector<iggy::InteractionTarget2D> &targetList = targets.targets();
	for (std::size_t index = 0; index < targetList.size(); ++index) {
		const iggy::InteractionTarget2D &target = targetList[index];
		if (!config.includeDisabledTargets && !target.enabled) {
			continue;
		}

		const iggy::TileCoord tile = iggy::tileForPoint(target.position);
		if (!ContainsTile(result.dirtyTiles, tile)) {
			continue;
		}

		result.affectedTargets.push_back({
			target.id,
			tile,
			index,
			target,
		});
	}
}

} // namespace

namespace iggy {

bool NpcActorInteractionRefresh2DResult::hasWork() const
{
	return refreshed;
}

bool NpcActorInteractionRefresh2DResult::hasAffectedFacts() const
{
	return !affectedActors.empty() || !affectedTargets.empty();
}

NpcActorInteractionRefresh2DResult NpcActorInteractionRefresher2D::refresh(
	const NpcActorMovementRefreshWork2D &work,
	const NpcActorState2DRegistry &actors,
	const InteractionTarget2DRegistry &targets,
	const NpcActorInteractionRefresh2DConfig &config) const
{
	NpcActorInteractionRefresh2DResult result;
	result.work = work;
	result.actors = actors;
	result.targets = targets;
	result.dirtyTiles = InteractionDirtyTiles(work);
	result.dirtyTileCount = result.dirtyTiles.size();

	if (result.dirtyTiles.empty()) {
		result.status = NpcActorInteractionRefresh2DStatus::NoRefreshNeeded;
		result.refreshed = false;
		return result;
	}

	AddAffectedActors(result, actors, config);
	AddAffectedTargets(result, targets, config);
	result.affectedActorCount = result.affectedActors.size();
	result.affectedTargetCount = result.affectedTargets.size();
	result.status = NpcActorInteractionRefresh2DStatus::Refreshed;
	result.refreshed = true;
	return result;
}

} // namespace iggy
