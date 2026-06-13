#include "runtime/RuntimePlayerInputInteractionEffectFrameStep.hpp"

namespace iggy::runtime {

RuntimePlayerInputInteractionEffectFrameResult RuntimePlayerInputInteractionEffectFrameStep::run(
	const RuntimePlayerInputInteractionEffectFrameInput &input) const
{
	RuntimePlayerInputInteractionEffectFrameResult result;
	result.playerInput = RuntimePlayerInputFrameStep {}.runGated(input.playerInput);
	result.interactions = RuntimeInteractionEffectCommandFrameStep {}.evaluate(
		result.playerInput.session,
		input.interactionTargets,
		input.interactionEffects,
		result.playerInput.command.intake.mapping.frame,
		input.interactionReach);
	result.session = result.playerInput.session;
	result.queue = result.playerInput.queue;
	return result;
}

RuntimePlayerInputInteractionEffectFrameResult RuntimePlayerInputInteractionEffectFrameStep::run(
	const RuntimePlayerInputInteractionEffectFrameInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	RuntimePlayerInputInteractionEffectFrameResult result;
	result.playerInput = RuntimePlayerInputFrameStep {}.runGated(input.playerInput, explicitWorld);
	result.interactions = RuntimeInteractionEffectCommandFrameStep {}.evaluate(
		result.playerInput.session,
		input.interactionTargets,
		input.interactionEffects,
		result.playerInput.command.intake.mapping.frame,
		input.interactionReach);
	result.session = result.playerInput.session;
	result.queue = result.playerInput.queue;
	return result;
}

} // namespace iggy::runtime
