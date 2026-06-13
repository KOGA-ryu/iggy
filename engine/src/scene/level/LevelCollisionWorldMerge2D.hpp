#pragma once

#include <cstddef>
#include <vector>

#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy {

enum class LevelCollisionWorldMerge2DIssueCode {
	DuplicateObjectId,
	WorldBuildFailed,
};

struct LevelCollisionWorldMerge2DIssue {
	LevelCollisionWorldMerge2DIssueCode code = LevelCollisionWorldMerge2DIssueCode::DuplicateObjectId;
	std::size_t objectIndex = 0;
	physics2d::CollisionObject2D object;
};

struct LevelCollisionWorldMerge2DResult {
	bool built = false;
	physics2d::CollisionWorld2D world;
	std::vector<LevelCollisionWorldMerge2DIssue> issues;
	std::size_t primaryObjectCount = 0;
	std::size_t secondaryObjectCount = 0;
	std::size_t mergedObjectCount = 0;

	[[nodiscard]] bool hasIssues() const;
};

class LevelCollisionWorldMerge2D {
public:
	[[nodiscard]] LevelCollisionWorldMerge2DResult merge(
		const physics2d::CollisionWorld2D &primary,
		const physics2d::CollisionWorld2D &secondary) const;
};

} // namespace iggy
