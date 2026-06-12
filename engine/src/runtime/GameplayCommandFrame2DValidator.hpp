#pragma once

#include <cstddef>
#include <vector>

#include "runtime/GameplayCommand2D.hpp"

namespace iggy::runtime {

struct GameplayCommandFrame2DInvalidCommand {
	std::size_t index = 0;
	GameplayCommand2DStatus status = GameplayCommand2DStatus::Valid;
	GameplayCommand2D command;
};

struct GameplayCommandFrame2DValidationResult {
	bool hasInvalidCommands = false;
	GameplayCommandFrame2D acceptedFrame;
	std::vector<GameplayCommandFrame2DInvalidCommand> invalidCommands;
};

class GameplayCommandFrame2DValidator {
public:
	[[nodiscard]] GameplayCommandFrame2DValidationResult validate(const GameplayCommandFrame2D &frame) const;
};

} // namespace iggy::runtime
