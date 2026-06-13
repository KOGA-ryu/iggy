#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/draft/DraftBuildingCompile2D.hpp"

namespace iggy {

struct DraftLevelGeometryPlan2DConfig {
	bool allowIssueBearingCompile = false;
	float rotationToleranceRadians = 0.001F;
};

struct DraftLevelCollisionBox2D {
	ResourceId sourceWallId;
	std::size_t sourceWallIndex = 0;
	Vec2 center;
	Vec2 size;
	float rotationRadians = 0.0F;
	ResourceId assetId;
	ResourceId definitionId;
};

enum class DraftLevelGeometryPlan2DIssueCode {
	BuildingCompileHasIssues,
	NonPositiveSegmentSize,
	UnsupportedRotatedSegment,
};

struct DraftLevelGeometryPlan2DIssue {
	DraftLevelGeometryPlan2DIssueCode code = DraftLevelGeometryPlan2DIssueCode::BuildingCompileHasIssues;
	std::size_t segmentIndex = 0;
	DraftCompiledWallSegment2D segment;
	std::size_t buildingIssueCount = 0;
};

struct DraftLevelGeometryPlan2DResult {
	std::vector<DraftLevelCollisionBox2D> collisionBoxes;
	std::vector<DraftLevelGeometryPlan2DIssue> issues;
	std::size_t sourceWallSegmentCount = 0;
	bool buildingCompileHadIssues = false;

	[[nodiscard]] bool hasIssues() const;
};

class DraftLevelGeometryPlanner2D {
public:
	[[nodiscard]] DraftLevelGeometryPlan2DResult plan(
		const DraftBuildingCompile2DResult &building,
		const DraftLevelGeometryPlan2DConfig &config = {}) const;
};

} // namespace iggy
