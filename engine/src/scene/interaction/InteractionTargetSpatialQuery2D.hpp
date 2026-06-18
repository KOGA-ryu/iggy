#pragma once

#include <cstddef>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy {

enum class InteractionTargetSpatialQuery2DStatus {
	Found,
	NotFound,
};

struct InteractionTargetSpatialQuery2DConfig {
	float extraRadius = 0.0F;
};

struct InteractionTargetSpatialQuery2DResult {
	InteractionTargetSpatialQuery2DStatus status =
		InteractionTargetSpatialQuery2DStatus::NotFound;
	ResourceId targetId;
	InteractionTarget2D target;
	std::size_t targetIndex = 0;
	float distance = 0.0F;
	float allowedDistance = 0.0F;

	[[nodiscard]] bool hasTarget() const;
};

class InteractionTargetSpatialQuery2D {
public:
	[[nodiscard]] InteractionTargetSpatialQuery2DResult find(
		const InteractionTarget2DRegistry &registry,
		Vec2 point,
		const InteractionTargetSpatialQuery2DConfig &config = {}) const;

	[[nodiscard]] InteractionTargetSpatialQuery2DResult findTileCenter(
		const InteractionTarget2DRegistry &registry,
		TileCoord tile,
		const InteractionTargetSpatialQuery2DConfig &config = {}) const;
};

} // namespace iggy
