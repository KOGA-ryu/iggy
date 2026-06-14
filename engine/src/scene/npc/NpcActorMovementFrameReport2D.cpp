#include "scene/npc/NpcActorMovementFrameReport2D.hpp"

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

iggy::NpcActorMovementFrameEvent2D EventForEntry(const iggy::NpcActorMovementFrameApply2DEntry &entry)
{
	if (entry.hasIssue) {
		return iggy::NpcActorMovementFrameEvent2D::ActorNotFound;
	}

	switch (entry.executor.status) {
	case iggy::NpcActorMovementExecutor2DStatus::Moved:
		return iggy::NpcActorMovementFrameEvent2D::MovementApplied;
	case iggy::NpcActorMovementExecutor2DStatus::BlockedByNpc:
		return iggy::NpcActorMovementFrameEvent2D::MovementBlocked;
	case iggy::NpcActorMovementExecutor2DStatus::NoMovement:
		return iggy::NpcActorMovementFrameEvent2D::MovementNoOp;
	case iggy::NpcActorMovementExecutor2DStatus::Rejected:
	case iggy::NpcActorMovementExecutor2DStatus::ActorMismatch:
	case iggy::NpcActorMovementExecutor2DStatus::ActorNotPresent:
		return iggy::NpcActorMovementFrameEvent2D::MovementRejected;
	}

	return iggy::NpcActorMovementFrameEvent2D::MovementRejected;
}

void MergePostMoveFacts(
	iggy::NpcActorMovementFrameReport2D &report,
	const iggy::NpcActorPostMoveReport2D &postMove)
{
	for (iggy::TileCoord tile : postMove.dirtyTiles) {
		AddDirtyTile(report.dirtyTiles, tile);
	}

	report.needsOccupancyRebuild = report.needsOccupancyRebuild || postMove.needsOccupancyRebuild;
	report.needsAiMapQueryRefresh = report.needsAiMapQueryRefresh || postMove.needsAiMapQueryRefresh;
	report.needsInteractionRefresh = report.needsInteractionRefresh || postMove.needsInteractionRefresh;
	report.needsRenderRefresh = report.needsRenderRefresh || postMove.needsRenderRefresh;
	report.needsVisibilityRefresh = report.needsVisibilityRefresh || postMove.needsVisibilityRefresh;
}

void AddRefreshEvents(iggy::NpcActorMovementFrameReport2D &report)
{
	if (report.needsOccupancyRebuild) {
		report.events.push_back(iggy::NpcActorMovementFrameEvent2D::RefreshNeeded);
	}
	if (report.needsAiMapQueryRefresh) {
		report.events.push_back(iggy::NpcActorMovementFrameEvent2D::RefreshNeeded);
	}
	if (report.needsInteractionRefresh) {
		report.events.push_back(iggy::NpcActorMovementFrameEvent2D::RefreshNeeded);
	}
	if (report.needsRenderRefresh) {
		report.events.push_back(iggy::NpcActorMovementFrameEvent2D::RefreshNeeded);
	}
	if (report.needsVisibilityRefresh) {
		report.events.push_back(iggy::NpcActorMovementFrameEvent2D::RefreshNeeded);
	}
}

} // namespace

namespace iggy {

bool NpcActorMovementFrameReport2D::changed() const
{
	return apply.changed;
}

bool NpcActorMovementFrameReport2D::hasDirtyTiles() const
{
	return !dirtyTiles.empty();
}

NpcActorMovementFrameReport2D NpcActorMovementFrameReporter2D::report(
	const NpcActorMovementFrameApply2DResult &apply) const
{
	NpcActorMovementFrameReport2D result;
	result.apply = apply;
	result.registry = apply.registry;
	result.requestCount = apply.requestCount;
	result.entryCount = apply.entries.size();
	result.movedCount = apply.movedCount;
	result.blockedCount = apply.blockedCount;
	result.rejectedCount = apply.rejectedCount;
	result.noMovementCount = apply.noMovementCount;
	result.missingActorCount = apply.missingActorCount;
	result.changedCount = apply.changedCount;

	for (const NpcActorMovementFrameApply2DEntry &entry : apply.entries) {
		result.events.push_back(EventForEntry(entry));
		if (!entry.hasIssue) {
			MergePostMoveFacts(result, entry.executor.postMove);
		}
	}

	for (TileCoord tile : result.dirtyTiles) {
		(void)tile;
		result.events.push_back(NpcActorMovementFrameEvent2D::DirtyTileObserved);
	}

	AddRefreshEvents(result);

	result.events.push_back(apply.changed
		? NpcActorMovementFrameEvent2D::MovementFrameChanged
		: NpcActorMovementFrameEvent2D::MovementFrameUnchanged);

	return result;
}

} // namespace iggy
