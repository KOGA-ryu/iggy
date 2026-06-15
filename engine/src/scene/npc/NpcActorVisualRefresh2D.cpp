#include "scene/npc/NpcActorVisualRefresh2D.hpp"

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

std::vector<iggy::TileCoord> DirtyTilesForType(
	const iggy::NpcActorMovementRefreshWork2D &work,
	iggy::NpcActorMovementRefreshWork2DType type)
{
	std::vector<iggy::TileCoord> dirtyTiles;
	for (const iggy::NpcActorMovementRefreshWork2DItem &item : work.items) {
		if (item.type != type) {
			continue;
		}
		for (iggy::TileCoord tile : item.dirtyTiles) {
			AddDirtyTile(dirtyTiles, tile);
		}
	}
	return dirtyTiles;
}

void AddAffectedActors(
	iggy::NpcActorVisualRefreshPacket2D &packet,
	const iggy::NpcActorState2DRegistry &actors,
	const iggy::NpcActorVisualRefresh2DConfig &config)
{
	for (std::size_t index = 0; index < actors.actors.size(); ++index) {
		const iggy::NpcActorState2D &actor = actors.actors[index];
		if (!config.includeAbsentActors && !actor.present) {
			continue;
		}

		const iggy::TileCoord tile = iggy::tileForPoint(actor.position);
		if (!ContainsTile(packet.dirtyTiles, tile)) {
			continue;
		}

		packet.affectedActors.push_back({
			actor.npcId,
			tile,
			index,
			actor,
		});
	}
}

iggy::NpcActorVisualRefreshPacket2D BuildPacket(
	const iggy::NpcActorMovementRefreshWork2D &work,
	const iggy::NpcActorState2DRegistry &actors,
	const iggy::NpcActorVisualRefresh2DConfig &config,
	iggy::NpcActorMovementRefreshWork2DType type)
{
	iggy::NpcActorVisualRefreshPacket2D packet;
	packet.dirtyTiles = DirtyTilesForType(work, type);
	packet.dirtyTileCount = packet.dirtyTiles.size();

	if (packet.dirtyTiles.empty()) {
		packet.status = iggy::NpcActorVisualRefresh2DStatus::NoRefreshNeeded;
		packet.refreshNeeded = false;
		return packet;
	}

	AddAffectedActors(packet, actors, config);
	packet.affectedActorCount = packet.affectedActors.size();
	packet.status = iggy::NpcActorVisualRefresh2DStatus::Refreshed;
	packet.refreshNeeded = true;
	return packet;
}

} // namespace

namespace iggy {

bool NpcActorVisualRefreshPacket2D::hasWork() const
{
	return refreshNeeded;
}

bool NpcActorVisualRefreshPacket2D::hasAffectedActors() const
{
	return !affectedActors.empty();
}

bool NpcActorVisualRefresh2DResult::hasWork() const
{
	return render.hasWork() || visibility.hasWork();
}

NpcActorVisualRefresh2DResult NpcActorVisualRefresher2D::refresh(
	const NpcActorMovementRefreshWork2D &work,
	const NpcActorState2DRegistry &actors,
	const NpcActorVisualRefresh2DConfig &config) const
{
	NpcActorVisualRefresh2DResult result;
	result.work = work;
	result.actors = actors;
	result.render = BuildPacket(
		work,
		actors,
		config,
		NpcActorMovementRefreshWork2DType::RenderRefresh);
	result.visibility = BuildPacket(
		work,
		actors,
		config,
		NpcActorMovementRefreshWork2DType::VisibilityRefresh);
	return result;
}

} // namespace iggy
