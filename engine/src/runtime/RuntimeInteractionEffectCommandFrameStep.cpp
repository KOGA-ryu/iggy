#include "runtime/RuntimeInteractionEffectCommandFrameStep.hpp"

namespace iggy::runtime {

bool RuntimeInteractionEffectCommandFrameResult::hasInteractions() const
{
	return !interactions.empty();
}

RuntimeInteractionEffectCommandFrameResult RuntimeInteractionEffectCommandFrameStep::evaluate(
	const RuntimeSessionState &session,
	const InteractionTarget2DRegistry &targets,
	const InteractionEffectCatalog2D &effects,
	const GameplayCommandFrame2D &frame,
	const InteractionReach2DConfig &reachConfig) const
{
	RuntimeInteractionEffectCommandFrameResult result;

	for (std::size_t index = 0; index < frame.commands.size(); ++index) {
		const GameplayCommand2D &command = frame.commands[index];
		if (command.type != GameplayCommand2DType::Interact)
			continue;

		RuntimeInteractionEffectCommandFrameEntry entry;
		entry.commandIndex = index;
		entry.result = RuntimeInteractionEffectCommandStep {}.evaluate(session, targets, effects, command, reachConfig);

		if (entry.result.status == RuntimeInteractionEffectCommandStatus::Ready)
			++result.readyCount;
		else if (entry.result.status == RuntimeInteractionEffectCommandStatus::NoEffects)
			++result.noEffectCount;
		else
			++result.blockedCount;
		result.requestedEffectCount += entry.result.effects.effects.size();
		result.interactions.push_back(entry);
	}

	return result;
}

} // namespace iggy::runtime
