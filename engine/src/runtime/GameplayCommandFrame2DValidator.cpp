#include "runtime/GameplayCommandFrame2DValidator.hpp"

namespace iggy::runtime {

GameplayCommandFrame2DValidationResult GameplayCommandFrame2DValidator::validate(const GameplayCommandFrame2D &frame) const
{
	GameplayCommandFrame2DValidationResult result;

	for (std::size_t index = 0; index < frame.commands.size(); ++index) {
		const GameplayCommand2D command = frame.commands[index];
		const GameplayCommand2DStatus status = iggy::runtime::validate(command);
		if (status == GameplayCommand2DStatus::Valid) {
			result.acceptedFrame.commands.push_back(command);
			continue;
		}

		result.hasInvalidCommands = true;
		result.invalidCommands.push_back({ index, status, command });
	}

	return result;
}

} // namespace iggy::runtime
