#include "runtime/RuntimeNpcActorMovementFrameStep.hpp"

namespace iggy::runtime {

RuntimeNpcActorMovementFrameResult RuntimeNpcActorMovementFrameStep::run(
	const RuntimeNpcActorMovementFrameInput &input) const
{
	RuntimeNpcActorMovementFrameResult result;
	result.apply = NpcActorMovementFrameApplier2D {}.apply(input.state.npcActors, input.requests);
	result.report = NpcActorMovementFrameReporter2D {}.report(result.apply);
	result.state = input.state;
	result.state.npcActors = result.apply.registry;
	result.changed = result.report.changed();
	return result;
}

} // namespace iggy::runtime
