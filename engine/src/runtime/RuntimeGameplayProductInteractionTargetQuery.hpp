#pragma once

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "runtime/RuntimeGameplayProductPlayMode.hpp"
#include "scene/interaction/InteractionReach2D.hpp"
#include "scene/interaction/InteractionTargetQuery2D.hpp"
#include "scene/interaction/InteractionTargetSpatialQuery2D.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayProductInteractionTargetQueryKind {
	None,
	Point,
	TileCenter,
};

enum class RuntimeGameplayProductInteractionTargetQueryStatus {
	NotLoaded,
	MissingQuery,
	TargetNotFound,
	TargetFound,
};

struct RuntimeGameplayProductInteractionTargetQueryInput {
	RuntimeGameplayProductPlayModeState state;
	RuntimeGameplayProductInteractionTargetQueryKind kind =
		RuntimeGameplayProductInteractionTargetQueryKind::None;
	Vec2 point;
	TileCoord tile;
	InteractionTargetSpatialQuery2DConfig spatialConfig;
	InteractionReach2DConfig reachConfig;
};

struct RuntimeGameplayProductInteractionTargetQueryResult {
	RuntimeGameplayProductInteractionTargetQueryStatus status =
		RuntimeGameplayProductInteractionTargetQueryStatus::NotLoaded;
	RuntimeGameplayProductInteractionTargetQueryKind kind =
		RuntimeGameplayProductInteractionTargetQueryKind::None;
	InteractionTargetSpatialQuery2DResult spatial;
	ResourceId targetId;
	InteractionTarget2D target;
	bool hasTarget = false;
	bool hasPlayer = false;
	bool hasReach = false;
	bool reachable = false;
	InteractionTargetQuery2DResult targetQuery;
	InteractionReach2DResult reach;
};

class RuntimeGameplayProductInteractionTargetQuery {
public:
	[[nodiscard]] RuntimeGameplayProductInteractionTargetQueryResult find(
		const RuntimeGameplayProductInteractionTargetQueryInput &input) const;
};

} // namespace iggy::runtime
