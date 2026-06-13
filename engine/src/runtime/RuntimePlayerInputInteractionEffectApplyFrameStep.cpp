#include "runtime/RuntimePlayerInputInteractionEffectApplyFrameStep.hpp"

namespace iggy::runtime {

namespace {

RuntimePlayerInputInteractionEffectApplyFrameInput SeparateInput(
	const RuntimePlayerInputInteractionStateApplyFrameInput &input)
{
	return {
		input.playerInput,
		input.interaction.targets,
		input.interaction.effects,
		input.interactionReach,
	};
}

RuntimePlayerInputInteractionStateApplyFrameResult StateResultFrom(
	const RuntimePlayerInputInteractionEffectApplyFrameResult &separate,
	const RuntimeInteractionState &inputInteraction)
{
	RuntimePlayerInputInteractionStateApplyFrameResult result;
	result.playerInput = separate.playerInput;
	result.application = separate.application;
	result.session = separate.session;
	result.queue = separate.queue;
	result.interaction.targets = separate.interactionTargets;
	result.interaction.effects = inputInteraction.effects;
	result.events = separate.events;
	return result;
}

} // namespace

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
	result.events = result.application.events;
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
	result.events = result.application.events;
	return result;
}

RuntimePlayerInputInteractionStateApplyFrameResult RuntimePlayerInputInteractionEffectApplyFrameStep::runWithInteractionState(
	const RuntimePlayerInputInteractionStateApplyFrameInput &input) const
{
	return StateResultFrom(run(SeparateInput(input)), input.interaction);
}

RuntimePlayerInputInteractionStateApplyFrameResult RuntimePlayerInputInteractionEffectApplyFrameStep::runWithInteractionState(
	const RuntimePlayerInputInteractionStateApplyFrameInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	return StateResultFrom(run(SeparateInput(input), explicitWorld), input.interaction);
}

} // namespace iggy::runtime
