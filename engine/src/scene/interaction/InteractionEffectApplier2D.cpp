#include "scene/interaction/InteractionEffectApplier2D.hpp"

namespace {

iggy::InteractionEffectApplyStatus StatusForToggle(const iggy::InteractionTargetToggle2DResult &toggle)
{
	if (toggle.status == iggy::InteractionTargetToggle2DStatus::Toggled)
		return iggy::InteractionEffectApplyStatus::Applied;
	if (toggle.status == iggy::InteractionTargetToggle2DStatus::NoChange)
		return iggy::InteractionEffectApplyStatus::NoOp;
	return iggy::InteractionEffectApplyStatus::TargetMissing;
}

} // namespace

namespace iggy {

InteractionEffectApplyResult InteractionEffectApplier2D::apply(
	const InteractionTarget2DRegistry &registry,
	const InteractionEffect2D &effect) const
{
	InteractionEffectApplyResult result;
	result.registry = registry;
	result.effect = effect;
	result.effectStatus = validate(effect);

	if (result.effectStatus != InteractionEffect2DStatus::Valid) {
		result.status = InteractionEffectApplyStatus::InvalidEffect;
		return result;
	}

	if (effect.type == InteractionEffect2DType::None) {
		result.status = InteractionEffectApplyStatus::NoOp;
		return result;
	}

	if (effect.type == InteractionEffect2DType::InspectText || effect.type == InteractionEffect2DType::EmitEvent) {
		result.status = InteractionEffectApplyStatus::Deferred;
		return result;
	}

	result.toggle = InteractionTargetToggle2D {}.apply(registry, effect.targetId, effect.enabledValue);
	result.status = StatusForToggle(result.toggle);
	result.registry = result.toggle.registry;
	result.mutated = result.toggle.changed;
	return result;
}

} // namespace iggy
