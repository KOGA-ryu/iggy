#include "runtime/RuntimeInteractionCommandFrameStep.hpp"

namespace iggy::runtime {

bool RuntimeInteractionCommandFrameResult::hasInteractions() const
{
	return !interactions.empty();
}

RuntimeInteractionCommandFrameResult RuntimeInteractionCommandFrameStep::evaluate(
	const RuntimeSessionState &session,
	const InteractionTarget2DRegistry &registry,
	const GameplayCommandFrame2D &frame,
	const InteractionReach2DConfig &reachConfig) const
{
	RuntimeInteractionCommandFrameResult result;

	for (std::size_t index = 0; index < frame.commands.size(); ++index) {
		const GameplayCommand2D &command = frame.commands[index];
		if (command.type != GameplayCommand2DType::Interact)
			continue;

		RuntimeInteractionCommandFrameEntry entry;
		entry.commandIndex = index;
		entry.result = RuntimeInteractionCommandStep {}.evaluate(session, registry, command, reachConfig);
		if (entry.result.ready())
			++result.readyCount;
		else
			++result.blockedCount;
		result.interactions.push_back(entry);
	}

	return result;
}

} // namespace iggy::runtime
