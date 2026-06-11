#pragma once

#include "world/Point.hpp"

namespace dev {

class EnemyPursuitStepPlanner {
public:
	[[nodiscard]] Point nextStepToward(Point from, Point to) const;
};

} // namespace dev
