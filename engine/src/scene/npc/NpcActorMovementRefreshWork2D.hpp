#pragma once

#include <vector>

#include "scene/level/TileCoord.hpp"
#include "scene/npc/NpcActorMovementFrameReport2D.hpp"

namespace iggy {

enum class NpcActorMovementRefreshWork2DType {
	OccupancyRebuild,
	AiMapQueryRefresh,
	InteractionRefresh,
	RenderRefresh,
	VisibilityRefresh,
};

struct NpcActorMovementRefreshWork2DItem {
	NpcActorMovementRefreshWork2DType type = NpcActorMovementRefreshWork2DType::OccupancyRebuild;
	std::vector<TileCoord> dirtyTiles;
};

struct NpcActorMovementRefreshWork2D {
	NpcActorMovementFrameReport2D report;
	std::vector<TileCoord> dirtyTiles;
	std::vector<NpcActorMovementRefreshWork2DItem> items;
	bool needsOccupancyRebuild = false;
	bool needsAiMapQueryRefresh = false;
	bool needsInteractionRefresh = false;
	bool needsRenderRefresh = false;
	bool needsVisibilityRefresh = false;

	[[nodiscard]] bool hasWork() const;
};

class NpcActorMovementRefreshWorkProjector2D {
public:
	[[nodiscard]] NpcActorMovementRefreshWork2D project(
		const NpcActorMovementFrameReport2D &report) const;
};

} // namespace iggy
