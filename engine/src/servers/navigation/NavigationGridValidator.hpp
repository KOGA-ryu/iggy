#pragma once

#include "scene/level/LevelTileMap.hpp"
#include "servers/navigation/NavigationRequest.hpp"

namespace iggy::navigation {

class NavigationGridValidator {
public:
	[[nodiscard]] NavigationRequest validate(const LevelTileMap &map, const NavigationGridValidationInput &input) const;
};

} // namespace iggy::navigation
