#include "runtime/RuntimeGameplayProductInputFrameTargetAction.hpp"

namespace iggy::runtime {
namespace {

RuntimeGameplayProductInputEvent2D InteractEvent(ResourceId targetId)
{
	RuntimeGameplayProductInputEvent2D event;
	event.control = RuntimeGameplayProductInputControl2D::Interact;
	event.kind = RuntimeGameplayProductInputEventKind::Pressed;
	event.hasTargetId = true;
	event.targetId = targetId;
	return event;
}

bool CanSynthesizeInteract(
	const RuntimeGameplayProductInputFrameTargetContextResult &targetContext)
{
	return targetContext.status ==
			RuntimeGameplayProductInputFrameTargetContextStatus::TargetProjected &&
		targetContext.hasPrimaryTileEvent &&
		targetContext.primaryTileEventIndex < targetContext.frame.events.size() &&
		targetContext.target.status ==
			RuntimeGameplayProductInteractionTargetQueryStatus::TargetFound &&
		targetContext.target.hasTarget &&
		!targetContext.target.targetId.empty();
}

} // namespace

RuntimeGameplayProductInputFrameTargetActionResult
RuntimeGameplayProductInputFrameTargetAction::synthesize(
	const RuntimeGameplayProductInputFrameTargetActionInput &input) const
{
	RuntimeGameplayProductInputFrameTargetActionResult result;
	result.frame = input.targetContext.frame;

	if (!CanSynthesizeInteract(input.targetContext))
		return result;

	result.status =
		RuntimeGameplayProductInputFrameTargetActionStatus::InteractSynthesized;
	result.hasActionEvent = true;
	result.actionEventIndex = input.targetContext.primaryTileEventIndex;
	result.targetId = input.targetContext.target.targetId;
	result.frame.events[result.actionEventIndex] = InteractEvent(result.targetId);
	return result;
}

} // namespace iggy::runtime
