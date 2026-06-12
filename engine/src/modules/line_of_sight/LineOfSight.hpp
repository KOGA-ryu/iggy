#pragma once

#include "core/math/Vec2.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy::line_of_sight {

enum class LineOfSightStatus {
	Visible,
	Blocked,
	StartOutOfBounds,
	EndOutOfBounds,
	StartBlocked,
	EndBlocked,
};

struct LineOfSightTrace {
	LineOfSightStatus status = LineOfSightStatus::Visible;
	TileCoord startTile;
	TileCoord endTile;
	TileCoord hitTile;
	Vec2 hitPoint;
	float distance = 0.0F;

	[[nodiscard]] bool visible() const;
	[[nodiscard]] bool blocked() const;
	[[nodiscard]] bool valid() const;
};

[[nodiscard]] LineOfSightTrace Trace(const LevelTileMap &map, Vec2 from, Vec2 to);
[[nodiscard]] bool CanSee(const LevelTileMap &map, Vec2 from, Vec2 to);

} // namespace iggy::line_of_sight
