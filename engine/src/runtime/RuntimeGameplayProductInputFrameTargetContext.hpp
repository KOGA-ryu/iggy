#pragma once

#include <cstddef>

#include "runtime/RuntimeGameplayProductInputAdapter.hpp"
#include "runtime/RuntimeGameplayProductInputTargetContext.hpp"
#include "runtime/RuntimeGameplayProductInteractionTargetQuery.hpp"
#include "runtime/RuntimeGameplayProductPlayMode.hpp"
#include "scene/interaction/InteractionReach2D.hpp"
#include "scene/interaction/InteractionTargetSpatialQuery2D.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayProductInputFrameTargetContextStatus {
	Unchanged,
	NoEligiblePrimaryTile,
	TargetProjected,
};

struct RuntimeGameplayProductInputFrameTargetContextInput {
	RuntimeGameplayProductPlayModeState state;
	RuntimeGameplayProductInputFrame2D frame;
	InteractionTargetSpatialQuery2DConfig spatialConfig;
	InteractionReach2DConfig reachConfig;
};

struct RuntimeGameplayProductInputFrameTargetContextResult {
	RuntimeGameplayProductInputFrameTargetContextStatus status =
		RuntimeGameplayProductInputFrameTargetContextStatus::Unchanged;
	RuntimeGameplayProductInputFrame2D frame;
	bool hasPrimaryTileEvent = false;
	std::size_t primaryTileEventIndex = 0;
	TileCoord primaryTile;
	RuntimeGameplayProductInteractionTargetQueryResult target;
	RuntimeGameplayProductInputTargetContextResult targetContext;
};

class RuntimeGameplayProductInputFrameTargetContext {
public:
	[[nodiscard]] RuntimeGameplayProductInputFrameTargetContextResult enrich(
		const RuntimeGameplayProductInputFrameTargetContextInput &input) const;
};

} // namespace iggy::runtime
