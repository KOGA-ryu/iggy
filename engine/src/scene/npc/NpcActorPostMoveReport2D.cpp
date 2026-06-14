#include "scene/npc/NpcActorPostMoveReport2D.hpp"

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

void MarkMovedRefreshes(iggy::NpcActorPostMoveReport2D &report)
{
	report.needsOccupancyRebuild = true;
	report.needsAiMapQueryRefresh = true;
	report.needsInteractionRefresh = true;
	report.needsRenderRefresh = true;
	report.needsVisibilityRefresh = true;
}

} // namespace

namespace iggy {

bool NpcActorPostMoveReport2D::moved() const
{
	return status == NpcActorPostMoveReport2DStatus::Moved;
}

bool NpcActorPostMoveReport2D::blocked() const
{
	return status == NpcActorPostMoveReport2DStatus::Blocked;
}

bool NpcActorPostMoveReport2D::hasDirtyTiles() const
{
	return !dirtyTiles.empty();
}

NpcActorPostMoveReport2D NpcActorPostMoveReporter2D::report(
	const NpcActorPostMoveReport2DInput &input) const
{
	NpcActorPostMoveReport2D result;
	result.npcId = input.npcId;
	result.oldPosition = input.oldPosition;
	result.newPosition = input.newPosition;
	result.oldTile = tileForPoint(input.oldPosition);
	result.newTile = tileForPoint(input.newPosition);
	result.status = input.status;
	result.blockingKind = input.blockingKind;
	result.blockingNpcId = input.blockingNpcId;

	if (result.status == NpcActorPostMoveReport2DStatus::Moved) {
		result.blockingKind = NpcActorPostMoveBlockingKind2D::None;
		result.blockingNpcId = {};
		AddDirtyTile(result.dirtyTiles, result.oldTile);
		AddDirtyTile(result.dirtyTiles, result.newTile);
		MarkMovedRefreshes(result);
		return result;
	}

	return result;
}

NpcActorPostMoveReport2D NpcActorPostMoveReporter2D::report(
	const ResourceId &npcId,
	Vec2 oldPosition,
	Vec2 newPosition,
	NpcActorPostMoveReport2DStatus status,
	NpcActorPostMoveBlockingKind2D blockingKind,
	const ResourceId &blockingNpcId) const
{
	return report({
		npcId,
		oldPosition,
		newPosition,
		status,
		blockingKind,
		blockingNpcId,
	});
}

} // namespace iggy
