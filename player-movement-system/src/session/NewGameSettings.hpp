#pragma once

#include "world/Point.hpp"

namespace dev {

struct NewGameSettings {
	Point playerStart;
	int playerHitPoints = 20;
};

} // namespace dev
