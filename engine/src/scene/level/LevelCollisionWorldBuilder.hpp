#pragma once

#include <cstddef>
#include <vector>

#include "scene/level/LevelTileMap.hpp"
#include "scene/level/TileCoord.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy {

enum class LevelCollisionWorldBuildIssueCode {
	CollisionWorldBuildFailed,
};

struct LevelCollisionWorldBuildIssue {
	LevelCollisionWorldBuildIssueCode code = LevelCollisionWorldBuildIssueCode::CollisionWorldBuildFailed;
	TileCoord tile;
	std::size_t tileIndex = 0;
};

struct LevelCollisionWorldBuildResult {
	bool built = false;
	physics2d::CollisionWorld2D world;
	std::vector<LevelCollisionWorldBuildIssue> issues;
};

class LevelCollisionWorldBuilder {
public:
	[[nodiscard]] LevelCollisionWorldBuildResult build(const LevelTileMap &map) const;
};

} // namespace iggy
