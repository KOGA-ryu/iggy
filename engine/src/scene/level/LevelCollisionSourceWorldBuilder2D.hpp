#pragma once

#include <cstddef>
#include <vector>

#include "scene/level/LevelCollisionSource2D.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy {

struct LevelCollisionSourceWorldBuilder2DConfig {
	float rotationToleranceRadians = 0.001F;
};

enum class LevelCollisionSourceWorldBuilder2DIssueCode {
	NonPositiveBoxSize,
	UnsupportedRotation,
	CollisionWorldBuildFailed,
};

struct LevelCollisionSourceWorldBuilder2DIssue {
	LevelCollisionSourceWorldBuilder2DIssueCode code = LevelCollisionSourceWorldBuilder2DIssueCode::NonPositiveBoxSize;
	std::size_t boxIndex = 0;
	LevelCollisionSourceBox2D box;
};

struct LevelCollisionSourceWorldBuilder2DResult {
	bool built = false;
	physics2d::CollisionWorld2D world;
	std::vector<LevelCollisionSourceWorldBuilder2DIssue> issues;
	std::size_t sourceBoxCount = 0;
	std::size_t generatedObjectCount = 0;

	[[nodiscard]] bool hasIssues() const;
};

class LevelCollisionSourceWorldBuilder2D {
public:
	[[nodiscard]] LevelCollisionSourceWorldBuilder2DResult build(
		const LevelCollisionSource2D &source,
		const LevelCollisionSourceWorldBuilder2DConfig &config = {}) const;
};

} // namespace iggy
