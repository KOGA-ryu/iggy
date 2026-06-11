#pragma once

#include "player/ActorPosition.hpp"

namespace dev {

class ActorStepCommitter {
public:
	void commit(ActorPosition &position, Point nextTile) const;
};

} // namespace dev
