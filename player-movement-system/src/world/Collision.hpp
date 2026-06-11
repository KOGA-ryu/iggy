#pragma once

#include "world/Point.hpp"

namespace dev {

class Collision {
public:
	bool blocksMovement(Point tile) const;
};

} // namespace dev

