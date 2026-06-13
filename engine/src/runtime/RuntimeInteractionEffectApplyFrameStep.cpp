#include "runtime/RuntimeInteractionEffectApplyFrameStep.hpp"

namespace iggy::runtime {

bool RuntimeInteractionEffectApplyFrameResult::hasInteractions() const
{
	return !entries.empty();
}

RuntimeInteractionEffectApplyFrameResult RuntimeInteractionEffectApplyFrameStep::apply(
	const RuntimeSessionState &session,
	const InteractionTarget2DRegistry &targets,
	const InteractionEffectCatalog2D &effects,
	const GameplayCommandFrame2D &frame,
	const InteractionReach2DConfig &reachConfig) const
{
	RuntimeInteractionEffectApplyFrameResult result;
	result.registry = targets;

	InteractionTarget2DRegistry current = targets;
	for (std::size_t index = 0; index < frame.commands.size(); ++index) {
		const GameplayCommand2D &command = frame.commands[index];
		if (command.type != GameplayCommand2DType::Interact)
			continue;

		RuntimeInteractionEffectApplyFrameEntry entry;
		entry.commandIndex = index;
		entry.result = RuntimeInteractionEffectApplyStep {}.apply(session, current, effects, command, reachConfig);
		result.entries.push_back(entry);
		for (const InteractionEvent2D &event : entry.result.events.events)
			recordInteractionEvent(result.events, event);

		current = entry.result.registry;
		result.registry = current;

		if (entry.result.status == RuntimeInteractionEffectApplyStatus::Applied) {
			++result.appliedCount;
			result.mutated = result.mutated || entry.result.mutated;
		} else if (entry.result.status == RuntimeInteractionEffectApplyStatus::NoOp) {
			++result.noOpCount;
		} else if (entry.result.status == RuntimeInteractionEffectApplyStatus::InteractionNotReady) {
			++result.notReadyCount;
		} else if (entry.result.status == RuntimeInteractionEffectApplyStatus::Failed) {
			++result.failedCount;
			result.mutated = result.mutated || entry.result.mutated;
			result.status = RuntimeInteractionEffectApplyFrameStatus::Failed;
			return result;
		}
	}

	result.status = result.mutated
		? RuntimeInteractionEffectApplyFrameStatus::Applied
		: RuntimeInteractionEffectApplyFrameStatus::NoOp;
	return result;
}

} // namespace iggy::runtime
