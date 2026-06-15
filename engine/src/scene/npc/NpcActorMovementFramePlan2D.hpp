#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "scene/level/LevelTileMap.hpp"
#include "scene/npc/NpcActorEscapeRouteTarget2D.hpp"
#include "scene/npc/NpcActorFrameState2D.hpp"
#include "scene/npc/NpcActorMovementFrameApply2D.hpp"
#include "scene/npc/NpcActorMovementFrameIntent2D.hpp"
#include "scene/npc/NpcActorNavigationRequest2D.hpp"
#include "scene/npc/NpcActorOccupancy2D.hpp"
#include "scene/npc/NpcActorPathReport2D.hpp"
#include "scene/npc/NpcActorPathStep2D.hpp"
#include "scene/npc/NpcActorPathStepOccupancyFilter2D.hpp"
#include "scene/npc/NpcActorRouteTarget2D.hpp"

namespace iggy {

enum class NpcActorMovementFramePlan2DStatus {
	Planned,
	NoRequests,
};

enum class NpcActorMovementFramePlan2DEntryStatus {
	RequestPrepared,
	NoMovementIntent,
	RouteTargetFailed,
	EscapeRouteTargetFailed,
	NavigationRequestFailed,
	PathNotFound,
	StepNotProposed,
};

struct NpcActorMovementFramePlan2DConfig {
	NpcActorMovementIntent2DConfig intent;
	NpcActorRouteTarget2DConfig route;
	NpcActorEscapeRouteTarget2DConfig escapeRoute;
	NpcActorOccupancy2DConfig occupancy;
	NpcActorPathStep2DConfig pathStep;
	NpcActorPathStepOccupancyFilter2DConfig occupancyFilter;
};

struct NpcActorMovementFramePlan2DEntry {
	std::size_t frameIndex = 0;
	NpcActorMovementIntent2D intent;
	NpcActorRouteTarget2D route;
	NpcActorEscapeRouteTarget2D escapeRoute;
	NpcActorNavigationRequest2D navigation;
	NpcActorPathReport2D path;
	NpcActorPathStep2D step;
	NpcActorPathStepOccupancyFilter2D filter;
	std::optional<std::size_t> requestIndex;
	NpcActorMovementFrameApply2DRequest request;
	NpcActorMovementFramePlan2DEntryStatus status = NpcActorMovementFramePlan2DEntryStatus::NoMovementIntent;
	bool requestPrepared = false;
};

struct NpcActorMovementFramePlan2DResult {
	NpcActorFrameState2DProjectionResult frameState;
	NpcActorMovementFrameIntent2DResult movementIntents;
	NpcActorOccupancy2D occupancy;
	std::vector<NpcActorMovementFramePlan2DEntry> entries;
	std::vector<NpcActorMovementFrameApply2DRequest> requests;
	std::size_t entryCount = 0;
	std::size_t requestCount = 0;
	std::size_t preparedCount = 0;
	std::size_t blockedRequestCount = 0;
	std::size_t noMovementIntentCount = 0;
	std::size_t routeFailedCount = 0;
	std::size_t escapeRouteFailedCount = 0;
	std::size_t navigationFailedCount = 0;
	std::size_t pathFailedCount = 0;
	std::size_t stepNotProposedCount = 0;
	NpcActorMovementFramePlan2DStatus status = NpcActorMovementFramePlan2DStatus::NoRequests;

	[[nodiscard]] bool hasRequests() const;
};

class NpcActorMovementFramePlanner2D {
public:
	[[nodiscard]] NpcActorMovementFramePlan2DResult plan(
		const NpcActorState2DRegistry &actors,
		const NpcActorControlState2DRegistry &controls,
		const LevelTileMap &map,
		const NpcActorMovementFramePlan2DConfig &config = {}) const;
};

} // namespace iggy
