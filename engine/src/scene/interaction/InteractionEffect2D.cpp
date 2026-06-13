#include "scene/interaction/InteractionEffect2D.hpp"

#include <utility>

namespace iggy {

InteractionEffect2D noneInteractionEffect()
{
	return {};
}

InteractionEffect2D inspectTextInteractionEffect(ResourceId targetId, std::string text)
{
	InteractionEffect2D effect;
	effect.type = InteractionEffect2DType::InspectText;
	effect.targetId = std::move(targetId);
	effect.text = std::move(text);
	return effect;
}

InteractionEffect2D toggleTargetInteractionEffect(ResourceId targetId, bool enabledValue)
{
	InteractionEffect2D effect;
	effect.type = InteractionEffect2DType::ToggleTarget;
	effect.targetId = std::move(targetId);
	effect.enabledValue = enabledValue;
	return effect;
}

InteractionEffect2D emitInteractionEventEffect(ResourceId targetId, ResourceId eventId)
{
	InteractionEffect2D effect;
	effect.type = InteractionEffect2DType::EmitEvent;
	effect.targetId = std::move(targetId);
	effect.eventId = std::move(eventId);
	return effect;
}

InteractionEffect2D pickupItemInteractionEffect(ResourceId targetId, ResourceId dropId)
{
	InteractionEffect2D effect;
	effect.type = InteractionEffect2DType::PickupItem;
	effect.targetId = std::move(targetId);
	effect.dropId = std::move(dropId);
	return effect;
}

InteractionEffect2DStatus validate(const InteractionEffect2D &effect)
{
	if (effect.type == InteractionEffect2DType::InspectText && effect.text.empty())
		return InteractionEffect2DStatus::MissingText;
	if (effect.type == InteractionEffect2DType::ToggleTarget && effect.targetId.empty())
		return InteractionEffect2DStatus::MissingTarget;
	if (effect.type == InteractionEffect2DType::EmitEvent && effect.eventId.empty())
		return InteractionEffect2DStatus::MissingEvent;
	if (effect.type == InteractionEffect2DType::PickupItem && effect.dropId.empty())
		return InteractionEffect2DStatus::MissingDropId;
	return InteractionEffect2DStatus::Valid;
}

bool valid(const InteractionEffect2D &effect)
{
	return validate(effect) == InteractionEffect2DStatus::Valid;
}

} // namespace iggy
