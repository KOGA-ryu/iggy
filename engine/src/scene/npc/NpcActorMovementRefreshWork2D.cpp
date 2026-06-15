#include "scene/npc/NpcActorMovementRefreshWork2D.hpp"

namespace {

void AddItem(
	iggy::NpcActorMovementRefreshWork2D &work,
	iggy::NpcActorMovementRefreshWork2DType type)
{
	iggy::NpcActorMovementRefreshWork2DItem item;
	item.type = type;
	item.dirtyTiles = work.dirtyTiles;
	work.items.push_back(item);
}

} // namespace

namespace iggy {

bool NpcActorMovementRefreshWork2D::hasWork() const
{
	return !items.empty();
}

NpcActorMovementRefreshWork2D NpcActorMovementRefreshWorkProjector2D::project(
	const NpcActorMovementFrameReport2D &report) const
{
	NpcActorMovementRefreshWork2D work;
	work.report = report;
	work.dirtyTiles = report.dirtyTiles;
	work.needsOccupancyRebuild = report.needsOccupancyRebuild;
	work.needsAiMapQueryRefresh = report.needsAiMapQueryRefresh;
	work.needsInteractionRefresh = report.needsInteractionRefresh;
	work.needsRenderRefresh = report.needsRenderRefresh;
	work.needsVisibilityRefresh = report.needsVisibilityRefresh;

	if (work.needsOccupancyRebuild) {
		AddItem(work, NpcActorMovementRefreshWork2DType::OccupancyRebuild);
	}
	if (work.needsAiMapQueryRefresh) {
		AddItem(work, NpcActorMovementRefreshWork2DType::AiMapQueryRefresh);
	}
	if (work.needsInteractionRefresh) {
		AddItem(work, NpcActorMovementRefreshWork2DType::InteractionRefresh);
	}
	if (work.needsRenderRefresh) {
		AddItem(work, NpcActorMovementRefreshWork2DType::RenderRefresh);
	}
	if (work.needsVisibilityRefresh) {
		AddItem(work, NpcActorMovementRefreshWork2DType::VisibilityRefresh);
	}

	return work;
}

} // namespace iggy
