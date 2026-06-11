#pragma once

#include "targeting/Target.hpp"
#include "world/Point.hpp"

namespace dev {

class TargetResolver {
public:
	Target resolveAtTile(Point tile) const;
};

} // namespace dev

