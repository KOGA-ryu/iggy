#pragma once

#include "core/math/Vec2.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "servers/navigation/NavigationPath.hpp"
#include "servers/navigation/NavigationRequest.hpp"

namespace iggy::navigation {

class NavigationGridPathfinder {
public:
	[[nodiscard]] NavigationPath findPath(const LevelTileMap &map, Vec2 start, const NavigationRequest &request) const;
};

} // namespace iggy::navigation
