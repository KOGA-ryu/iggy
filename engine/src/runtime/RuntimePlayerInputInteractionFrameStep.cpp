#include "runtime/RuntimePlayerInputInteractionFrameStep.hpp"

namespace iggy::runtime {

RuntimePlayerInputInteractionFrameResult RuntimePlayerInputInteractionFrameStep::run(const RuntimePlayerInputInteractionFrameInput &input) const
{
	RuntimePlayerInputInteractionFrameResult result;
	result.playerInput = RuntimePlayerInputFrameStep {}.runGated(input.playerInput);
	result.interactions = RuntimeInteractionCommandFrameStep {}.evaluate(
		result.playerInput.session,
		input.interactionTargets,
		result.playerInput.command.intake.mapping.frame,
		input.interactionReach);
	result.session = result.playerInput.session;
	result.queue = result.playerInput.queue;
	return result;
}

RuntimePlayerInputInteractionFrameResult RuntimePlayerInputInteractionFrameStep::run(
	const RuntimePlayerInputInteractionFrameInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	RuntimePlayerInputInteractionFrameResult result;
	result.playerInput = RuntimePlayerInputFrameStep {}.runGated(input.playerInput, explicitWorld);
	result.interactions = RuntimeInteractionCommandFrameStep {}.evaluate(
		result.playerInput.session,
		input.interactionTargets,
		result.playerInput.command.intake.mapping.frame,
		input.interactionReach);
	result.session = result.playerInput.session;
	result.queue = result.playerInput.queue;
	return result;
}

} // namespace iggy::runtime
