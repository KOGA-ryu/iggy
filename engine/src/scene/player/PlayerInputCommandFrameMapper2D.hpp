#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "runtime/GameplayCommand2D.hpp"
#include "scene/player/PlayerInputCommandMapper2D.hpp"
#include "scene/player/PlayerInputIntent2D.hpp"

namespace iggy {

struct PlayerInputCommandFrameMapIssue {
	std::size_t intentIndex = 0;
	PlayerInputCommandMapper2DResult map;
	PlayerInputIntent2D intent;
};

struct PlayerInputCommandFrameMapper2DResult {
	runtime::GameplayCommandFrame2D frame;
	std::vector<PlayerInputCommandFrameMapIssue> issues;

	[[nodiscard]] bool hasIssues() const;
};

class PlayerInputCommandFrameMapper2D {
public:
	[[nodiscard]] PlayerInputCommandFrameMapper2DResult map(ResourceId actorId, const std::vector<PlayerInputIntent2D> &intents) const;
};

} // namespace iggy
