#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "runtime/GameplayCommand2D.hpp"
#include "scene/player/PlayerInputCommandFrameMapper2D.hpp"
#include "scene/player/PlayerInputIntentGate2D.hpp"

namespace iggy {

struct PlayerInputGatedIntentIssue {
	std::size_t intentIndex = 0;
	PlayerInputIntentGate2DResult gate;
};

struct PlayerInputGatedCommandFrameMapper2DResult {
	runtime::GameplayCommandFrame2D frame;
	std::vector<PlayerInputGatedIntentIssue> gateIssues;
	PlayerInputCommandFrameMapper2DResult mapping;

	[[nodiscard]] bool hasIssues() const;
};

class PlayerInputGatedCommandFrameMapper2D {
public:
	[[nodiscard]] PlayerInputGatedCommandFrameMapper2DResult map(
		ResourceId actorId,
		const PlayerInputContext2D &context,
		const std::vector<PlayerInputIntent2D> &intents) const;
};

} // namespace iggy
