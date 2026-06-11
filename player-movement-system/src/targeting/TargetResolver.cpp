#include "TargetResolver.hpp"

namespace dev {

Target TargetResolver::resolveAtTile(Point tile) const
{
	return Target {
		.type = TargetType::EmptyTile,
		.id = 0,
		.tile = tile,
	};
}

} // namespace dev

