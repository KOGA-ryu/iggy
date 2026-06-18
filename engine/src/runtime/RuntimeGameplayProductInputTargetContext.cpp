#include "runtime/RuntimeGameplayProductInputTargetContext.hpp"

namespace iggy::runtime {

RuntimeGameplayProductInputTargetContextResult RuntimeGameplayProductInputTargetContext::project(
	const RuntimeGameplayProductInputTargetContextInput &input) const
{
	RuntimeGameplayProductInputTargetContextResult result;
	result.bindingContext = input.base;

	if (input.target.status !=
			RuntimeGameplayProductInteractionTargetQueryStatus::TargetFound ||
		!input.target.hasTarget || input.target.targetId.empty())
		return result;

	result.status =
		RuntimeGameplayProductInputTargetContextStatus::TargetProjected;
	result.bindingContext.hasHoveredTargetId = true;
	result.bindingContext.hoveredTargetId = input.target.targetId;
	result.hoveredTargetId = input.target.targetId;
	return result;
}

} // namespace iggy::runtime
