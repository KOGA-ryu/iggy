#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/draft/DraftWallDoorMorphPlan2D.hpp"

namespace iggy {

struct DraftWallCutCompile2DConfig {
	float rotationToleranceRadians = 0.001F;
	float minimumSegmentSize = 0.001F;
};

struct DraftCompiledWallSegment2D {
	ResourceId sourceWallId;
	std::size_t sourceWallIndex = 0;
	Vec2 center;
	Vec2 size;
	float rotationRadians = 0.0F;
	ResourceId assetId;
	ResourceId definitionId;
	bool hasDoorCut = false;
	std::vector<ResourceId> sourceDoorIds;
};

enum class DraftWallCutCompile2DIssueCode {
	UnsupportedRotatedWall,
	DoorLargerThanWall,
	DoorOutsideWallBounds,
	OverlappingDoorCuts,
	NoPositiveSegmentProduced,
};

struct DraftWallCutCompile2DIssue {
	DraftWallCutCompile2DIssueCode code = DraftWallCutCompile2DIssueCode::UnsupportedRotatedWall;
	std::size_t wallPlanIndex = 0;
	std::size_t wallIndex = 0;
	DraftCompiledWall2D wall;
	std::vector<DraftWallDoorMorphPlanDoor2D> doors;
};

struct DraftWallCutCompile2DResult {
	std::vector<DraftCompiledWallSegment2D> segments;
	std::vector<DraftWallCutCompile2DIssue> issues;

	[[nodiscard]] bool hasIssues() const;
};

class DraftWallCutCompiler2D {
public:
	[[nodiscard]] DraftWallCutCompile2DResult compile(
		const DraftWallDoorMorphPlan2DResult &plan,
		const DraftWallCutCompile2DConfig &config = {}) const;
};

} // namespace iggy
