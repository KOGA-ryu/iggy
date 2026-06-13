#include "scene/interaction/InteractionEffectPlan2D.hpp"

namespace iggy {

bool InteractionEffectPlan2DResult::ready() const
{
	return status == InteractionEffectPlan2DStatus::Ready;
}

InteractionEffectPlan2DResult InteractionEffectPlan2D::plan(
	const InteractionPlan2DResult &interaction,
	const InteractionEffectCatalog2D &catalog) const
{
	InteractionEffectPlan2DResult result;
	result.interaction = interaction;

	if (!interaction.ready()) {
		result.status = InteractionEffectPlan2DStatus::InteractionNotReady;
		return result;
	}

	const std::vector<InteractionEffect2D> *effects = catalog.find(interaction.targetId);
	if (effects == nullptr) {
		result.status = InteractionEffectPlan2DStatus::NoEffects;
		return result;
	}

	result.status = InteractionEffectPlan2DStatus::Ready;
	result.effects = *effects;
	return result;
}

} // namespace iggy
