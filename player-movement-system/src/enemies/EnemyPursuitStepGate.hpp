#pragma once

#include "world/Collision.hpp"
#include "world/Point.hpp"
#include "world/TileMap.hpp"

namespace dev {

class EnemyPursuitStepGate {
public:
	EnemyPursuitStepGate(const TileMap &map, const Collision &collision);

	[[nodiscard]] bool canEnter(Point tile) const;

private:
	const TileMap &map_;
	const Collision &collision_;
};

} // namespace dev
