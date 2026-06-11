#include "EnemyPursuitStepPlanner.hpp"

namespace dev {

namespace {

int Sign(int value)
{
	if (value == 0)
		return 0;
	return value > 0 ? 1 : -1;
}

} // namespace

Point EnemyPursuitStepPlanner::nextStepToward(Point from, Point to) const
{
	return {
		from.x + Sign(to.x - from.x),
		from.y + Sign(to.y - from.y),
	};
}

} // namespace dev
