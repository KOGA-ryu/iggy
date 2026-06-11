#pragma once

#include <cstdint>

#include "world/Point.hpp"

namespace dev {

using TargetId = uint32_t;

enum class TargetType {
	EmptyTile,
	Item,
	Enemy,
	Npc,
	Object,
};

struct Target {
	TargetType type = TargetType::EmptyTile;
	TargetId id = 0;
	Point tile;
};

} // namespace dev

