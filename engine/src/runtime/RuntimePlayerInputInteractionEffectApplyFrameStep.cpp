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
		input.inventory,
		input.requiredItems,
	};
}

const ResourceId *RequiredItemForTarget(
	const RuntimeInteractionRequiredItems &requiredItems,
	const ResourceId &targetId)
{
	for (const RuntimeInteractionRequiredItem &required : requiredItems) {
		if (required.targetId == targetId && !required.itemId.empty())
			return &required.itemId;
	}
	return nullptr;
}

InteractionEffectCatalog2D EffectsAllowedByInventory(
	const InteractionEffectCatalog2D &effects,
	const RuntimeInventoryState &inventory,
	const RuntimeInteractionRequiredItems &requiredItems)
{
	if (requiredItems.empty())
		return effects;

	std::vector<InteractionEffectEntry2D> entries;
	entries.reserve(effects.entries().size());
	for (const InteractionEffectEntry2D &entry : effects.entries()) {
		const ResourceId *requiredItem =
			RequiredItemForTarget(requiredItems, entry.targetId);
		if (requiredItem != nullptr &&
			!inventory.inventory.contains(*requiredItem)) {
			continue;
		}
		entries.push_back(entry);
	}

	const InteractionEffectCatalog2DBuildResult build =
		InteractionEffectCatalog2DBuilder {}.build(entries);
	return build.built ? build.catalog : InteractionEffectCatalog2D {};
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
		EffectsAllowedByInventory(
			input.interactionEffects,
			input.inventory,
			input.requiredItems),
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
		EffectsAllowedByInventory(
			input.interactionEffects,
			input.inventory,
			input.requiredItems),
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
