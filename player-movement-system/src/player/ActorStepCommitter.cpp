#include "ActorStepCommitter.hpp"

namespace dev {

void ActorStepCommitter::commit(ActorPosition &position, Point nextTile) const
{
	position.previous = position.tile;
	position.future = nextTile;
	position.tile = nextTile;
	position.precise = nextTile;
}

} // namespace dev
