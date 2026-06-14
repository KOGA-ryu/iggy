#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/draft/DraftLevelGeometryPlan2D.hpp"

namespace iggy {

struct LevelCollisionSource2DConfig {
	bool allowIssueBearingPlan = false;
	float rotationToleranceRadians = 0.001F;
};

struct LevelCollisionSourceBox2D {
	ResourceId sourceWallId;
	std::size_t sourceWallIndex = 0;
	Vec2 center;
	Vec2 size;
	float rotationRadians = 0.0F;
	ResourceId assetId;
	ResourceId definitionId;
};

// Derived compile output used as source input for collision cache/world builders.
// Project/editor truth remains authored draft symbols for now; custom collision
// should be authored there and compiled into this packet instead of hand-editing
// compiled boxes. This is not runtime/session-owned state.
struct LevelCollisionSource2D {
	std::vector<LevelCollisionSourceBox2D> boxes;
};

enum class LevelCollisionSource2DIssueCode {
	GeometryPlanHasIssues,
	NonPositiveBoxSize,
	UnsupportedRotation,
};

struct LevelCollisionSource2DIssue {
	LevelCollisionSource2DIssueCode code = LevelCollisionSource2DIssueCode::GeometryPlanHasIssues;
	std::size_t boxIndex = 0;
	DraftLevelCollisionBox2D box;
	std::size_t geometryIssueCount = 0;
};

struct LevelCollisionSource2DBuildResult {
	bool built = false;
	LevelCollisionSource2D source;
	std::vector<LevelCollisionSource2DIssue> issues;
	std::size_t sourceGeometryBoxCount = 0;
	bool geometryPlanHadIssues = false;

	[[nodiscard]] bool hasIssues() const;
};

class LevelCollisionSource2DBuilder {
public:
	[[nodiscard]] LevelCollisionSource2DBuildResult build(
		const DraftLevelGeometryPlan2DResult &geometry,
		const LevelCollisionSource2DConfig &config = {}) const;
};

} // namespace iggy
