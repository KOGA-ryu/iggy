#include "runtime/RuntimePlayerInputInteractionEffectApplyFrameStep.hpp"

namespace iggy::runtime {

RuntimePlayerInputInteractionEffectApplyFrameResult RuntimePlayerInputInteractionEffectApplyFrameStep::run(
	const RuntimePlayerInputInteractionEffectApplyFrameInput &input) const
{
	RuntimePlayerInputInteractionEffectApplyFrameResult result;
	result.playerInput = RuntimePlayerInputFrameStep {}.runGated(input.playerInput);
	result.session = result.playerInput.session;
	result.queue = result.playerInput.queue;
	if (result.playerInput.command.status == RuntimePlayerInputCommandRunnerStatus::QueueRejected) {
		result.application.registry = input.interactionTargets;
		result.interactionTargets = input.interactionTargets;
		return result;
	}

	result.application = RuntimeInteractionEffectApplyFrameStep {}.apply(
		result.session,
		input.interactionTargets,
		input.interactionEffects,
		result.playerInput.command.intake.mapping.frame,
		input.interactionReach);
	result.interactionTargets = result.application.registry;
	return result;
}

RuntimePlayerInputInteractionEffectApplyFrameResult RuntimePlayerInputInteractionEffectApplyFrameStep::run(
	const RuntimePlayerInputInteractionEffectApplyFrameInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	RuntimePlayerInputInteractionEffectApplyFrameResult result;
	result.playerInput = RuntimePlayerInputFrameStep {}.runGated(input.playerInput, explicitWorld);
	result.session = result.playerInput.session;
	result.queue = result.playerInput.queue;
	if (result.playerInput.command.status == RuntimePlayerInputCommandRunnerStatus::QueueRejected) {
		result.application.registry = input.interactionTargets;
		result.interactionTargets = input.interactionTargets;
		return result;
	}

	result.application = RuntimeInteractionEffectApplyFrameStep {}.apply(
		result.session,
		input.interactionTargets,
		input.interactionEffects,
		result.playerInput.command.intake.mapping.frame,
		input.interactionReach);
	result.interactionTargets = result.application.registry;
	return result;
}

} // namespace iggy::runtime
