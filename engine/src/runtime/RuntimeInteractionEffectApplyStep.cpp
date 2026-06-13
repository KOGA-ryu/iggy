#include "runtime/RuntimeInteractionEffectApplyStep.hpp"

namespace {

iggy::runtime::RuntimeInteractionEffectApplyStatus RuntimeStatusForApplication(
	iggy::InteractionEffectPlanApplyStatus status)
{
	if (status == iggy::InteractionEffectPlanApplyStatus::Applied)
		return iggy::runtime::RuntimeInteractionEffectApplyStatus::Applied;
	if (status == iggy::InteractionEffectPlanApplyStatus::NoOp)
		return iggy::runtime::RuntimeInteractionEffectApplyStatus::NoOp;
	if (status == iggy::InteractionEffectPlanApplyStatus::Failed)
		return iggy::runtime::RuntimeInteractionEffectApplyStatus::Failed;
	return iggy::runtime::RuntimeInteractionEffectApplyStatus::InteractionNotReady;
}

} // namespace

namespace iggy::runtime {

RuntimeInteractionEffectApplyResult RuntimeInteractionEffectApplyStep::apply(
	const RuntimeSessionState &session,
	const InteractionTarget2DRegistry &targets,
	const InteractionEffectCatalog2D &effects,
	const GameplayCommand2D &command,
	const InteractionReach2DConfig &reachConfig) const
{
	RuntimeInteractionEffectApplyResult result;
	result.registry = targets;
	result.command = RuntimeInteractionEffectCommandStep {}.evaluate(session, targets, effects, command, reachConfig);

	if (result.command.status == RuntimeInteractionEffectCommandStatus::NotInteractCommand
		|| result.command.status == RuntimeInteractionEffectCommandStatus::MissingPlayer
		|| result.command.status == RuntimeInteractionEffectCommandStatus::InteractionNotReady) {
		result.status = RuntimeInteractionEffectApplyStatus::InteractionNotReady;
		return result;
	}

	if (result.command.status == RuntimeInteractionEffectCommandStatus::NoEffects) {
		result.status = RuntimeInteractionEffectApplyStatus::NoOp;
		return result;
	}

	result.application = InteractionEffectPlanApplier2D {}.apply(targets, result.command.effects);
	result.status = RuntimeStatusForApplication(result.application.status);
	result.registry = result.application.registry;
	result.mutated = result.application.mutated;
	return result;
}

} // namespace iggy::runtime
