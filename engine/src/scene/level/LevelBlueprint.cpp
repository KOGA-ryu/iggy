#include "scene/level/LevelBlueprint.hpp"

namespace iggy {

bool LevelBounds::contains(int x, int y) const
{
	return x >= 0 && y >= 0 && x < width && y < height;
}

} // namespace iggy
