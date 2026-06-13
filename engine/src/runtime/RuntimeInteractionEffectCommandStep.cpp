#include "runtime/RuntimeInteractionEffectCommandStep.hpp"

namespace {

iggy::runtime::RuntimeInteractionEffectCommandStatus StatusForEffectPlan(
	const iggy::runtime::RuntimeInteractionCommandResult &interaction,
	const iggy::InteractionEffectPlan2DResult &effects)
{
	if (!interaction.ready())
		return iggy::runtime::RuntimeInteractionEffectCommandStatus::InteractionNotReady;
	if (effects.status == iggy::InteractionEffectPlan2DStatus::Ready)
		return iggy::runtime::RuntimeInteractionEffectCommandStatus::Ready;
	if (effects.status == iggy::InteractionEffectPlan2DStatus::NoEffects)
		return iggy::runtime::RuntimeInteractionEffectCommandStatus::NoEffects;
	return iggy::runtime::RuntimeInteractionEffectCommandStatus::InteractionNotReady;
}

} // namespace

namespace iggy::runtime {

bool RuntimeInteractionEffectCommandResult::ready() const
{
	return status == RuntimeInteractionEffectCommandStatus::Ready;
}

RuntimeInteractionEffectCommandResult RuntimeInteractionEffectCommandStep::evaluate(
	const RuntimeSessionState &session,
	const InteractionTarget2DRegistry &targets,
	const InteractionEffectCatalog2D &effects,
	const GameplayCommand2D &command,
	const InteractionReach2DConfig &reachConfig) const
{
	RuntimeInteractionEffectCommandResult result;
	result.interaction = RuntimeInteractionCommandStep {}.evaluate(session, targets, command, reachConfig);

	if (result.interaction.status == RuntimeInteractionCommandStatus::NotInteractCommand) {
		result.status = RuntimeInteractionEffectCommandStatus::NotInteractCommand;
		return result;
	}
	if (result.interaction.status == RuntimeInteractionCommandStatus::MissingPlayer) {
		result.status = RuntimeInteractionEffectCommandStatus::MissingPlayer;
		return result;
	}

	result.effects = InteractionEffectPlan2D {}.plan(result.interaction.plan, effects);
	result.status = StatusForEffectPlan(result.interaction, result.effects);
	return result;
}

} // namespace iggy::runtime
