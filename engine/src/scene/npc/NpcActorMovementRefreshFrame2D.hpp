#pragma once

#include <cstddef>

#include "scene/ai/AiMap2D.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"
#include "scene/npc/NpcActorAiMapRefresh2D.hpp"
#include "scene/npc/NpcActorInteractionRefresh2D.hpp"
#include "scene/npc/NpcActorMovementFrameReport2D.hpp"
#include "scene/npc/NpcActorMovementRefreshWork2D.hpp"
#include "scene/npc/NpcActorOccupancyRefresh2D.hpp"
#include "scene/npc/NpcActorVisualRefresh2D.hpp"

namespace iggy {

struct NpcActorMovementRefreshFrame2DConfig {
	NpcActorOccupancyRefresh2DConfig occupancy;
	NpcActorInteractionRefresh2DConfig interaction;
	NpcActorAiMapRefresh2DConfig aiMap;
	NpcActorVisualRefresh2DConfig visual;
};

struct NpcActorMovementRefreshFrame2DInput {
	NpcActorMovementFrameReport2D movementReport;
	NpcActorState2DRegistry actors;
	NpcActorOccupancy2D previousOccupancy;
	InteractionTarget2DRegistry interactionTargets;
	AiMap2D aiMap;
	NpcActorMovementRefreshFrame2DConfig config;
};

struct NpcActorMovementRefreshFrame2DResult {
	NpcActorMovementRefreshFrame2DInput input;
	NpcActorMovementRefreshWork2D work;
	NpcActorOccupancyRefresh2DResult occupancy;
	NpcActorInteractionRefresh2DResult interaction;
	NpcActorAiMapRefresh2DResult aiMap;
	NpcActorVisualRefresh2DResult visual;
	std::size_t dirtyTileCount = 0;
	bool occupancyRefreshed = false;
	bool interactionRefreshed = false;
	bool aiMapRefreshed = false;
	bool renderRefreshed = false;
	bool visibilityRefreshed = false;

	[[nodiscard]] bool hasRefreshWork() const;
	[[nodiscard]] bool refreshedAny() const;
};

class NpcActorMovementRefreshFrameProjector2D {
public:
	[[nodiscard]] NpcActorMovementRefreshFrame2DResult project(
		const NpcActorMovementRefreshFrame2DInput &input) const;
};

} // namespace iggy
