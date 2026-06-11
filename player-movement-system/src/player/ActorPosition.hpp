#pragma once

#include "world/Point.hpp"

namespace dev {

struct ActorPosition {
	Point tile;
	Point future;
	Point previous;
	Point precise;
};

} // namespace dev

