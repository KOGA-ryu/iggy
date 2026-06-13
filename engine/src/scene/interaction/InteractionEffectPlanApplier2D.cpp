#include "scene/interaction/InteractionEffectPlanApplier2D.hpp"

namespace {

bool FailedEffect(iggy::InteractionEffectApplyStatus status)
{
	return status == iggy::InteractionEffectApplyStatus::InvalidEffect
		|| status == iggy::InteractionEffectApplyStatus::TargetMissing;
}

} // namespace

namespace iggy {

InteractionEffectPlanApplyResult InteractionEffectPlanApplier2D::apply(
	const InteractionTarget2DRegistry &registry,
	const InteractionEffectPlan2DResult &plan) const
{
	InteractionEffectPlanApplyResult result;
	result.registry = registry;
	result.plan = plan;

	if (!plan.ready()) {
		result.status = InteractionEffectPlanApplyStatus::InteractionNotReady;
		return result;
	}

	if (plan.effects.empty()) {
		result.status = InteractionEffectPlanApplyStatus::NoOp;
		return result;
	}

	InteractionTarget2DRegistry current = registry;
	for (std::size_t index = 0; index < plan.effects.size(); ++index) {
		InteractionEffectPlanApplyEntry entry;
		entry.effectIndex = index;
		entry.result = InteractionEffectApplier2D {}.apply(current, plan.effects[index]);
		result.entries.push_back(entry);
		for (const InteractionEvent2D &event : entry.result.events.events)
			recordInteractionEvent(result.events, event);

		if (entry.result.status == InteractionEffectApplyStatus::Applied)
			++result.appliedCount;
		else if (entry.result.status == InteractionEffectApplyStatus::Deferred)
			++result.deferredCount;
		else if (entry.result.status == InteractionEffectApplyStatus::NoOp)
			++result.noOpCount;
		else if (FailedEffect(entry.result.status))
			++result.failedCount;

		current = entry.result.registry;
		result.registry = current;
		result.mutated = result.mutated || entry.result.mutated;

		if (FailedEffect(entry.result.status)) {
			result.status = InteractionEffectPlanApplyStatus::Failed;
			return result;
		}
	}

	result.status = result.mutated ? InteractionEffectPlanApplyStatus::Applied : InteractionEffectPlanApplyStatus::NoOp;
	return result;
}

} // namespace iggy
